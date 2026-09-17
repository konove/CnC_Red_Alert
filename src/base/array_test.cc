// Tests fixed-array bounds, element identity, and signed index handling.

#include "base/array.h"

#include <cstddef>
#include <span>

#include "gtest/gtest.h"

namespace {

constexpr int kValues[] = {3, 5, 8};
static_assert(base::At(kValues, 1) == 5);
static_assert(base::At(std::span(kValues), 2) == 8);

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
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)base::At(kValues, -1), "Check failed");
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)base::At(kValues, 3), "Check failed");
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)base::At(kValues, static_cast<std::size_t>(-1)),
               "Check failed");
}

TEST(ArrayTest, ConsumeFrontPreservesStorageAndAdvancesExtent) {
  int values[] = {7, 9};
  std::span<int> remaining(values);
  base::ConsumeFront(remaining) = 8;
  EXPECT_EQ(values[0], 8);
  ASSERT_EQ(remaining.size(), 1U);
  EXPECT_EQ(base::ConsumeFront(remaining), 9);
  EXPECT_TRUE(remaining.empty());
}

TEST(ArrayDeathTest, RejectsConsumingAnEmptyView) {
  std::span<int> remaining;
  // GoogleTest formats failure diagnostics inside its death-test macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)base::ConsumeFront(remaining), "Check failed");
}

TEST(ArrayTest, SpanAccessPreservesReferencesExtentsAndSingleEvaluation) {
  int values[] = {2, 4, 6};
  int index = 0;
  base::At(std::span(values), index++) = 7;
  EXPECT_EQ(index, 1);
  EXPECT_EQ(values[0], 7);
  const std::span<const int> view(values);
  EXPECT_EQ(&base::At(view, 2), &values[2]);
  EXPECT_EQ(base::At(view.subspan(1), 0), 4);
}

TEST(ArrayDeathTest, SpanRejectsNegativeEndEmptyAndOversizedIndices) {
  int values[] = {2, 4};
  const std::span<int> view(values);
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)base::At(view, -1), "Check failed");
  EXPECT_DEATH((void)base::At(view, 2), "Check failed");
  EXPECT_DEATH((void)base::At(view.subspan(1, 0), 0), "Check failed");
  EXPECT_DEATH((void)base::At(view, static_cast<std::size_t>(-1)),
               "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

}  // namespace
