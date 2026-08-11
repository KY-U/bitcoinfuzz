#include <bitcoinfuzz/basemodule.h>
#include <optional>
#include <string>

namespace bitcoinfuzz {
namespace module {
class BdkSp : public BaseModule {
public:
  BdkSp(void);
  std::optional<std::string> silentpayments_create_outputs(
      const SilentPaymentsCreateOutputsInput &input) const override;
  ~BdkSp() noexcept override = default;
};
} // namespace module
} // namespace bitcoinfuzz
