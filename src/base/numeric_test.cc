#include "base/numeric.h"

#include <cstddef>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "base/types.h"

namespace base {
namespace {

TEST(ToSizeTest, PreservesNonNegativeValues) {
  EXPECT_EQ(ToSize(0), std::size_t{0});
  EXPECT_EQ(ToSize(42), std::size_t{42});
  EXPECT_EQ(ToSize(int64_t{1} * (1 << 30) * 1024), std::size_t{1} << 40);
  EXPECT_EQ(ToSize(std::numeric_limits<ssize>::max()),
            static_cast<std::size_t>(std::numeric_limits<ssize>::max()));
}

TEST(ToSizeTest, IsUsableInConstantExpressions) {
  static_assert(ToSize(7) == 7U);
  static_assert(ToSize(uint16_t{65535}) == 65535U);
}

TEST(ToSignedTest, PreservesRepresentableValues) {
  EXPECT_EQ(ToSigned(std::size_t{0}), 0);
  EXPECT_EQ(ToSigned(std::size_t{42}), 42);
  EXPECT_EQ(ToSigned(uint32_t{0xFFFFFFFF}), ssize{0xFFFFFFFF});
  static_assert(ToSigned(3U) == 3);
}

enum class Index { kFirst = 0, kSecond = 1, kLast = 31 };
using enum Index;
enum class Flags : uint8_t { kNone = 0, kRead = 1, kWrite = 2 };

TEST(BitTest, SetsTheIndexedBit) {
  EXPECT_EQ(Bit<uint32_t>(0), 1U);
  EXPECT_EQ(Bit<uint32_t>(kSecond), 2U);
  EXPECT_EQ(Bit<uint32_t>(kLast), 0x80000000U);
  EXPECT_EQ(Bit<uint64_t>(63), uint64_t{1} << 63);
  EXPECT_EQ(Bit<uint8_t>(7), uint8_t{0x80});
  static_assert(Bit<uint32_t>(kFirst) == 1U);
  static_assert((Bit<uint64_t>(40) | Bit<uint64_t>(1)) ==
                (uint64_t{1} << 40 | 2U));
}

TEST(AnyTest, IsTrueWhenAnyBitIsSet) {
  static_assert(!Any(Flags::kNone));
  static_assert(Any(Flags::kRead));
  EXPECT_TRUE(Any(static_cast<Flags>(3)));
  EXPECT_FALSE(Any(Flags{}));
}

#ifndef NDEBUG
TEST(BitDeathTest, RejectsIndicesOutsideTheWidth) {
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(Bit<uint32_t>(32), "digits");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(Bit<uint32_t>(-1), "index >= 0");
}

TEST(ToSizeDeathTest, RejectsNegativeValues) {
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(ToSize(-1), "in_range");
}

TEST(ToSignedDeathTest, RejectsValuesAboveSignedRange) {
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(ToSigned(std::numeric_limits<std::size_t>::max()), "in_range");
}
#endif

}  // namespace
}  // namespace base
