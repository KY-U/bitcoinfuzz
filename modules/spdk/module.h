#include <bitcoinfuzz/basemodule.h>
#include <optional>
#include <string>

namespace bitcoinfuzz {
namespace module {
class Spdk : public BaseModule {
public:
  Spdk(void);
  std::optional<std::string> silentpayments_create_outputs(
      const SilentPaymentsCreateOutputsInput &input) const override;
  ~Spdk() noexcept override = default;
};
} // namespace module
} // namespace bitcoinfuzz
