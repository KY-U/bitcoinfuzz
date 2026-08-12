use silentpayments::{
    secp256k1::{All, Parity, PublicKey, Scalar, Secp256k1, SecretKey},
    sending::generate_recipient_pubkeys,
    utils::{sending::calculate_partial_secret, OutPoint},
    SilentPaymentAddress,
};
use std::collections::HashMap;
use std::ffi::CString;
use std::os::raw::c_char;
use std::{ptr, slice};

/// Kept byte-for-byte in sync with the peer modules' sentinels: the driver
/// compares the responses verbatim, so a different spelling would read as a
/// mismatch instead of as agreement on rejecting the input.
const INVALID_SECKEY: &str = "INVALID_SECKEY";
const CREATE_FAIL: &str = "CREATE_FAIL";

/// Tells the C++ wrapper to drop the response instead of comparing it. See
/// [`diverges_on_intermediate_zero_sum`] for the single case that uses it.
const SKIP: &str = "SKIP_INTERMEDIATE_ZERO_SUM";

const SECKEY_LEN: usize = 32;
const OUTPOINT_LEN: usize = 36;

/// Creates the BIP-352 Silent Payments outputs for a set of recipients, mirroring
/// the `secp256k1` module's `secp256k1_silentpayments_create_outputs` so the C++
/// driver can compare the two byte-for-byte.
///
/// `silentpayments` takes the eligible input keys directly as `(SecretKey,
/// bool)` pairs rather than deriving eligibility from script pubkeys, so the
/// target's taproot flag maps straight onto the boolean. Taproot keys are
/// negated to their even-Y form before summing, matching libsecp256k1's split of
/// the same keys into keypairs and plain secret keys.
///
/// # Arguments
/// * `outpoint36` - the serialized 36-byte smallest outpoint of the transaction
/// * `input_seckeys` - `n_inputs` concatenated 32-byte input secret keys
/// * `input_is_taproot` - `n_inputs` flags, non-zero for a taproot input
/// * `n_inputs` - number of inputs being spent
/// * `scan_seckeys` - `n_recipients` concatenated 32-byte scan secret keys
/// * `spend_seckeys` - `n_recipients` concatenated 32-byte spend secret keys
/// * `n_recipients` - number of recipients
///
/// # Returns
/// * the concatenated 32-byte x-only outputs in recipient order, hex-encoded;
/// * the "CREATE_FAIL" sentinel when output creation is rejected;
/// * the "INVALID_SECKEY" sentinel when a secret key is out of range;
/// * the "SKIP_INTERMEDIATE_ZERO_SUM" sentinel for the known upstream
///   divergence described on [`diverges_on_intermediate_zero_sum`];
/// * null only when the arguments themselves are malformed.
///
/// Apart from the skip, the sentinels are responses: the driver compares them
/// against the peer modules, so an accept-vs-reject disagreement trips its
/// assert.
///
/// # Safety
/// `outpoint36` must point to 36 valid bytes, `input_seckeys` to `n_inputs * 32`,
/// `input_is_taproot` to `n_inputs`, and `scan_seckeys` and `spend_seckeys` to
/// `n_recipients * 32` each.
#[no_mangle]
pub unsafe extern "C" fn spdk_create_outputs(
    outpoint36: *const u8,
    input_seckeys: *const u8,
    input_is_taproot: *const u8,
    n_inputs: usize,
    scan_seckeys: *const u8,
    spend_seckeys: *const u8,
    n_recipients: usize,
) -> *mut c_char {
    if outpoint36.is_null()
        || input_seckeys.is_null()
        || input_is_taproot.is_null()
        || scan_seckeys.is_null()
        || spend_seckeys.is_null()
        || n_inputs == 0
        || n_recipients == 0
    {
        return ptr::null_mut();
    }

    let outpoint: [u8; OUTPOINT_LEN] =
        match slice::from_raw_parts(outpoint36, OUTPOINT_LEN).try_into() {
            Ok(arr) => arr,
            Err(_) => return ptr::null_mut(),
        };
    let input_seckeys = slice::from_raw_parts(input_seckeys, n_inputs * SECKEY_LEN);
    let is_taproot = slice::from_raw_parts(input_is_taproot, n_inputs);
    let scan_seckeys = slice::from_raw_parts(scan_seckeys, n_recipients * SECKEY_LEN);
    let spend_seckeys = slice::from_raw_parts(spend_seckeys, n_recipients * SECKEY_LEN);

    let secp = Secp256k1::new();

    let mut input_keys: Vec<(SecretKey, bool)> = Vec::with_capacity(n_inputs);
    for i in 0..n_inputs {
        let seckey = match parse_seckey(&input_seckeys[i * SECKEY_LEN..(i + 1) * SECKEY_LEN]) {
            Some(sk) => sk,
            None => return str_to_c_string(INVALID_SECKEY),
        };
        input_keys.push((seckey, is_taproot[i] != 0));
    }

    // Every key is parsed before any protocol step runs, because libsecp256k1
    // validates all of them up front too. Interleaving the two would let one
    // library answer INVALID_SECKEY where the other answers CREATE_FAIL for the
    // same input, which the driver would report as a mismatch.
    let mut addresses: Vec<SilentPaymentAddress> = Vec::with_capacity(n_recipients);
    for i in 0..n_recipients {
        let scan_seckey = match parse_seckey(&scan_seckeys[i * SECKEY_LEN..(i + 1) * SECKEY_LEN]) {
            Some(sk) => sk,
            None => return str_to_c_string(INVALID_SECKEY),
        };
        let spend_seckey = match parse_seckey(&spend_seckeys[i * SECKEY_LEN..(i + 1) * SECKEY_LEN])
        {
            Some(sk) => sk,
            None => return str_to_c_string(INVALID_SECKEY),
        };
        addresses.push(SilentPaymentAddress::new_v0(
            PublicKey::from_secret_key(&secp, &scan_seckey),
            PublicKey::from_secret_key(&secp, &spend_seckey),
        ));
    }

    if diverges_on_intermediate_zero_sum(&secp, &input_keys) {
        return str_to_c_string(SKIP);
    }

    // A single outpoint is passed because the target already picks the smallest
    // one; `calculate_partial_secret` would otherwise select it itself.
    let outpoints = [OutPoint::from_bytes(outpoint)];
    let partial_secret = match calculate_partial_secret(&input_keys, &outpoints) {
        Ok(secret) => secret,
        Err(_) => return str_to_c_string(CREATE_FAIL),
    };

    let payments = match generate_recipient_pubkeys(addresses.clone(), partial_secret) {
        Ok(payments) => payments,
        Err(_) => return str_to_c_string(CREATE_FAIL),
    };

    // The returned map groups the outputs per address, with each group in
    // recipient order. Walk the recipients in their original order and take the
    // n-th output of their address, so the response lines up with libsecp256k1's
    // recipient-ordered output array.
    let mut seen_per_address = <HashMap<&SilentPaymentAddress, usize>>::new();
    let mut result = String::with_capacity(n_recipients * SECKEY_LEN * 2);
    for address in addresses.iter() {
        let nth = seen_per_address.entry(address).or_insert(0);
        let output = match payments.get(address).and_then(|outputs| outputs.get(*nth)) {
            Some(output) => output,
            None => return str_to_c_string(CREATE_FAIL),
        };
        *nth += 1;
        result.push_str(&hex::encode(output.serialize()));
    }

    str_to_c_string(&result)
}

fn parse_seckey(bytes: &[u8]) -> Option<SecretKey> {
    SecretKey::from_slice(bytes).ok()
}

/// Reports the one input shape where `silentpayments` disagrees with BIP-352, so
/// the caller can drop the response instead of letting the driver flag a
/// mismatch that is already understood.
///
/// BIP-352 constrains only the total: "Let ''a = a_1 + a_2 + ... + a_n'' [...]
/// If ''a = 0'', fail". `get_a_sum_secret_keys` folds the keys with
/// `SecretKey::add_tweak`, which rejects zero at every step, so it also fails
/// when an intermediate sum is zero and the total is not. libsecp256k1 only
/// checks the total and succeeds, which is the divergence. It is reachable with,
/// for instance, one taproot input whose key is negated to even-Y and one
/// non-taproot input holding that same key.
///
/// bdk_sp has the same gap and tracks it with an ignored BIP-352 vector; this
/// one has not been reported upstream yet. Skipping keeps every other input
/// comparing this library's own key summation.
///
/// TODO: delete this function and its call site once `silentpayments` sums the
/// input keys without rejecting intermediate zeros. The target then covers these
/// inputs like any other, and the skip would start hiding real regressions.
fn diverges_on_intermediate_zero_sum(
    secp: &Secp256k1<All>,
    input_keys: &[(SecretKey, bool)],
) -> bool {
    // Mirrors get_a_sum_secret_keys: taproot keys are negated to their even-Y
    // form, everything else is summed as it is.
    let keys = input_keys
        .iter()
        .map(|(sk, is_taproot)| {
            let (_, parity) = sk.x_only_public_key(secp);
            if *is_taproot && parity == Parity::Odd {
                sk.negate()
            } else {
                *sk
            }
        })
        .collect::<Vec<SecretKey>>();

    let Some((first, rest)) = keys.split_first() else {
        return false;
    };

    // add_tweak rejects an in-range secret key only when the running sum reaches
    // zero, so an error here is the intermediate zero itself.
    let mut running = *first;
    let mut hit_zero = false;
    for sk in rest {
        match running.add_tweak(&Scalar::from(*sk)) {
            Ok(sum) => running = sum,
            Err(_) => {
                hit_zero = true;
                break;
            }
        }
    }
    if !hit_zero {
        return false;
    }

    // A zero total is a genuine BIP-352 failure that every module reports, so
    // only a non-zero total diverges. Summing the public keys decides that
    // without tripping over the intermediate zero: point addition stays defined
    // for every partial sum and fails only if the total is the point at
    // infinity, which is exactly a = 0.
    let pubkeys = keys
        .iter()
        .map(|sk| sk.public_key(secp))
        .collect::<Vec<PublicKey>>();
    let pubkey_refs = pubkeys.iter().collect::<Vec<&PublicKey>>();
    PublicKey::combine_keys(&pubkey_refs).is_ok()
}

/// Frees a string allocated by this library.
///
/// # Safety
/// Caller must ensure `ptr` was allocated by this library's functions.
#[no_mangle]
pub unsafe extern "C" fn spdk_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        let _ = CString::from_raw(ptr);
    }
}

fn str_to_c_string(input: &str) -> *mut c_char {
    match CString::new(input) {
        Ok(s) => s.into_raw(),
        Err(_) => ptr::null_mut(),
    }
}
