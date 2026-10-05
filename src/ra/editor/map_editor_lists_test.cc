// Tests for the scenario editor's trigger and team lists
// (MapEditClass::Handle_Triggers() and Handle_Teams()), played by a script
// on a headless screen: that the lists are in name order, that the entry
// clicked is the one acted on, and what Edit, New, Delete and OK do.

#include "base/pooled.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"

namespace ra {
namespace {

// Both list dialogs are 400 by 250 in the screen's top left corner, with the
// same row of buttons along the bottom. A list row is 9 pixels tall with the
// fixture's font.
constexpr int kEditX = 155 + 5;
constexpr int kNewX = 210 + 5;
constexpr int kDeleteX = 265 + 5;
constexpr int kOkX = 320 + 5;
constexpr int kButtonsY = 221 + 4;
constexpr int kRowHeight = 9;

// Three trigger types made out of name order, in a heap with room for one
// more.
class TriggerListTest : public UiFixture {
 protected:
  TriggerListTest() {
    // Closing a list redraws the map, radar and all, for the player's house.
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    // The trigger dialog that Edit and New open lists every kind of object.
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    AircraftTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    for (const char* const name : {"zulu", "Alfa", "mike"}) {
      base::NewPooled<TriggerType>()->Set_Name(name);
    }
  }

  // The middle of row `row` of the list, which starts at 40, 25.
  static constexpr int kRowX = 40 + 20;
  static constexpr int RowY(const int row) {
    return 25 + (kRowHeight * row) + 5;
  }

  static bool Exists(const char* name) {
    return TriggerType::FromName(name) != nullptr;
  }
};

TEST_F(TriggerListTest, OkClosesItAndChangesNothing) {
  player().At(3).Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 3);
}

TEST_F(TriggerListTest, ReturnClosesItToo) {
  player().At(3).Key(engine::window::KN_RETURN);

  TheMap().Handle_Triggers();

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 3);
}

// The list is in name order whatever the case, and opens with its first
// entry selected.
TEST_F(TriggerListTest, DeleteRemovesTheFirstInNameOrderWhenNoneIsPicked) {
  player().At(3).Click(kDeleteX, kButtonsY).At(10).Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_FALSE(Exists("Alfa"));
  EXPECT_TRUE(Exists("mike"));
  EXPECT_TRUE(Exists("zulu"));
}

TEST_F(TriggerListTest, DeleteRemovesTheEntryClicked) {
  player()
      .At(3)
      .Click(kRowX, RowY(1))
      .At(8)
      .Click(kDeleteX, kButtonsY)
      .At(15)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_TRUE(Exists("Alfa"));
  EXPECT_FALSE(Exists("mike"));
  EXPECT_TRUE(Exists("zulu"));
}

TEST_F(TriggerListTest, TheLastRowIsTheLastInNameOrder) {
  player()
      .At(3)
      .Click(kRowX, RowY(2))
      .At(8)
      .Click(kDeleteX, kButtonsY)
      .At(15)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_FALSE(Exists("zulu"));
  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 2);
}

// After a delete the list comes back with its new first entry selected.
TEST_F(TriggerListTest, DeletingTwiceRemovesTheFirstTwo) {
  player()
      .At(3)
      .Click(kDeleteX, kButtonsY)
      .At(10)
      .Click(kDeleteX, kButtonsY)
      .At(17)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_FALSE(Exists("Alfa"));
  EXPECT_FALSE(Exists("mike"));
  EXPECT_TRUE(Exists("zulu"));
}

// Edit opens the trigger dialog on the entry clicked; the list keeps that
// entry selected when it comes back.
TEST_F(TriggerListTest, EditOpensTheDialogOnTheEntryClicked) {
  TriggerType::FromName("mike")->action_control = MULTI_ONLY;
  // The trigger dialog's second-action button, under its first action list.
  constexpr int kActionControlX = 45 + 5;
  constexpr int kActionControlY = 120 + 11 + 3;
  player()
      .At(3)
      .Click(kRowX, RowY(1))
      .At(8)
      .Click(kEditX, kButtonsY)
      .At(15)
      .Click(kActionControlX, kActionControlY)
      .At(20)
      .Key(engine::window::KN_RETURN)
      .At(27)
      .Click(kDeleteX, kButtonsY)
      .At(34)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_TRUE(player().finished());
  // The trigger edited was the one clicked, and the one then deleted.
  EXPECT_FALSE(Exists("mike"));
  EXPECT_EQ(TriggerType::FromName("Alfa")->action_control, MULTI_ONLY);
  EXPECT_EQ(TriggerType::FromName("zulu")->action_control, MULTI_ONLY);
}

TEST_F(TriggerListTest, NewAddsTheTriggerTheDialogAccepts) {
  player()
      .At(3)
      .Click(kNewX, kButtonsY)
      .At(10)
      .Key(engine::window::KN_RETURN)
      .At(17)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 4);
}

TEST_F(TriggerListTest, NewAddsNothingWhenTheDialogIsCancelled) {
  player()
      .At(3)
      .Click(kNewX, kButtonsY)
      .At(10)
      .Key(engine::window::KN_ESC)
      .At(17)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Triggers();

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 3);
}

// Three team types made out of name order.
class TeamListTest : public UiFixture {
 protected:
  TeamListTest() {
    // Closing a list redraws the map, radar and all, for the player's house.
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    for (const char* const name : {"zulu", "Alfa", "mike"}) {
      auto* team_type = base::NewPooled<TeamTypeClass>();
      team_type->Set_Name(name);
      // The list shows each team's house; the team dialog always sets one.
      team_type->House = HOUSE_GREECE;
    }
  }

  // The middle of row `row` of the list, which starts at 35, 20.
  static constexpr int kRowX = 35 + 20;
  static constexpr int RowY(const int row) {
    return 20 + (kRowHeight * row) + 5;
  }

  static bool Exists(const char* name) {
    return TeamTypeClass::From_Name(name) != nullptr;
  }
};

TEST_F(TeamListTest, OkClosesItAndChangesNothing) {
  player().At(3).Click(kOkX, kButtonsY);

  TheMap().Handle_Teams("Teams");

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 3);
}

TEST_F(TeamListTest, DeleteRemovesTheFirstInNameOrderWhenNoneIsPicked) {
  player().At(3).Click(kDeleteX, kButtonsY).At(10).Click(kOkX, kButtonsY);

  TheMap().Handle_Teams("Teams");

  EXPECT_FALSE(Exists("Alfa"));
  EXPECT_TRUE(Exists("mike"));
  EXPECT_TRUE(Exists("zulu"));
}

TEST_F(TeamListTest, DeleteRemovesTheEntryClicked) {
  player()
      .At(3)
      .Click(kRowX, RowY(2))
      .At(8)
      .Click(kDeleteX, kButtonsY)
      .At(15)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Teams("Teams");

  EXPECT_TRUE(Exists("Alfa"));
  EXPECT_TRUE(Exists("mike"));
  EXPECT_FALSE(Exists("zulu"));
}

TEST_F(TeamListTest, DeletingTwiceRemovesTheFirstTwo) {
  player()
      .At(3)
      .Click(kDeleteX, kButtonsY)
      .At(10)
      .Click(kDeleteX, kButtonsY)
      .At(17)
      .Click(kOkX, kButtonsY);

  TheMap().Handle_Teams("Teams");

  EXPECT_FALSE(Exists("Alfa"));
  EXPECT_FALSE(Exists("mike"));
  EXPECT_TRUE(Exists("zulu"));
}

}  // namespace
}  // namespace ra
