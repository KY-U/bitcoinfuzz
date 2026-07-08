#include <cstddef>
#include <cstdint>

extern "C" char *rustcrypto_aes256_cbc(const uint8_t *key, const uint8_t *iv,
                                       bool pad, const uint8_t *data,
                                       size_t len);
extern "C" void rustcrypto_aes_free_string(char *ptr);
