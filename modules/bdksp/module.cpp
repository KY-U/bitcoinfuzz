#include "module.h"
#include "sp_lib/sp_lib.h"

namespace bitcoinfuzz {
namespace module {

BdkSp::BdkSp(void) : BaseModule("BdkSp") {}

std::optional<std::string> BdkSp::silentpayments_create_outputs(
    const SilentPaymentsCreateOutputsInput &input) const {
  const size_t num_inputs = input.input_seckeys.size() / 32;
  const size_t num_recipients = input.scan_seckeys.size() / 32;
  if (num_inputs == 0 || num_recipients == 0 ||
      input.input_seckeys.size() != num_inputs * 32 ||
      input.input_is_taproot.size() != num_inputs ||
      input.scan_seckeys.size() != num_recipients * 32 ||
      input.spend_seckeys.size() != num_recipients * 32) {
    return std::nullopt;
  }

  // Returns the recipient ordered outputs as hex, or an "INVALID_SECKEY" /
  // "CREATE_FAIL" rejection sentinel, matching the secp256k1 module. Null means
  // the arguments were malformed, which the checks above already rule out.
  char *result = ::bdk_sp_create_outputs(
      input.outpoint_smallest.data(), input.input_seckeys.data(),
      input.input_is_taproot.data(), num_inputs, input.scan_seckeys.data(),
      input.spend_seckeys.data(), num_recipients);
  if (!result)
    return std::nullopt;

  std::string s(result);
  ::bdk_sp_free_string(result);

  // TODO: remove together with the Rust side's
  // diverges_on_intermediate_zero_sum once bdk_sp stops rejecting a zero
  // intermediate input key sum. Upstream already knows about it, so comparing
  // it would only stop the target from reaching anything else.
  if (s == BDK_SP_SKIP)
    return std::nullopt;

  return s;
}

} // namespace module
} // namespace bitcoinfuzz
