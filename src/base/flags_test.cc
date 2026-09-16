#include "base/flags.h"

#include <cstdint>

#include <gtest/gtest.h>

#include "base/attributes.h"
#include "base/numeric.h"

namespace {

enum class CNC_FLAG_ENUM Effect : uint32_t { kNone = 0, kGhost = 1, kFading = 2, kPredator = 4 };
enum class Plain { kA = 1, kB = 2 };

}  // namespace

template <>
inline constexpr bool base::kIsFlagEnum<Effect> = true;

namespace {

TEST(FlagsTest, CombinesAndMasks) {
  constexpr Effect both = Effect::kGhost | Effect::kFading;
  static_assert(static_cast<uint32_t>(both) == 3);
  static_assert((both & Effect::kFading) == Effect::kFading);
  static_assert((both & Effect::kPredator) == Effect::kNone);
  static_assert((both ^ Effect::kGhost) == Effect::kFading);
  static_assert((~Effect::kNone & Effect::kPredator) == Effect::kPredator);
}

TEST(FlagsTest, CompoundAssignmentUpdatesInPlace) {
  Effect flags = Effect::kNone;
  flags |= Effect::kGhost;
  flags |= Effect::kPredator;
  EXPECT_TRUE(base::Any(flags & Effect::kGhost));
  flags &= ~Effect::kGhost;
  EXPECT_FALSE(base::Any(flags & Effect::kGhost));
  EXPECT_EQ(flags, Effect::kPredator);
}

TEST(FlagsTest, OnlyOptedInEnumsQualify) {
  static_assert(base::FlagEnum<Effect>);
  static_assert(!base::FlagEnum<Plain>);
  static_assert(!base::FlagEnum<int>);
}

}  // namespace
