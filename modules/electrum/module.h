#include <bitcoinfuzz/basemodule.h>
#include <cstdint>
#include <optional>
#include <span>

namespace bitcoinfuzz {
namespace module {
class Electrum : public BaseModule {
public:
  Electrum(void);
  std::optional<std::string> bip32_deserialize_extended_key(
      std::span<const uint8_t> buffer) const override;
  ~Electrum() noexcept override = default;
};

} // namespace module
} // namespace bitcoinfuzz
