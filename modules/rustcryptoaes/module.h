#include <bitcoinfuzz/basemodule.h>
#include <optional>
#include <span>

namespace bitcoinfuzz {
namespace module {
class RustCryptoAes : public BaseModule {
public:
  RustCryptoAes(void);
  std::optional<std::string>
  aes256_cbc(std::span<const uint8_t> key, std::span<const uint8_t> iv,
             bool pad, std::span<const uint8_t> data) const override;
  ~RustCryptoAes() noexcept override = default;
};
} // namespace module
} // namespace bitcoinfuzz
