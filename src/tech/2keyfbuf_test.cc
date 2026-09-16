#include "tech/2keyfbuf.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "sdllib/shape.h"

namespace {

// Buffer_Frame_To_Page caches one line-blit flag per scan line of a shape and
// keys that cache on the drawing effects the shape was prepared for. The key
// is written by Setup_Shape_Header and read back on the next draw, so these
// tests pin the property both sides depend on: the same flags always produce
// the same key, and only the four effect bits take part in it.
//
// The original code spelled the mask out twice and got the precedence wrong on
// the reading side, writing `flags & (TRANS | FADING | PREDATOR | GHOST)` but
// comparing against `(flags & TRANS) | FADING | PREDATOR | GHOST`. That made
// the stored key differ from the computed one for every flag combination that
// did not request all four effects, so the cache never hit and every draw
// rebuilt the header.

TEST(ShapeEffectFlagsTest, KeepsOnlyTheFourEffectBits) {
  EXPECT_EQ(ShapeEffectFlags(SHAPE_TRANS), SHAPE_TRANS);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_FADING), SHAPE_FADING);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_PREDATOR), SHAPE_PREDATOR);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_GHOST), SHAPE_GHOST);
}

TEST(ShapeEffectFlagsTest, DropsFlagsTheLineCacheDoesNotDependOn) {
  EXPECT_EQ(ShapeEffectFlags(SHAPE_NORMAL), SHAPE_NORMAL);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_HORZ_REV | SHAPE_VERT_REV | SHAPE_SCALING |
                             SHAPE_WIN_REL | SHAPE_CENTER | SHAPE_PRIORITY |
                             SHAPE_SHADOW | SHAPE_PARTIAL | SHAPE_COLOR),
            SHAPE_NORMAL);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_CENTER | SHAPE_GHOST | SHAPE_COLOR),
            SHAPE_GHOST);
}

TEST(ShapeEffectFlagsTest, CombinesTheRequestedEffects) {
  EXPECT_EQ(ShapeEffectFlags(SHAPE_TRANS | SHAPE_FADING),
            SHAPE_TRANS | SHAPE_FADING);
  EXPECT_EQ(ShapeEffectFlags(SHAPE_TRANS | SHAPE_FADING | SHAPE_PREDATOR |
                             SHAPE_GHOST),
            SHAPE_TRANS | SHAPE_FADING | SHAPE_PREDATOR | SHAPE_GHOST);
}

// The regression itself: a shape drawn twice with the same flags must produce
// a matching key, so the second draw can reuse the cached line flags. Under
// the old reading expression this held only when all four effects were asked
// for.
TEST(ShapeEffectFlagsTest, TheSameFlagsAlwaysAgreeWithTheStoredKey) {
  constexpr ShapeFlags_Type kAllEffects =
      SHAPE_TRANS | SHAPE_FADING | SHAPE_PREDATOR | SHAPE_GHOST;
  for (uint32_t bits = 0; bits <= 0xFFFF; bits++) {
    const auto flags = static_cast<ShapeFlags_Type>(bits);
    // Only the effects this shape actually asked for may appear in the key.
    // The broken reading expression added the other three unconditionally.
    EXPECT_EQ(ShapeEffectFlags(flags), flags & kAllEffects) << "flags " << bits;
  }
}

// Distinct effect requests must not collide, or a shape prepared for one
// effect would be reused for another.
TEST(ShapeEffectFlagsTest, DifferentEffectsProduceDifferentKeys) {
  EXPECT_NE(ShapeEffectFlags(SHAPE_TRANS), ShapeEffectFlags(SHAPE_GHOST));
  EXPECT_NE(ShapeEffectFlags(SHAPE_TRANS),
            ShapeEffectFlags(SHAPE_TRANS | SHAPE_FADING));
  EXPECT_NE(ShapeEffectFlags(SHAPE_CENTER | SHAPE_TRANS),
            ShapeEffectFlags(SHAPE_CENTER | SHAPE_PREDATOR));
}

}  // namespace
