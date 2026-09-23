// Tests for how a trigger action's data reads the values scenario INI files
// store.

#include "ra/taction.h"

#include "gtest/gtest.h"
#include "ra/defines.h"

namespace {

// The shipped scenarios store a special weapon as its value in the low byte
// and 0xFF in the three above it: SCG12EA's "nuke" trigger holds -255, which
// the original game read, through a one-byte enum, as SPC_NUCLEAR_BOMB.
TEST(TActionDataTest, SpecialWeaponReadsTheLowByteOfTheIniValue) {
  TActionClass action;
  action.Data.Value = -255;
  EXPECT_EQ(action.Data.Special, SPC_NUCLEAR_BOMB);
  action.Data.Value = -252;
  EXPECT_EQ(action.Data.Special, SPC_PARA_INFANTRY);
  action.Data.Value = -1;
  EXPECT_EQ(action.Data.Special, SPC_NONE);
}

// SCG10EA's "lch3" trigger plays movie -170, which the original read as 86.
TEST(TActionDataTest, MovieReadsTheLowByteOfTheIniValue) {
  TActionClass action;
  action.Data.Value = -170;
  EXPECT_EQ(action.Data.Movie, static_cast<VQType>(86));
  action.Data.Value = -1;
  EXPECT_EQ(action.Data.Movie, VQ_NONE);
}

}  // namespace
