#include <cstddef>
#include <cstdint>

extern "C" bool tiny_miniscript_descriptor_parse(const char *input);
extern "C" bool tiny_miniscript_miniscript_parse(const uint8_t *input,
                                                 size_t len);
