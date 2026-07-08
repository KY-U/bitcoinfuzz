#ifndef BITCOINFUZZ_UTIL_H
#define BITCOINFUZZ_UTIL_H

#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include <vector>

namespace bitcoinfuzz {

/**
 * Returns a byte vector of specified size regardless of the number of
 * remaining bytes available from the fuzzer. Pads with zero value bytes if
 * needed to achieve the specified size.
 */
template <typename B = uint8_t>
[[nodiscard]] inline std::vector<B>
ConsumeFixedLengthByteVector(FuzzedDataProvider &fuzzed_data_provider,
                             const size_t length) noexcept {
  static_assert(sizeof(B) == 1);
  auto random_bytes = fuzzed_data_provider.ConsumeBytes<B>(length);
  random_bytes.resize(length);
  return random_bytes;
}

} // namespace bitcoinfuzz

#endif // BITCOINFUZZ_UTIL_H
