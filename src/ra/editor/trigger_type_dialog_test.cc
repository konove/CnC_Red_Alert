// Tests for the scenario editor's trigger dialog, TriggerType::Edit(), played
// by a script on a headless screen.

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "base/pooled.h"
#include "base/unaligned.h"
#include "engine/crypto/crc.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/display.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "game/ui/input.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/team_type.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/color_scheme.h"

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
    team_type_ = base::NewPooled<TeamType>();
    team_type_->Set_Name("tanks");
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alrm");
  }

  TeamType* team_type_ = nullptr;
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
  const TeamType* event1_team;
  const TeamType* event2_team;
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

// The size and the one colour of the arrow button the tests give the drop
// lists.
constexpr int kArrowWidth = 8;
constexpr int kArrowHeight = 9;
constexpr uint8_t kArrowColor = 250;

// A shape file of the three frames a button draws (up, down, disabled), each
// kArrowWidth by kArrowHeight pixels of kArrowColor.
std::string ArrowShape() {
  // The header, an offset entry per frame and the two that end the table,
  // then one run of pixels all the frames share.
  constexpr int kFrames = 3;
  constexpr size_t kHeaderSize = 14;
  constexpr size_t kEntrySize = 8;
  constexpr size_t kDataStart = kHeaderSize + ((kFrames + 2) * kEntrySize);
  constexpr uint32_t kKeyFrame = 0x80000000U;
  // LCW: fill the frame with one colour, then stop.
  const std::string pixels = {'\xfe', char{kArrowWidth * kArrowHeight}, '\0',
                              static_cast<char>(kArrowColor), '\x80'};
  std::vector<std::byte> file(kDataStart);
  const std::span<std::byte> bytes(file);
  base::WriteUnaligned<uint16_t>(bytes, kFrames);
  base::WriteUnaligned<uint16_t>(bytes.subspan(6), kArrowWidth);
  base::WriteUnaligned<uint16_t>(bytes.subspan(8), kArrowHeight);
  for (size_t frame = 0; frame < kFrames; ++frame) {
    base::WriteUnaligned<uint32_t>(
        bytes.subspan(kHeaderSize + (frame * kEntrySize)),
        kKeyFrame | uint32_t{kDataStart});
  }
  std::string shape(file.size(), '\0');
  std::ranges::transform(file, shape.begin(), [](const std::byte b) {
    return static_cast<char>(b);
  });
  return shape + pixels;
}

// An unencrypted MIX archive holding `files`, pairs of name and contents.
std::string MixOf(
    const std::vector<std::pair<std::string_view, std::string>>& files) {
  struct Entry {
    int32_t crc;
    int32_t offset;
    int32_t size;
  };
  std::vector<Entry> index;
  std::string data;
  for (const auto& [name, contents] : files) {
    index.push_back({.crc = std::bit_cast<int32_t>(CrcEngine::Compute(name)),
                     .offset = static_cast<int32_t>(data.size()),
                     .size = static_cast<int32_t>(contents.size())});
    data += contents;
  }
  // The archive finds a file by a binary search for its name's CRC.
  std::ranges::sort(index, {}, &Entry::crc);
  std::vector<std::byte> header(6 + (index.size() * sizeof(Entry)));
  const std::span<std::byte> bytes(header);
  base::WriteUnaligned<int16_t>(bytes, static_cast<int16_t>(index.size()));
  base::WriteUnaligned<int32_t>(bytes.subspan(2),
                                static_cast<int32_t>(data.size()));
  for (size_t i = 0; i < index.size(); ++i) {
    base::WriteUnaligned(bytes.subspan(6 + (i * sizeof(Entry))), index.at(i));
  }
  std::string mix(header.size(), '\0');
  std::ranges::transform(header, mix.begin(), [](const std::byte b) {
    return static_cast<char>(b);
  });
  return mix + data;
}

// The dialog with the arrow shapes its drop lists take their buttons from.
// Without them, as in the tests above, the buttons have no size and no click
// can open a list.
class TriggerTypeDialogDropListTest : public TriggerTypeDialogTest {
 protected:
  static constexpr const char* kArchive = "editor_buttons.mix";
  // Where the lists are: the choice of event or action on the left, the data
  // it takes to the right of it.
  static constexpr int kChoiceX = 45;
  static constexpr int kChoiceWidth = 160;
  static constexpr int kDataX = kChoiceX + kChoiceWidth + 20;
  static constexpr int kDataWidth = 95;
  static constexpr int kEvent1Y = 65;
  static constexpr int kAction1Y = 120;
  static constexpr int kAction2Y = kAction1Y + 22;

  void SetUp() override {
    TriggerTypeDialogTest::SetUp();
    WriteGameFile(kArchive, MixOf({{"EBTN-UP.SHP", ArrowShape()},
                                   {"EBTN-DN.SHP", ArrowShape()}}));
    ASSERT_NE(MixArchive::Register(kArchive), nullptr);
    ASSERT_TRUE(MixArchive::Cache(kArchive));
  }

  void TearDown() override {
    MixArchive::Unregister(kArchive);
    TriggerTypeDialogTest::TearDown();
  }

  // How far under the top of its text box a list opens: the lists of events
  // and actions, and the lists of what they take.
  static constexpr int kChoiceListTop = 9;
  static constexpr int kDataListTop = 18;
  // The middle of the first and second rows of an open list, from its top; a
  // row is as tall as a line of the 6x10 font.
  static constexpr int kFirstRow = 5;
  static constexpr int kSecondRow = 15;

  // Queues, at `frame`, a click on the arrow of the drop list whose text box
  // is `width` wide at (`x`, `y`). Returns a frame by which it is open.
  int Open(const int frame, const int x, const int y, const int width) {
    player().At(frame).Click(x + width + 3, y + 3);
    return frame + 5;
  }

  // Queues, from `frame` on, opening that list and a click on the second row
  // of it, `list_top` under the text box. The pick closes the list. Returns
  // a frame by which the dialog has taken it.
  int PickSecondRow(const int frame, const int x, const int y, const int width,
                    const int list_top) {
    player()
        .At(Open(frame, x, y, width))
        .Click(x + 5, y + list_top + kSecondRow);
    return frame + 10;
  }

  // The second of `names` in the order the lists show them: by name,
  // ignoring case.
  template <class Kind>
  static Kind SecondByName(std::vector<std::pair<std::string, Kind>> names) {
    std::ranges::sort(names, {}, [](const auto& entry) {
      return absl::AsciiStrToLower(entry.first);
    });
    return names.at(1).second;
  }
};

TEST_F(TriggerTypeDialogDropListTest, AnEventPickedFromTheListIsStored) {
  std::vector<std::pair<std::string, TEventType>> names;
  for (const TEventType type : magic_enum::enum_values<TEventType>()) {
    if (type != TEVENT_COUNT) {
      names.emplace_back(EventChoices.at(type).Description(), type);
    }
  }
  const TEventType second = SecondByName(names);
  ASSERT_NE(second, TEVENT_NONE);

  const int frame =
      PickSecondRow(3, kChoiceX, kEvent1Y, kChoiceWidth, kChoiceListTop);
  player().At(frame).Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->event1.Event, second);
  EXPECT_EQ(trigger_type_->event2.Event, TEVENT_NONE);
}

TEST_F(TriggerTypeDialogDropListTest, AnActionPickedFromTheListIsStored) {
  std::vector<std::pair<std::string, TriggerActionType>> names;
  for (const TriggerActionType type :
       magic_enum::enum_values<TriggerActionType>()) {
    names.emplace_back(ActionName(type), type);
  }
  const TriggerActionType second = SecondByName(names);
  ASSERT_NE(second, TriggerActionType::kNone);

  const int frame =
      PickSecondRow(3, kChoiceX, kAction1Y, kChoiceWidth, kChoiceListTop);
  player().At(frame).Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->action1->type(), second);
  EXPECT_EQ(trigger_type_->action2->type(), TriggerActionType::kNone);
}

// The open list shows the action chosen, the first row here, as a bar of
// the dialog's colour scheme and the others as text alone.
TEST_F(TriggerTypeDialogDropListTest, TheOpenActionListHighlightsTheChoice) {
  RemapControlType* const previous = Get_Color_Scheme();
  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_RED));
  engine::gfx::PaletteIndex first_row;
  engine::gfx::PaletteIndex second_row;
  // A column past the end of every action's name.
  const int x = kChoiceX + kChoiceWidth - 2;
  const int y = kAction1Y + kChoiceListTop;
  player()
      .At(Open(3, kChoiceX, kAction1Y, kChoiceWidth))
      .Do([&] {
        PixelView& view = TheScreen().visible_view();
        first_row = view.GetPixel(x, y + kFirstRow);
        second_row = view.GetPixel(x, y + kSecondRow);
      })
      .At(12)
      .Click(kCancelX, kButtonsY);

  EXPECT_FALSE(trigger_type_->Edit());
  Set_Color_Scheme(previous);
  EXPECT_EQ(first_row, SchemeColor(PCOLOR_RED));
  EXPECT_NE(second_row, SchemeColor(PCOLOR_RED));
}

// Picking a speech or a sound plays it, which nothing here can hear; what
// the test sees is that the pick is kept.
TEST_F(TriggerTypeDialogDropListTest, ASpeechPickedFromTheListIsStored) {
  trigger_type_->action1 =
      MakeTriggerAction({.type = TriggerActionType::kPlaySpeech});
  trigger_type_->action2 =
      MakeTriggerAction({.type = TriggerActionType::kPlaySpeech});
  trigger_type_->action_control = MULTI_AND;
  int frame = PickSecondRow(3, kDataX, kAction1Y, kDataWidth, kDataListTop);
  frame = PickSecondRow(frame, kDataX, kAction2Y, kDataWidth, kDataListTop);
  player().At(frame).Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->action1->ToRecord().value, 1);
  EXPECT_EQ(trigger_type_->action2->ToRecord().value, 1);
}

TEST_F(TriggerTypeDialogDropListTest, ASoundPickedFromTheListIsStored) {
  trigger_type_->action1 =
      MakeTriggerAction({.type = TriggerActionType::kPlaySound});
  trigger_type_->action2 =
      MakeTriggerAction({.type = TriggerActionType::kPlaySound});
  trigger_type_->action_control = MULTI_AND;
  int frame = PickSecondRow(3, kDataX, kAction1Y, kDataWidth, kDataListTop);
  frame = PickSecondRow(frame, kDataX, kAction2Y, kDataWidth, kDataListTop);
  player().At(frame).Click(kOkX, kButtonsY);

  EXPECT_TRUE(trigger_type_->Edit());
  EXPECT_EQ(trigger_type_->action1->ToRecord().value, 1);
  EXPECT_EQ(trigger_type_->action2->ToRecord().value, 1);
}

}  // namespace
}  // namespace ra
