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
  EXPECT_EQ(ToSize(int64_t{1} << 40), std::size_t{1} << 40);
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

#ifndef NDEBUG
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
