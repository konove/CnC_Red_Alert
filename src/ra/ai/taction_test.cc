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
TEST(TriggerActionDataTest, SpecialWeaponReadsTheLowByteOfTheIniValue) {
  TriggerAction action;
  action.data.value = -255;
  EXPECT_EQ(action.data.special, SPC_NUCLEAR_BOMB);
  action.data.value = -252;
  EXPECT_EQ(action.data.special, SPC_PARA_INFANTRY);
  action.data.value = -1;
  EXPECT_EQ(action.data.special, SPC_NONE);
}

// SCG10EA's "lch3" trigger plays movie -170, which the original read as 86.
TEST(TriggerActionDataTest, MovieReadsTheLowByteOfTheIniValue) {
  TriggerAction action;
  action.data.value = -170;
  EXPECT_EQ(action.data.movie, static_cast<VQType>(86));
  action.data.value = -1;
  EXPECT_EQ(action.data.movie, VQ_NONE);
}

// A Game whose only house heap is empty: no house the actions name exists.
class TriggerActionTest : public GameFixture {
 protected:
  // Carries out `action` as a trigger of `house` that is attached to nothing.
  static bool Perform(TriggerAction& action,
                      const HousesType house = HOUSE_USSR) {
    return action(house, nullptr, -1, Cell{});
  }
};

// A trigger may belong to a house the scenario does not have.
TEST_F(TriggerActionTest, ActionsOnTheTriggersHouseSkipAMissingHouse) {
  TriggerAction action;
  action.type = TACTION_BASE_BUILDING;
  action.data.flag = true;
  EXPECT_TRUE(Perform(action));

  action.type = TACTION_1_SPECIAL;
  action.data.special = SPC_NUCLEAR_BOMB;
  EXPECT_TRUE(Perform(action));

  action.type = TACTION_FULL_SPECIAL;
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionTest, ActionsOnTheNamedHouseSkipAMissingHouse) {
  for (const TriggerActionType type :
       {TACTION_ALL_HUNT, TACTION_BEGIN_PRODUCTION, TACTION_FIRE_SALE,
        TACTION_AUTOCREATE}) {
    TriggerAction action;
    action.type = type;
    action.data.house = HOUSE_GREECE;
    EXPECT_TRUE(Perform(action));
    action.data.house = HOUSE_NONE;
    EXPECT_TRUE(Perform(action));
  }
}

// Read_INI() leaves Team null when the scenario names a team that does not
// exist.
TEST_F(TriggerActionTest, TeamActionsSkipAMissingTeam) {
  TriggerAction action;
  action.type = TACTION_CREATE_TEAM;
  EXPECT_TRUE(Perform(action));
  action.type = TACTION_DESTROY_TEAM;
  EXPECT_TRUE(Perform(action));
  action.type = TACTION_REINFORCEMENTS;
  EXPECT_FALSE(Perform(action));
}

TEST_F(TriggerActionTest, ForceTriggerDoesNothingWhenNoTriggerCanBeMade) {
  TheObjectHeaps().trigger_type().Reset(1);
  TheObjectHeaps().trigger().Reset(0);
  TriggerAction action;
  action.type = TACTION_FORCE_TRIGGER;
  action.trigger_type = base::NewPooled<TriggerTypeClass>();
  ASSERT_NE(action.trigger_type, nullptr);
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionTest, TextTriggerSkipsAnIdWithNoText) {
  TriggerAction action;
  action.type = TACTION_TEXT_TRIGGER;
  action.data.value = -1;
  EXPECT_TRUE(Perform(action));
  action.data.value = 0;
  EXPECT_TRUE(Perform(action));
}

}  // namespace

}  // namespace ra
