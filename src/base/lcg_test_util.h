// File: A deterministic random generator for tests that feed generated inputs
// to two implementations, so every failure is a reproducible case.

#ifndef CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_
#define CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace base {

// A 64-bit linear congruential generator (Knuth's MMIX constants) that hands
// out its high bits. The low bits of such a generator repeat quickly, bit k
// every 2^(k+1) steps, so they are never used.
//
// Example:
//   base::Lcg lcg(7);
//   const auto bytes = lcg.Bytes(lcg.Below(100));
class Lcg {
 public:
  explicit Lcg(const uint32_t seed) : state_(seed) {
    // A few steps, so that small seeds do not start out alike.
    for (int step = 0; step < 4; ++step) {
      Next();
    }
  }

  // Returns 24 random bits.
  uint32_t Next() {
    state_ = (state_ * 6364136223846793005ULL) + 1442695040888963407ULL;
    return static_cast<uint32_t>(state_ >> 40U);
  }

  // Returns a value in [0, bound); `bound` must be positive and at most 2^24.
  int Below(const int bound) {
    return static_cast<int>(Next() % static_cast<uint32_t>(bound));
  }

  uint8_t Byte() { return static_cast<uint8_t>(Next() >> 16U); }

  // Returns `size` random bytes.
  std::vector<std::byte> Bytes(const int size) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ranges::generate(bytes, [this] { return std::byte{Byte()}; });
    return bytes;
  }

 private:
  uint64_t state_;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_
