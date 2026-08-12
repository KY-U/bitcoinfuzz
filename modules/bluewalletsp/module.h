#include <bitcoinfuzz/basemodule.h>
#include <optional>
#include <string>

namespace bitcoinfuzz {
namespace module {
class BlueWalletSp : public BaseModule {
public:
  BlueWalletSp(void);
  std::optional<std::string> silentpayments_create_outputs(
      const SilentPaymentsCreateOutputsInput &input) const override;
  ~BlueWalletSp() noexcept override = default;
};

} // namespace module
} // namespace bitcoinfuzz
