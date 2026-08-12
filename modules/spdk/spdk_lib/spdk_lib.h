#include <cstdint>

// Marks a response the caller must drop rather than compare. Spelled the same
// on both sides of the FFI; see spdk_lib's diverges_on_intermediate_zero_sum.
#define SPDK_SKIP "SKIP_INTERMEDIATE_ZERO_SUM"

// Creates the BIP-352 Silent Payments outputs for n_recipients recipients from
// n_inputs input secret keys. input_is_taproot holds one flag per input key,
// non-zero for a taproot input. scan_seckeys and spend_seckeys hold one 32-byte
// key per recipient each. recipient_is_labeled holds one flag per recipient,
// non-zero when that recipient's spend key must be tweaked with the label in
// recipient_labels at the same index. Returns the concatenated 32-byte x-only
// outputs in recipient order as hex, the "INVALID_SECKEY", "CREATE_FAIL" or
// "LABEL_FAIL" rejection sentinel, SPDK_SKIP for the known upstream divergence,
// or null if the arguments themselves are malformed.
extern "C" char *
spdk_create_outputs(const uint8_t *outpoint36, const uint8_t *input_seckeys,
                    const uint8_t *input_is_taproot, size_t n_inputs,
                    const uint8_t *scan_seckeys, const uint8_t *spend_seckeys,
                    const uint8_t *recipient_is_labeled,
                    const uint32_t *recipient_labels, size_t n_recipients);
extern "C" void spdk_free_string(void *ptr);
