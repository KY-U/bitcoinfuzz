from embit.descriptor.miniscript import Miniscript
from embit.descriptor import Descriptor
from embit.psbt import PSBT
from embit.bip32 import HDKey
from embit.networks import NETWORKS


def miniscript_parse(input):
    try:
        ms = Miniscript.from_string(input, taproot=False)
        ms.verify()
        return True
    except Exception as _:
        try:
            ms = Miniscript.from_string(input, taproot=True)
            ms.verify()
            return True
        except Exception as _:
            return False


def descriptor_parse(input):
    try:
        desc = Descriptor.from_string(input)
        return True
    except Exception as _:
        return False


def psbt_parse(data):
    try:
        psbt_obj = PSBT.parse(data)

        result = []  # format similar to rustbitcoin implementation

        tx = psbt_obj.tx
        # result.append(f"v={tx.version}")
        result.append(f"lt={tx.locktime}")
        result.append(f"in={len(tx.vin)}")
        result.append(f"out={len(tx.vout)}")

        # ip details
        for i, vin in enumerate(tx.vin):
            result.append(f"in{i}prev={vin.txid.hex()}:{vin.vout}")
            result.append(f"in{i}seq={vin.sequence}")

            # check utxo
            psbt_input = psbt_obj.inputs[i]
            has_utxo = (
                hasattr(psbt_input, "witness_utxo")
                and psbt_input.witness_utxo is not None
                or hasattr(psbt_input, "non_witness_utxo")
                and psbt_input.non_witness_utxo is not None
            )
            if has_utxo:
                result.append(f"in{i}utxo=1")

            # count sig
            sig_count = (
                len(psbt_input.partial_sigs)
                if hasattr(psbt_input, "partial_sigs")
                else 0
            )
            result.append(f"in{i}sigs={sig_count}")

            redeem_script_hex = (
                psbt_input.redeem_script.data.hex()
                if psbt_input.redeem_script is not None
                else ""
            )
            result.append(f"in{i}rs={redeem_script_hex}")

            witness_script_hex = (
                psbt_input.witness_script.data.hex()
                if psbt_input.witness_script is not None
                else ""
            )
            result.append(f"in{i}ws={witness_script_hex}")

            # raw PSBT_IN_SIGHASH_TYPE value, or 0 if unset
            sighash = (
                psbt_input.sighash_type if psbt_input.sighash_type is not None else 0
            )
            result.append(f"in{i}sh={sighash}")

            result.append(f"in{i}bip32={len(psbt_input.bip32_derivations)}")

            # Report finalization on *non-empty* final scriptSig/scriptWitness
            # rather than mere presence. Bitcoin Core stores the final witness
            # as a plain CScriptWitness whose IsNull() is just stack.empty(),
            # so it cannot tell an absent PSBT_IN_FINAL_SCRIPTWITNESS key from
            # one present with a zero-item stack; presence-based semantics
            # would make this flag incomparable across modules for that
            # degenerate input. An empty witness finalizes nothing anyway.
            final_scriptsig = psbt_input.final_scriptsig
            final_scriptwitness = psbt_input.final_scriptwitness
            if (final_scriptsig is not None and len(final_scriptsig.data) > 0) or (
                final_scriptwitness is not None and len(final_scriptwitness.items) > 0
            ):
                result.append(f"in{i}fin=1")

        for i, vout in enumerate(tx.vout):
            result.append(f"out{i}val={vout.value}")
            result.append(f"out{i}script={vout.script_pubkey.data.hex()}")

            psbt_output = psbt_obj.outputs[i]

            redeem_script_hex = (
                psbt_output.redeem_script.data.hex()
                if psbt_output.redeem_script is not None
                else ""
            )
            result.append(f"out{i}rs={redeem_script_hex}")

            witness_script_hex = (
                psbt_output.witness_script.data.hex()
                if psbt_output.witness_script is not None
                else ""
            )
            result.append(f"out{i}ws={witness_script_hex}")

            result.append(f"out{i}bip32={len(psbt_output.bip32_derivations)}")

        return ";".join(result) + ";"
    except Exception as _:
        return None


def bip32_master_keygen(data):
    try:
        root = HDKey.from_seed(data, version=NETWORKS["main"]["xprv"])
        return root.to_base58()
    except Exception as _:
        return "INVALID"


def bip32_deserialize_extended_key(data: str) -> str:
    try:
        data = data.decode()
        key: HDKey = HDKey.from_base58(data)
    except Exception:
        return "INVALID"

    depth = f"{key.depth:02x}"

    fp = key.fingerprint.hex().rjust(8, "0")

    child = f"{key.child_number:08x}"

    chaincode = key.chain_code.hex()

    key_bytes = key.key.serialize()
    key_hex = key_bytes.hex()

    result = (
        f"depth={depth};"
        f"fp={fp};"
        f"child={child};"
        f"chaincode={chaincode};"
        f"key={key_hex}"
    )
    return result
