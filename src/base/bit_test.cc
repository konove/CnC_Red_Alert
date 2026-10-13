#include "base/bit.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace base {
namespace {

enum class Index { kFirst = 0, kSecond = 1, kLast = 31 };

TEST(BitTest, SetsTheIndexedBit) {
  EXPECT_EQ(Bit<uint32_t>(0), 1U);
  EXPECT_EQ(Bit<uint32_t>(Index::kSecond), 2U);
  EXPECT_EQ(Bit<uint32_t>(Index::kLast), 0x80000000U);
  EXPECT_EQ(Bit<uint64_t>(63), uint64_t{1} << 63);
  EXPECT_EQ(Bit<uint8_t>(7), uint8_t{0x80});
  static_assert(Bit<uint32_t>(Index::kFirst) == 1U);
  static_assert((Bit<uint64_t>(40) | Bit<uint64_t>(1)) ==
                (uint64_t{1} << 40 | 2U));
}

#ifndef NDEBUG
TEST(BitDeathTest, RejectsIndicesOutsideTheWidth) {
  // The switch and formatted failure diagnostic are inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(Bit<uint32_t>(32), "digits");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(Bit<uint32_t>(-1), "index >= 0");
}
#endif

}  // namespace
}  // namespace base
