// File: A deterministic random generator for tests that feed generated inputs
// to two implementations, so every failure is a reproducible case.

#ifndef CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_
#define CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace base {

// A linear congruential generator (the Numerical Recipes constants).
//
// Example:
//   base::Lcg lcg(7);
//   const auto bytes = lcg.Bytes(lcg.Below(100));
class Lcg {
 public:
  explicit Lcg(const uint32_t seed) : state_(seed) {}

  // Returns 24 random bits.
  uint32_t Next() {
    state_ = (state_ * 1664525U) + 1013904223U;
    return state_ >> 8;
  }

  // Returns a value in [0, bound); `bound` must be positive.
  int Below(const int bound) {
    return static_cast<int>(Next() % static_cast<uint32_t>(bound));
  }

  uint8_t Byte() { return static_cast<uint8_t>(Next() & 0xffU); }

  // Returns `size` random bytes.
  std::vector<std::byte> Bytes(const int size) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ranges::generate(bytes, [this] { return std::byte{Byte()}; });
    return bytes;
  }

 private:
  uint32_t state_;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_LCG_TEST_UTIL_H_
