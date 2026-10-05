// Tests for the scenario editor's trigger dialog, TriggerType::Edit(), played
// by a script on a headless screen.

#include <string>

#include "base/pooled.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "game/ui/input.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/teamtype.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/ai/trigger_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"

namespace ra {
namespace {

// A Game with every type the dialog lists, one team type and one trigger
// type to edit.
class TriggerTypeDialogTest : public UiFixture {
 protected:
  void SetUp() override {
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    AircraftTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    team_type_ = base::NewPooled<TeamTypeClass>();
    team_type_->Set_Name("tanks");
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alrm");
  }

  TeamTypeClass* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

TEST_F(TriggerTypeDialogTest, ReturnAcceptsIt) {
  player().At(3).Key(engine::window::KN_RETURN);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_TRUE(player().finished());
}

TEST_F(TriggerTypeDialogTest, EscCancelsIt) {
  player().At(3).Key(engine::window::KN_ESC);

  EXPECT_FALSE(trigger_type_->Edit());
}

// Where the dialog, which sits in the screen's top left corner, puts the
// gadgets the tests click, a few pixels inside each.
constexpr int kNameEditX = 40 + 3;
constexpr int kNameEditY = 30 + 3;
constexpr int kEventControlX = 45 + 5;
constexpr int kEventControlY = 65 + 11 + 3;
constexpr int kActionControlX = 45 + 5;
constexpr int kActionControlY = 120 + 11 + 3;
constexpr int kOkX = 35 + 5;
constexpr int kCancelX = 420 - 80 + 5;
constexpr int kButtonsY = 220 - 30 + 3;

TEST_F(TriggerTypeDialogTest, TheOkButtonAcceptsIt) {
  player().At(3).Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
}

TEST_F(TriggerTypeDialogTest, TheCancelButtonLeavesTheTriggerAsItWas) {
  trigger_type_->event_control = MULTI_ONLY;
  player()
      .At(3)
      .Click(kEventControlX, kEventControlY)
      .At(8)
      .Click(kCancelX, kButtonsY);

  EXPECT_FALSE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->event_control, MULTI_ONLY);
}

// The button under the first event goes round the four ways two events can
// combine.
TEST_F(TriggerTypeDialogTest, TheEventButtonCyclesThroughTheEventStyles) {
  const MultiStyle expected[] = {MULTI_AND, MULTI_OR, MULTI_LINKED, MULTI_ONLY};
  int clicks = 0;
  for (const MultiStyle style : expected) {
    ++clicks;
    SCOPED_TRACE(clicks);
    trigger_type_->event_control = MULTI_ONLY;
    engine::window::ScriptedPlayer clicking(engine::window::TheDisplay(),
                                            game::TheKeyboard(),
                                            game::TheMouse(), clock());
    for (int click = 0; click < clicks; ++click) {
      clicking.At(3 + (click * 5)).Click(kEventControlX, kEventControlY);
    }
    clicking.At(3 + (clicks * 5)).Key(engine::window::KN_RETURN);

    EXPECT_TRUE(trigger_type_->Edit());
    EXPECT_EQ(trigger_type_->event_control, style);
  }
}

TEST_F(TriggerTypeDialogTest, TheActionButtonTogglesTheSecondAction) {
  trigger_type_->action_control = MULTI_ONLY;
  player()
      .At(3)
      .Click(kActionControlX, kActionControlY)
      .At(8)
      .Key(engine::window::KN_RETURN);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->action_control, MULTI_AND);
}

TEST_F(TriggerTypeDialogTest, TheNameCanBeTypedOver) {
  player()
      .At(3)
      .Click(kNameEditX, kNameEditY)
      .At(8)
      .Key(engine::window::KN_BACKSPACE)
      .At(10)
      .Key(engine::window::KN_BACKSPACE)
      .At(12)
      .Type("xy")
      .At(18)
      .Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_STRCASEEQ(trigger_type_->Name(), "alxy");
}

// A trigger must have a name to be found by, so an empty one is replaced.
TEST_F(TriggerTypeDialogTest, AnEmptyNameBecomesUnderscores) {
  trigger_type_->Set_Name("");
  player().At(3).Key(engine::window::KN_RETURN);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_STREQ(trigger_type_->Name(), "____");
}

// The owner list cannot show "no house", so the dialog gives the trigger one
// as it opens, and a cancel does not take it back.
TEST_F(TriggerTypeDialogTest, ATriggerWithoutAHouseGetsOneEvenWhenCancelled) {
  trigger_type_->house = HOUSE_NONE;
  player().At(3).Key(engine::window::KN_ESC);

  EXPECT_FALSE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->house, HOUSE_GOOD);
}

TEST_F(TriggerTypeDialogTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(3).Click(kEventControlX, kEventControlY);

  EXPECT_THROW(trigger_type_->Edit(), engine::window::ScriptOverrun);
}

// What a trigger type holds, in a form that can be compared and printed.
struct Contents {
  std::string name;
  HousesType house;
  TriggerType::Persistence persistence;
  MultiStyle event_control;
  MultiStyle action_control;
  TEventType event1;
  TEventType event2;
  const TeamTypeClass* event1_team;
  const TeamTypeClass* event2_team;
  int event1_value;
  int event2_value;
  TriggerActionRecord action1;
  TriggerActionRecord action2;
};

// The value of the event's data that its kind of event uses, 0 for an event
// that takes none.
int EventValue(const TEventClass& event) {
  const NeedType need = Event_Needs(event.Event);
  if (need == NEED_HOUSE) {
    return static_cast<int>(event.Data.House);
  }
  if (need == NEED_STRUCTURE) {
    return static_cast<int>(event.Data.Structure);
  }
  if (need == NEED_INFANTRY) {
    return static_cast<int>(event.Data.Infantry);
  }
  if (need == NEED_AIRCRAFT) {
    return static_cast<int>(event.Data.Aircraft);
  }
  if (need == NEED_UNIT) {
    return static_cast<int>(event.Data.Unit);
  }
  if (need == NEED_WAYPOINT || need == NEED_TIME || need == NEED_NUMBER) {
    return event.Data.Value;
  }
  return 0;
}

Contents ContentsOf(const TriggerType& type) {
  return {.name = type.Name(),
          .house = type.house,
          .persistence = type.persistence,
          .event_control = type.event_control,
          .action_control = type.action_control,
          .event1 = type.event1.Event,
          .event2 = type.event2.Event,
          .event1_team = type.event1.Team,
          .event2_team = type.event2.Team,
          .event1_value = EventValue(type.event1),
          .event2_value = EventValue(type.event2),
          .action1 = type.action1->ToRecord(),
          .action2 = type.action2->ToRecord()};
}

// A trigger opened in the dialog and accepted with Return must come out as
// it went in, whatever its events and actions are: every kind of event and
// action has its own field in the dialog, filled when it opens and read back
// on OK.
class TriggerTypeDialogRoundTripTest : public TriggerTypeDialogTest {
 protected:
  void SetUp() override {
    TriggerTypeDialogTest::SetUp();
    other_trigger_type_ = base::NewPooled<TriggerType>();
    other_trigger_type_->Set_Name("othr");
    trigger_type_->house = HOUSE_USSR;
    trigger_type_->persistence = TriggerType::Persistence::PERSISTANT;
    trigger_type_->event_control = MULTI_AND;
    trigger_type_->action_control = MULTI_AND;
  }

  // Gives `event` the kind `type` and a value of the sort that kind takes.
  void Fill(TEventClass& event, const TEventType type) const {
    event.Event = type;
    const NeedType need = Event_Needs(type);
    if (need == NEED_HOUSE) {
      event.Data.House = HOUSE_UKRAINE;
    } else if (need == NEED_TEAM) {
      event.Team = team_type_;
    } else if (need == NEED_STRUCTURE) {
      event.Data.Structure = STRUCT_POWER;
    } else if (need == NEED_INFANTRY) {
      event.Data.Infantry = INFANTRY_E2;
    } else if (need == NEED_AIRCRAFT) {
      event.Data.Aircraft = AIRCRAFT_HIND;
    } else if (need == NEED_UNIT) {
      event.Data.Unit = UNIT_JEEP;
    } else if (need == NEED_WAYPOINT) {
      event.Data.Value = 5;
    } else if (need == NEED_TIME || need == NEED_NUMBER) {
      event.Data.Value = 7;
    }
  }

  // An action of the kind `type` with a value of the sort that kind takes.
  [[nodiscard]] TriggerActionRecord Record(const TriggerActionType type) const {
    TriggerActionRecord record{.type = type};
    const NeedType need = ActionNeed(type);
    if (need == NEED_TEAM) {
      record.team_type = team_type_;
    } else if (need == NEED_TRIGGER) {
      record.trigger_type = other_trigger_type_;
    } else if (need == NEED_HOUSE) {
      record.value = static_cast<int>(HOUSE_UKRAINE);
    } else if (need == NEED_WAYPOINT) {
      record.value = 5;
    } else if (need == NEED_NUMBER) {
      record.value = 7;
    } else if (need != NEED_NONE) {
      // A special weapon, movie, flag, theme, sound, speech or quarry: an
      // index into its list.
      record.value = 1;
    }
    return record;
  }

  // Opens the dialog on the trigger type and accepts it. Returns what the
  // trigger type holds afterwards.
  Contents EditAndAccept() {
    engine::window::ScriptedPlayer accepting(engine::window::TheDisplay(),
                                             game::TheKeyboard(),
                                             game::TheMouse(), clock());
    accepting.At(3).Key(engine::window::KN_RETURN);
    EXPECT_TRUE(trigger_type_->Edit());
    return ContentsOf(*trigger_type_);
  }

  TriggerType* other_trigger_type_ = nullptr;
};

TEST_F(TriggerTypeDialogRoundTripTest, EveryKindOfEventComesBackAsItWas) {
  for (const TEventType type : magic_enum::enum_values<TEventType>()) {
    // The enum's end marker is no event.
    if (type == TEVENT_COUNT) {
      continue;
    }
    SCOPED_TRACE(magic_enum::enum_name(type));
    Fill(trigger_type_->event1, type);
    Fill(trigger_type_->event2, type);
    const Contents before = ContentsOf(*trigger_type_);

    const Contents after = EditAndAccept();

    EXPECT_EQ(after.event1, before.event1);
    EXPECT_EQ(after.event2, before.event2);
    EXPECT_EQ(after.event1_team, before.event1_team);
    EXPECT_EQ(after.event2_team, before.event2_team);
    EXPECT_EQ(after.event1_value, before.event1_value);
    EXPECT_EQ(after.event2_value, before.event2_value);
  }
}

TEST_F(TriggerTypeDialogRoundTripTest, EveryKindOfActionComesBackAsItWas) {
  for (const TriggerActionType type :
       magic_enum::enum_values<TriggerActionType>()) {
    SCOPED_TRACE(magic_enum::enum_name(type));
    trigger_type_->action1 = MakeTriggerAction(Record(type));
    trigger_type_->action2 = MakeTriggerAction(Record(type));
    const Contents before = ContentsOf(*trigger_type_);

    const Contents after = EditAndAccept();

    EXPECT_EQ(after.action1, before.action1);
    EXPECT_EQ(after.action2, before.action2);
  }
}

TEST_F(TriggerTypeDialogRoundTripTest, TheRestComesBackAsItWas) {
  const Contents before = ContentsOf(*trigger_type_);

  const Contents after = EditAndAccept();

  EXPECT_EQ(after.name, before.name);
  EXPECT_EQ(after.house, before.house);
  EXPECT_EQ(after.persistence, before.persistence);
  EXPECT_EQ(after.event_control, before.event_control);
  EXPECT_EQ(after.action_control, before.action_control);
}

}  // namespace
}  // namespace ra
