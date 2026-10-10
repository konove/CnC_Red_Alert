#include "ra/objects/fuse.h"

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

TEST(FuseTest, StartsWithNoImpactPoint) {
  const FuseClass fuse;
  EXPECT_EQ(fuse.ImpactPoint(), Coordinate{});
}

TEST(FuseTest, ArmRecordsTheImpactPoint) {
  FuseClass fuse;
  const Coordinate target = Coordinate::FromLeptons(0, 0x400);
  fuse.Arm(Coordinate{}, target, 20, 0);
  EXPECT_EQ(fuse.ImpactPoint(), target);
}

TEST(FuseTest, DoesNotTripWhileArming) {
  FuseClass fuse;
  const Coordinate target = Coordinate::FromLeptons(0, 0x400);
  fuse.Arm(target, target, 20, 2);
  // On the impact point, but the arming delay has two frames to run.
  EXPECT_FALSE(fuse.Checkup(target));
  EXPECT_FALSE(fuse.Checkup(target));
  EXPECT_TRUE(fuse.Checkup(target));
}

TEST(FuseTest, TripsWhenTheTimerRunsOut) {
  FuseClass fuse;
  const Coordinate far = Coordinate::FromLeptons(0, 0x4000);
  fuse.Arm(Coordinate{}, far, 3, 0);
  EXPECT_FALSE(fuse.Checkup(Coordinate{}));
  EXPECT_FALSE(fuse.Checkup(Coordinate{}));
  EXPECT_TRUE(fuse.Checkup(Coordinate{}));
}

TEST(FuseTest, TripsWhenTheDistanceStartsToGrow) {
  FuseClass fuse;
  const Coordinate target = Coordinate::FromLeptons(0, 0x80);
  fuse.Arm(Coordinate{}, target, 200, 0);
  EXPECT_FALSE(fuse.Checkup(Coordinate::FromLeptons(0, 0x40)));
  EXPECT_TRUE(fuse.Checkup(Coordinate::FromLeptons(0, 0x20)));
}

}  // namespace
}  // namespace ra
