// Tests fixed-array bounds, element identity, and signed index handling.

#include "base/array.h"

#include <cstddef>
#include <span>

#include "gtest/gtest.h"

namespace {

constexpr int kValues[] = {3, 5, 8};
static_assert(base::At(kValues, 1) == 5);

TEST(ArrayTest, ReturnsTheOriginalElement) {
  int values[] = {1, 2, 3};
  base::At(values, 1) = 9;
  EXPECT_EQ(values[1], 9);
  EXPECT_EQ(&base::At(values, std::size_t{2}), &values[2]);
}

TEST(ArrayTest, PreservesNestedArrayExtents) {
  int values[2][3] = {{1, 2, 3}, {4, 5, 6}};
  EXPECT_EQ(base::At(base::At(values, 1), 2), 6);
}

TEST(ArrayTest, SuffixIncludesTheOnePastPosition) {
  int values[] = {1, 2, 3};
  EXPECT_EQ(base::Suffix(values, 1).front(), 2);
  EXPECT_TRUE(base::Suffix(values, 3).empty());
  EXPECT_EQ(base::Suffix(values, 3).data(),
            std::span(values).subspan(3).data());
}

TEST(ArrayDeathTest, RejectsInvalidIndices) {
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH((void)base::At(kValues, -1), "Check failed");
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH((void)base::At(kValues, 3), "Check failed");
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH((void)base::At(kValues, static_cast<std::size_t>(-1)),
               "Check failed");
}

}  // namespace
