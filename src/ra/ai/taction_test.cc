// Tests for how a trigger action's data reads the values scenario INI files
// store, and for carrying the actions out in a real Game with no game data.

#include "ra/ai/taction.h"

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "ra/ai/trigtype.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/startup/game_test_util.h"

namespace ra {

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

// A Game whose only house heap is empty: no house the actions name exists.
class TActionTest : public GameFixture {
 protected:
  // Carries out `action` as a trigger of `house` that is attached to nothing.
  static bool Perform(TActionClass& action,
                      const HousesType house = HOUSE_USSR) {
    return action(house, nullptr, -1, Cell{});
  }
};

// A trigger may belong to a house the scenario does not have.
TEST_F(TActionTest, ActionsOnTheTriggersHouseSkipAMissingHouse) {
  TActionClass action;
  action.Action = TACTION_BASE_BUILDING;
  action.Data.Bool = true;
  EXPECT_TRUE(Perform(action));

  action.Action = TACTION_1_SPECIAL;
  action.Data.Special = SPC_NUCLEAR_BOMB;
  EXPECT_TRUE(Perform(action));

  action.Action = TACTION_FULL_SPECIAL;
  EXPECT_TRUE(Perform(action));
}

TEST_F(TActionTest, ActionsOnTheNamedHouseSkipAMissingHouse) {
  for (const TActionType type : {TACTION_ALL_HUNT, TACTION_BEGIN_PRODUCTION,
                                 TACTION_FIRE_SALE, TACTION_AUTOCREATE}) {
    TActionClass action;
    action.Action = type;
    action.Data.House = HOUSE_GREECE;
    EXPECT_TRUE(Perform(action));
    action.Data.House = HOUSE_NONE;
    EXPECT_TRUE(Perform(action));
  }
}

// Read_INI() leaves Team null when the scenario names a team that does not
// exist.
TEST_F(TActionTest, TeamActionsSkipAMissingTeam) {
  TActionClass action;
  action.Action = TACTION_CREATE_TEAM;
  EXPECT_TRUE(Perform(action));
  action.Action = TACTION_DESTROY_TEAM;
  EXPECT_TRUE(Perform(action));
  action.Action = TACTION_REINFORCEMENTS;
  EXPECT_FALSE(Perform(action));
}

TEST_F(TActionTest, ForceTriggerDoesNothingWhenNoTriggerCanBeMade) {
  TheObjectHeaps().trigger_type().Reset(1);
  TheObjectHeaps().trigger().Reset(0);
  TActionClass action;
  action.Action = TACTION_FORCE_TRIGGER;
  action.Trigger = base::NewPooled<TriggerTypeClass>();
  ASSERT_NE(action.Trigger, nullptr);
  EXPECT_TRUE(Perform(action));
}

TEST_F(TActionTest, TextTriggerSkipsAnIdWithNoText) {
  TActionClass action;
  action.Action = TACTION_TEXT_TRIGGER;
  action.Data.Value = -1;
  EXPECT_TRUE(Perform(action));
  action.Data.Value = 0;
  EXPECT_TRUE(Perform(action));
}

}  // namespace

}  // namespace ra
