#include "base/enum_step.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace {

enum class Theater : int8_t { kNone = -1, kTemperate, kSnow, kInterior };
enum class Plain { kA, kB };

}  // namespace

template <>
inline constexpr bool base::kIsSteppableEnum<Theater> = true;

namespace {

TEST(EnumStepTest, PrefixReturnsTheUpdatedValue) {
  Theater t = Theater::kNone;
  EXPECT_EQ(++t, Theater::kTemperate);
  EXPECT_EQ(++t, Theater::kSnow);
  EXPECT_EQ(--t, Theater::kTemperate);
  EXPECT_EQ(t, Theater::kTemperate);
}

TEST(EnumStepTest, PostfixReturnsThePreviousValue) {
  Theater t = Theater::kSnow;
  EXPECT_EQ(t++, Theater::kSnow);
  EXPECT_EQ(t, Theater::kInterior);
  EXPECT_EQ(t--, Theater::kInterior);
  EXPECT_EQ(t, Theater::kSnow);
}

TEST(EnumStepTest, LoopVisitsEachValueOnce) {
  int visited = 0;
  Theater t = Theater::kNone;
  for (t = Theater::kTemperate; t <= Theater::kSnow; ++t) {
    ++visited;
  }
  EXPECT_EQ(visited, 2);
  EXPECT_EQ(t, Theater::kInterior);
}

TEST(EnumStepTest, OnlyOptedInEnumsQualify) {
  static_assert(base::SteppableEnum<Theater>);
  static_assert(!base::SteppableEnum<Plain>);
  static_assert(!base::SteppableEnum<int>);
}

}  // namespace
