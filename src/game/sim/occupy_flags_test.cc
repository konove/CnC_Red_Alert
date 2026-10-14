// Tests for game::OccupyFlags: the bit each occupant takes, which the saved
// games store, and the infantry-spot queries.

#include "game/sim/occupy_flags.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(OccupyFlagsTest, EachOccupantIsItsOwnBitFromTheLowEnd) {
  // The order of the bit-fields this replaced, which the saves store.
  EXPECT_EQ(OccupyFlags().With(Occupant::kCenter).bits(), 0x01);
  EXPECT_EQ(OccupyFlags().With(Occupant::kSouthEast).bits(), 0x10);
  EXPECT_EQ(OccupyFlags().With(Occupant::kVehicle).bits(), 0x20);
  EXPECT_EQ(OccupyFlags().With(Occupant::kBuilding).bits(), 0x80);
}

TEST(OccupyFlagsTest, AnySpotSeesOnlyTheInfantrySpots) {
  EXPECT_FALSE(OccupyFlags::FromBits(0xE0).AnySpot());
  EXPECT_TRUE(OccupyFlags::FromBits(0x04).AnySpot());
}

TEST(OccupyFlagsTest, AllSpotsNeedsEveryInfantrySpot) {
  EXPECT_FALSE(OccupyFlags::FromBits(0xEF).AllSpots());
  EXPECT_TRUE(OccupyFlags::FromBits(0x1F).AllSpots());
}

TEST(OccupyFlagsTest, SetFalseClearsOnlyThatBit) {
  OccupyFlags flags = OccupyFlags::FromBits(0xFF);
  flags.Set(Occupant::kMonolith, false);
  EXPECT_EQ(flags.bits(), 0xBF);
  EXPECT_FALSE(flags.Has(Occupant::kMonolith));
  EXPECT_TRUE(flags.Has(Occupant::kBuilding));
}

TEST(OccupyFlagsTest, AnyIsFalseOnlyWhenEmpty) {
  EXPECT_FALSE(OccupyFlags().any());
  EXPECT_TRUE(OccupyFlags().With(Occupant::kBuilding).any());
}

}  // namespace
}  // namespace game
