/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// The actions a scenario trigger performs when it springs: carrying them out,
// reading and writing them in the scenario INI, and naming them in the editor.

#include "ra/ai/trigger_action.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/fixed.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "engine/window/hardware_cursor.h"
#include "game/core/object_heap.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/reinforcements.h"
#include "ra/ai/scenario.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/assets.h"
#include "ra/assets/audio.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/data/rules.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/super.h"
#include "ra/objects/tracker.h"
#include "ra/objects/type.h"
#include "ra/screens/movie.h"
#include "ra/ui/msglist.h"

namespace ra {

namespace {

// The unit of the timer actions' values, a tenth of a minute, in game ticks.
constexpr int64_t kTicksPerTimerUnit = kTicksPerMinute / 10;

// Kills `object` with damage equal to its strength, which no armor reduces.
void Destroy(ObjectClass& object) {
  // Take_Damage() takes the damage by reference.
  int damage = object.Strength;
  object.Take_Damage(damage, 0, WARHEAD_AP, nullptr, true);
}

// Destroys every object in `heap` that `trigger` is attached to. Each is
// detached first so its death does not spring the trigger again.
// TODO: an object that Take_Damage() deletes on the spot closes its gap in the
// heap's in-use list, and the loop then steps over the object that moved into
// it. Stepping back (as TriggerActionType::kDestroyTrigger does) would kill
// objects that survive today.
template <class Heap>
void DestroyObjectsAttachedTo(Heap& heap, const TriggerClass* trigger) {
  for (int index = 0; index < heap.Count(); index++) {
    auto* object = heap.ActiveAt(index);
    if (object->Trigger == trigger) {
      object->Trigger = nullptr;
      Destroy(*object);
    }
  }
}

// The scenario editor's name for each action. A name ending in "..." tells the
// designer the action takes a value (see ActionNeed()).
constexpr base::EnumArray<TriggerActionType, const char*> kActionNames = {
    "-No Action-",
    "Winner is...",
    "Loser is...",
    "Production Begins",
    "Create Team...",
    "Destroy All Teams",
    "All to Hunt...",
    "Reinforcement (team)...",
    "Drop Zone Flare (waypoint)...",
    "Fire Sale...",
    "Play Movie...",
    "Text Trigger (ID num)...",
    "Destroy Trigger...",
    "Autocreate Begins...",
    "~don't use~",
    "Allow Win",
    "Reveal all map",
    "Reveal around waypoint...",
    "Reveal zone of waypoint...",
    "Play sound effect...",
    "Play music theme...",
    "Play speech...",
    "Force Trigger...",
    "Timer Start",
    "Timer Stop",
    "Timer Extend (1/10th min)...",
    "Timer Shorten (1/10th min)...",
    "Timer Set (1/10th min)...",
    "Global Set...",
    "Global Clear...",
    "Auto Base Building...",
    "Grow shroud one 'step'",
    "Destroy attached building",
    "Add 1-time special weapon...",
    "Add repeating special weapon...",
    "Preferred target...",
    "Launch Nukes"};

// Returns the object in slot `slot_index` of `heap`, or nullptr when the index
// is none or the scenario never filled the slot: an empty slot would read as a
// zeroed object. `field` names the reference for the log.
template <class T>
T* FilledSlotOrNull(game::ObjectHeap<T>& heap, const int slot_index,
                    const std::string_view field) {
  if (heap.IsAllocated(slot_index)) {
    return heap.Slot(slot_index);
  }
  if (heap.SlotOrNull(slot_index) != nullptr) {
    DLOG(WARNING) << "ReadTriggerActionRecord: action " << field
                  << " names slot " << slot_index
                  << ", which the scenario never filled";
  }
  return nullptr;
}

// Returns the low byte of `value` as a number. The original game kept the
// one-byte values (house, special weapon, movie, speech, flag) in a union with
// the int the scenario stores, and the shipped scenarios rely on it: SCG12EA's
// nuke is special weapon -255 and SCG10EA's "lch3" plays movie -170.
constexpr int LowByte(const int value) { return static_cast<int8_t>(value); }

// Returns the one-byte enum a record's `value` means, which is its low byte.
template <class Enum>
constexpr Enum LowByteAs(const int value) {
  return static_cast<Enum>(LowByte(value));
}

// Returns the sound a record's `value` means: the value, or its low byte when
// the value is no sound and the byte is one. SCU31EA and others store a sound
// as 0xFFFF00xx; the original read it through a one-byte enum, and VocType is
// wider here.
int SoundValue(const int value) {
  const auto is_sound = [](const int number) {
    return number >= 0 &&
           std::cmp_less(number, magic_enum::enum_count<VocType>());
  };
  if (is_sound(value)) {
    return value;
  }
  const int low_byte = static_cast<int>(static_cast<uint32_t>(value) & 0xFFU);
  if (!is_sound(low_byte)) {
    return value;
  }
  DLOG(WARNING) << "SoundValue: read sound " << value << " as " << low_byte;
  return low_byte;
}

// kNone, and the two types that are not carried out here: kAllowWin, which
// TriggerClass and the scenario's win check look for by type, and the unused
// kWinLose. Also what a type number that is no action becomes.
class NoAction final : public TriggerAction {
 public:
  explicit NoAction(const TriggerActionType type) : type_(type) {}

  [[nodiscard]] TriggerActionType type() const override { return type_; }
  bool Perform(const TriggerActionContext& /*context*/) override {
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type_};
  }

 private:
  TriggerActionType type_;
};

// Reveals the entire map to the player.
class RevealAllAction final : public TriggerAction {
 public:
  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kRevealAll;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    if (!ThePlayer()->IsVisionary) {
      ThePlayer()->IsVisionary = true;
      for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
        TheMap().Map_Cell(Cell::FromIndex(cell_index), ThePlayer());
      }
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type()};
  }
};

// Starts the mission timer (kStartTimer) or stops it (kStopTimer), which only
// suspends it: the time left stays.
class TimerSwitchAction final : public TriggerAction {
 public:
  explicit TimerSwitchAction(const TriggerActionType type) : type_(type) {}

  [[nodiscard]] TriggerActionType type() const override { return type_; }
  bool Perform(const TriggerActionContext& /*context*/) override {
    const bool start = type_ == TriggerActionType::kStartTimer;
    auto& timer = TheScenario().MissionTimer;
    if (timer.IsRunning() != start) {
      if (start) {
        timer.Start();
      } else {
        timer.Stop();
      }
      TheMap().Redraw_Tab();
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type_};
  }

 private:
  TriggerActionType type_;
};

// Makes the shroud creep back one step.
class CreepShadowAction final : public TriggerAction {
 public:
  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kCreepShadow;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    TheMap().Encroach_Shadow();
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type()};
  }
};

// Destroys what the trigger is attached to: the object it sprang on, the
// bridge at its cell and every object that carries it.
class DestroyObjectAction final : public TriggerAction {
 public:
  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kDestroyObject;
  }
  bool Perform(const TriggerActionContext& context) override {
    // Reports failure when there is no object to destroy, so the trigger is
    // not used up. The object may already be dead: a trigger with two actions
    // passes the same object to both, and the first may have killed it.
    const bool has_object =
        context.object != nullptr && context.object->IsActive;
    if (has_object) {
      Destroy(*context.object);
    }

    // If the trigger is attached to a bridge, then the bridge gets destroyed
    // regardless of whether the trigger was a forced or natural spring event.
    // Cell 0 means the trigger is not attached to a cell.
    if (context.cell.index() != 0) {
      TheMap().Destroy_Bridge_At(context.cell);
    }

    // Destroy all objects that have this trigger attached to them.
    if (context.trigger_id != -1) {
      const TriggerClass* sprung_trigger =
          TheObjectHeaps().trigger().Slot(context.trigger_id);
      DestroyObjectsAttachedTo(TheObjectHeaps().unit(), sprung_trigger);
      DestroyObjectsAttachedTo(TheObjectHeaps().infantry(), sprung_trigger);
      DestroyObjectsAttachedTo(TheObjectHeaps().aircraft(), sprung_trigger);
      DestroyObjectsAttachedTo(TheObjectHeaps().building(), sprung_trigger);
    }
    return has_object;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type()};
  }
};

// Launches nuclear missiles (duds) from every missile silo, whoever owns it.
class LaunchNukesAction final : public TriggerAction {
 public:
  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kLaunchNukes;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
      BuildingClass* silo = TheObjectHeaps().building().ActiveAt(index);
      if (*silo == STRUCT_MSLO) {
        silo->Assign_Mission(MISSION_MISSILE);
      }
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type()};
  }
};

// Declares the game won or lost. Only the player is ever flagged: the house
// named says whether the player is the one who won (kWin) or lost (kLose).
class DeclareWinnerAction final : public TriggerAction {
 public:
  DeclareWinnerAction(const TriggerActionType type, const HousesType house)
      : type_(type), house_(house) {}

  [[nodiscard]] TriggerActionType type() const override { return type_; }
  bool Perform(const TriggerActionContext& /*context*/) override {
    const bool names_player = house_ == ThePlayer()->Class->House;
    if (names_player == (type_ == TriggerActionType::kWin)) {
      ThePlayer()->Flag_To_Win();
    } else {
      ThePlayer()->Flag_To_Lose();
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type_, .value = static_cast<int>(house_)};
  }

 private:
  TriggerActionType type_;
  HousesType house_;
};

// Lets the house named start building. As for the three classes below, a
// house the scenario does not have (or HOUSE_NONE) is left alone.
class BeginProductionAction final : public TriggerAction {
 public:
  explicit BeginProductionAction(const HousesType house) : house_(house) {}

  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kBeginProduction;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    if (HouseClass* house = HouseClass::As_Pointer(house_); house != nullptr) {
      house->Begin_Production();
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type(), .value = static_cast<int>(house_)};
  }

 private:
  HousesType house_;
};

// Sends every unit of the house named into hunt mode.
class AllHuntAction final : public TriggerAction {
 public:
  explicit AllHuntAction(const HousesType house) : house_(house) {}

  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kAllHunt;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    if (HouseClass* house = HouseClass::As_Pointer(house_); house != nullptr) {
      house->Do_All_To_Hunt();
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type(), .value = static_cast<int>(house_)};
  }

 private:
  HousesType house_;
};

// Makes the house named sell all its buildings and send its units to
// hunt. The house's AI does both once it is in the endgame state.
class FireSaleAction final : public TriggerAction {
 public:
  explicit FireSaleAction(const HousesType house) : house_(house) {}

  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kFireSale;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    if (HouseClass* house = HouseClass::As_Pointer(house_); house != nullptr) {
      house->State = STATE_ENDGAME;
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type(), .value = static_cast<int>(house_)};
  }

 private:
  HousesType house_;
};

// Starts the team autocreate logic of the house named.
class AutocreateAction final : public TriggerAction {
 public:
  explicit AutocreateAction(const HousesType house) : house_(house) {}

  [[nodiscard]] TriggerActionType type() const override {
    return TriggerActionType::kAutocreate;
  }
  bool Perform(const TriggerActionContext& /*context*/) override {
    if (HouseClass* house = HouseClass::As_Pointer(house_); house != nullptr) {
      house->IsAlerted = true;
    }
    return true;
  }
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type(), .value = static_cast<int>(house_)};
  }

 private:
  HousesType house_;
};

// Returns `record` as an action keeps it: the fields its type does not use at
// their defaults, and the value as the number of what the action acts on.
// TODO: scaffolding for LegacyAction; each action class converts its own
// value in MakeTriggerAction() (docs/TRIGGER_ACTION_CLASSES_PLAN.md).
TriggerActionRecord Normalized(const TriggerActionRecord& record) {
  TriggerActionRecord kept{.type = record.type};
  switch (ActionNeed(record.type)) {
    case NEED_TEAM:
      kept.team_type = record.team_type;
      break;

    case NEED_TRIGGER:
      kept.trigger_type = record.trigger_type;
      break;

    case NEED_HOUSE:
    case NEED_SPECIAL:
    case NEED_MOVIE:
    case NEED_SPEECH:
      kept.value = LowByte(record.value);
      break;

    case NEED_BOOL:
      kept.value = LowByte(record.value) != 0 ? 1 : 0;
      break;

    case NEED_SOUND:
      kept.value = SoundValue(record.value);
      break;

    case NEED_THEME:
    case NEED_QUARRY:
    case NEED_WAYPOINT:
    case NEED_NUMBER:
      kept.value = record.value;
      break;

    case NeedType::NEED_NONE:
    case NeedType::NEED_INFANTRY:
    case NeedType::NEED_UNIT:
    case NeedType::NEED_AIRCRAFT:
    case NeedType::NEED_STRUCTURE:
    case NeedType::NEED_TIME:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }
  return kept;
}

// Every action that has no class of its own yet
// (docs/TRIGGER_ACTION_CLASSES_PLAN.md): the record's fields and a switch over
// the type.
class LegacyAction final : public TriggerAction {
 public:
  explicit LegacyAction(const TriggerActionRecord& record)
      : type_(record.type),
        team_type_(record.team_type),
        trigger_type_(record.trigger_type),
        data_{.value = record.value} {}

  [[nodiscard]] TriggerActionType type() const override { return type_; }
  bool Perform(const TriggerActionContext& context) override;
  void Detach(Target target) override;
  [[nodiscard]] TriggerActionRecord ToRecord() const override {
    return {.type = type_,
            .team_type = team_type_,
            .trigger_type = trigger_type_,
            .value = data_.value};
  }

 private:
  TriggerActionType type_;
  TeamTypeClass* team_type_;
  TriggerType* trigger_type_;

  // The action's value; type_ decides which member means something (see
  // ActionNeed()). The one-byte members alias the low byte of `value`.
  union {
    ThemeType theme;            // Musical theme.
    VocType sound;              // Sound effect.
    VoxType speech;             // Speech identifier.
    SpecialWeaponType special;  // Special weapon ability.
    QuarryType quarry;          // Preferred target for attack.
    VQType movie;               // The movie to play.
    bool flag;                  // Boolean value.
    int value;
  } data_;
};

void LegacyAction::Detach(const Target target) {
  if (target.kind() == RTTI_TEAMTYPE && team_type_ == target.AsTeamType()) {
    team_type_ = nullptr;
  }
  if (target.kind() == RTTI_TRIGGERTYPE &&
      trigger_type_ == target.AsTriggerType()) {
    trigger_type_ = nullptr;
  }
}

// Not const: carries out the action.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool LegacyAction::Perform(const TriggerActionContext& context) {
  HouseClass* owner = HouseClass::As_Pointer(context.house);
  bool success = true;

  switch (type_) {
    // Display a line of the mission's tutorial text over the tactical map.
    // An ID with no text displays nothing.
    case TriggerActionType::kTextTrigger: {
      const char* message = TheAssets().tutorial_text(data_.value);
      if (message == nullptr) {
        break;
      }
      TheSession().Messages.Add_Message(
          nullptr, 0, message, PCOLOR_GREEN, FontType::k6PointGradient,
          TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
          TheRules().MessageDelay * kTicksPerMinute);
      break;
    }

    // Set the preferred target for the house.
    case TriggerActionType::kPreferredTarget:
      if (owner != nullptr) {
        owner->PreferredTarget = data_.quarry;
      }
      break;

    // Turn the house's computer base building on or off. When on, the computer
    // builds bases and units. As for every house action here, a house the
    // scenario does not have (or HOUSE_NONE) is left alone.
    case TriggerActionType::kBaseBuilding:
      if (owner != nullptr) {
        owner->IsBaseBuilding = data_.flag;
      }
      break;

    // Set or clear a scenario global.
    case TriggerActionType::kSetGlobal:
    case TriggerActionType::kClearGlobal:
      TheScenario().Set_Global_To(data_.value,
                                  type_ == TriggerActionType::kSetGlobal);
      break;

    // Reveal the map around the waypoint to the player. Nothing to do once the
    // player sees everything.
    case TriggerActionType::kRevealSome:
      if (!ThePlayer()->IsVisionary) {
        TheMap().Sight_From(base::At(TheScenario().Waypoint, data_.value),
                            TheRules().GapShroudRadius, ThePlayer(), false);
      }
      break;

    // Reveal every cell of the movement zone the waypoint is in, which shows
    // whole islands or bodies of water. Zones are compared as the heaviest
    // vehicles see them (MZONE_CRUSHER), where crushable walls do not divide
    // the land.
    case TriggerActionType::kRevealZone:
      if (!ThePlayer()->IsVisionary) {
        const int zone = TheMap()
                             .at(base::At(TheScenario().Waypoint, data_.value))
                             .Zones.at(MZONE_CRUSHER);

        for (int map_cell_index = 0; map_cell_index < MAP_CELL_TOTAL;
             ++map_cell_index) {
          const Cell map_cell = Cell::FromIndex(map_cell_index);
          if (std::cmp_equal(TheMap().at(map_cell).Zones.at(MZONE_CRUSHER),
                             zone)) {
            TheMap().Map_Cell(map_cell, ThePlayer());
          }
        }
      }
      break;

    // Add time to the mission timer. The value is in tenths of a minute, as
    // for the two actions below.
    case TriggerActionType::kAddTimer:
      TheScenario().MissionTimer.Set(TheScenario().MissionTimer.Remaining() +
                                     (data_.value * kTicksPerTimerUnit));
      TheMap().Redraw_Tab();
      break;

    // Remove time from the mission timer, stopping at zero.
    case TriggerActionType::kSubTimer:
      TheScenario().MissionTimer.Set(
          std::max<int64_t>(0, TheScenario().MissionTimer.Remaining() -
                                   (data_.value * kTicksPerTimerUnit)));
      TheMap().Redraw_Tab();
      break;

    // Set the mission timer to the value specified.
    case TriggerActionType::kSetTimer:
      TheScenario().MissionTimer.Set(data_.value * kTicksPerTimerUnit);
      TheScenario().MissionTimer.Start();
      TheMap().Redraw_Tab();
      break;

    // Play a movie immediately. The game is suspended while it plays; the
    // movie leaves its own palette behind, so the game's is restored and the
    // whole map redrawn.
    case TriggerActionType::kPlayMovie:
      game::TheMouse().Hide();
      TheScreen().visible_view().Clear();
      Play_Movie(data_.movie, THEME_NONE, true);
      ThePalettes().game_palette().Set();
      TheMap().Flag_To_Redraw(true);
      game::TheMouse().Show();
      break;

    // Play a sound effect.
    case TriggerActionType::kPlaySound:
      PlaySoundEffect(data_.sound);
      break;

    // Play a musical theme.
    case TriggerActionType::kPlayMusic:
      TheTheme().Queue_Song(data_.theme);
      break;

    // Play the speech data_ specified.
    case TriggerActionType::kPlaySpeech:
      Speak(data_.speech);
      break;

    // Give the special weapon to the house, for one use or for good, and put
    // its button on the player's sidebar.
    case TriggerActionType::kOneSpecial:
    case TriggerActionType::kFullSpecial:
      if (owner == nullptr) {
        break;
      }
      owner->SuperWeapon.at(data_.special)
          .Enable(type_ == TriggerActionType::kOneSpecial, false);

      if (ThePlayer() == owner) {
        TheMap().Add(RTTI_SPECIAL, static_cast<int>(data_.special));
        TheMap().Column[1].Flag_To_Redraw();
      }
      break;

    // Destroying a trigger means that all triggers of that type_ will be
    // destroyed. Deleting one closes the gap in the heap's in-use list, so the
    // index is stepped back to look at the trigger that moved into it. The
    // springing trigger may be among them; TriggerClass::Spring() checks for
    // that when this returns.
    case TriggerActionType::kDestroyTrigger:
      if (trigger_type_ != nullptr) {
        for (int index = 0; index < TheObjectHeaps().trigger().Count();
             index++) {
          TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(index);
          if (trigger->Class == trigger_type_) {
            Detach_This_From_All(trigger->As_Target());
            base::DeletePooled(trigger);
            index--;
          }
        }
      }
      break;

    // A forced trigger will force an existing trigger of that type_ or will
    // create a trigger of that type_ and then force it to be sprung. Nothing
    // is sprung when the trigger heap is full and none could be made.
    case TriggerActionType::kForceTrigger:
      if (TriggerClass* forced_trigger = Find_Or_Make(trigger_type_);
          forced_trigger != nullptr) {
        forced_trigger->Spring(TEVENT_ANY, nullptr, Cell{}, true);
      }
      break;

    // Place a smoke marker at the waypoint specified.
    case TriggerActionType::kDropZoneFlare:
      base::NewPooled<AnimClass>(
          ANIM_LZ_SMOKE,
          base::At(TheScenario().Waypoint, data_.value).Center());
      break;

    // Manually create the team specified. Counting as scenario setup makes
    // Create_One_Of() ignore the type_'s limit on how many teams may exist.
    // team_type_ is null when the INI named a team that does not exist; the
    // three team actions then do nothing.
    case TriggerActionType::kCreateTeam:
      if (team_type_ != nullptr) {
        TheWorld().scenario_init()++;
        team_type_->Create_One_Of();
        TheWorld().scenario_init()--;
      }
      break;

    // Destroy all teams of the type_ specified.
    case TriggerActionType::kDestroyTeam:
      if (team_type_ != nullptr) {
        team_type_->Destroy_All_Of();
      }
      break;

    // Create a reinforcement of the team specified.
    case TriggerActionType::kReinforcements:
      success = team_type_ != nullptr && DeliverReinforcements(team_type_);
      break;

    // No action. TriggerActionType::kAllowWin is not carried out here:
    // TriggerClass and the scenario's win check look for it by type_.
    case TriggerActionType::kNone:
    case TriggerActionType::kWinLose:
    case TriggerActionType::kAllowWin:
    case TriggerActionType::kLaunchNukes:
    case TriggerActionType::kCreepShadow:
    case TriggerActionType::kRevealAll:
    case TriggerActionType::kStartTimer:
    case TriggerActionType::kStopTimer:
    case TriggerActionType::kDestroyObject:
    case TriggerActionType::kWin:
    case TriggerActionType::kLose:
    case TriggerActionType::kBeginProduction:
    case TriggerActionType::kFireSale:
    case TriggerActionType::kAutocreate:
    case TriggerActionType::kAllHunt:
    default:
      break;
  }
  return success;
}

}  // namespace

const char* ActionName(const TriggerActionType action) {
  return kActionNames.at(action);
}

NeedType ActionNeed(const TriggerActionType action) {
  switch (action) {
    case TriggerActionType::kOneSpecial:
    case TriggerActionType::kFullSpecial:
      return NEED_SPECIAL;

    case TriggerActionType::kFireSale:
    case TriggerActionType::kWin:
    case TriggerActionType::kLose:
    case TriggerActionType::kAllHunt:
    case TriggerActionType::kBeginProduction:
    case TriggerActionType::kAutocreate:
      return NEED_HOUSE;

    case TriggerActionType::kBaseBuilding:
      return NEED_BOOL;

    case TriggerActionType::kCreateTeam:
    case TriggerActionType::kDestroyTeam:
    case TriggerActionType::kReinforcements:
      return NEED_TEAM;

    case TriggerActionType::kForceTrigger:
    case TriggerActionType::kDestroyTrigger:
      return NEED_TRIGGER;

    case TriggerActionType::kDropZoneFlare:
    case TriggerActionType::kRevealSome:
    case TriggerActionType::kRevealZone:
      return NEED_WAYPOINT;

    case TriggerActionType::kPlayMusic:
      return NEED_THEME;

    case TriggerActionType::kPlayMovie:
      return NEED_MOVIE;

    case TriggerActionType::kPlaySound:
      return NEED_SOUND;

    case TriggerActionType::kPlaySpeech:
      return NEED_SPEECH;

    case TriggerActionType::kTextTrigger:
    case TriggerActionType::kAddTimer:
    case TriggerActionType::kSubTimer:
    case TriggerActionType::kSetTimer:
    case TriggerActionType::kSetGlobal:
    case TriggerActionType::kClearGlobal:
      return NEED_NUMBER;

    case TriggerActionType::kPreferredTarget:
      return NEED_QUARRY;

    case TriggerActionType::kNone:
    case TriggerActionType::kWinLose:
    case TriggerActionType::kAllowWin:
    case TriggerActionType::kRevealAll:
    case TriggerActionType::kStartTimer:
    case TriggerActionType::kStopTimer:
    case TriggerActionType::kCreepShadow:
    case TriggerActionType::kDestroyObject:
    case TriggerActionType::kLaunchNukes:
    default:
      break;
  }
  return NEED_NONE;
}

TriggerActionRecord ReadTriggerActionRecord(base::Tokenizer& tokens) {
  TriggerActionRecord record;
  record.type = static_cast<TriggerActionType>(
      base::ParseIntegerOr<int>(tokens.Next(), 0));

  if (const int format = TheWorld().new_ini_format();
      format != 0 && format != 1) {
    // Format 2 and later: the team and the trigger are heap slots
    // (ToIniEntry() writes this form).
    record.team_type =
        FilledSlotOrNull(TheObjectHeaps().team_type(),
                         base::ParseIntegerOr<int>(tokens.Next(), 0), "team");
    record.trigger_type = FilledSlotOrNull(
        TheObjectHeaps().trigger_type(),
        base::ParseIntegerOr<int>(tokens.Next(), 0), "trigger");
  } else {
    // The shipped scenarios: the team and the trigger are given by name.
    record.team_type = TeamTypeClass::From_Name(tokens.Next());
    if (ActionNeed(record.type) == NEED_TEAM && record.team_type == nullptr) {
      DLOG(WARNING) << "ReadTriggerActionRecord: " << ActionName(record.type)
                    << " names a team that does not exist";
    }
    record.trigger_type = TriggerType::FromName(tokens.Next());
  }

  record.value = base::ParseIntegerOr<int>(tokens.Next(), 0);
  return record;
}

std::string ToIniEntry(const TriggerActionRecord& record) {
  return std::format(
      "{},{},{},{}", std::to_underlying(record.type),
      TheObjectHeaps().team_type().PositionOf(record.team_type),
      TheObjectHeaps().trigger_type().PositionOf(record.trigger_type),
      record.value);
}

void TriggerAction::Detach(Target /*target*/) {}

std::unique_ptr<TriggerAction> MakeTriggerAction(
    const TriggerActionRecord& record) {
  const TriggerActionType type = record.type;
  switch (type) {
    case TriggerActionType::kRevealAll:
      return std::make_unique<RevealAllAction>();

    case TriggerActionType::kStartTimer:
    case TriggerActionType::kStopTimer:
      return std::make_unique<TimerSwitchAction>(type);

    case TriggerActionType::kCreepShadow:
      return std::make_unique<CreepShadowAction>();

    case TriggerActionType::kDestroyObject:
      return std::make_unique<DestroyObjectAction>();

    case TriggerActionType::kLaunchNukes:
      return std::make_unique<LaunchNukesAction>();

    case TriggerActionType::kWin:
    case TriggerActionType::kLose:
      return std::make_unique<DeclareWinnerAction>(
          type, LowByteAs<HousesType>(record.value));

    case TriggerActionType::kBeginProduction:
      return std::make_unique<BeginProductionAction>(
          LowByteAs<HousesType>(record.value));

    case TriggerActionType::kAllHunt:
      return std::make_unique<AllHuntAction>(
          LowByteAs<HousesType>(record.value));

    case TriggerActionType::kFireSale:
      return std::make_unique<FireSaleAction>(
          LowByteAs<HousesType>(record.value));

    case TriggerActionType::kAutocreate:
      return std::make_unique<AutocreateAction>(
          LowByteAs<HousesType>(record.value));

    // Not yet classes of their own.
    case TriggerActionType::kCreateTeam:
    case TriggerActionType::kDestroyTeam:
    case TriggerActionType::kReinforcements:
    case TriggerActionType::kDestroyTrigger:
    case TriggerActionType::kForceTrigger:
    case TriggerActionType::kDropZoneFlare:
    case TriggerActionType::kRevealSome:
    case TriggerActionType::kRevealZone:
    case TriggerActionType::kTextTrigger:
    case TriggerActionType::kAddTimer:
    case TriggerActionType::kSubTimer:
    case TriggerActionType::kSetTimer:
    case TriggerActionType::kSetGlobal:
    case TriggerActionType::kClearGlobal:
    case TriggerActionType::kPlayMovie:
    case TriggerActionType::kPlaySound:
    case TriggerActionType::kPlayMusic:
    case TriggerActionType::kPlaySpeech:
    case TriggerActionType::kBaseBuilding:
    case TriggerActionType::kOneSpecial:
    case TriggerActionType::kFullSpecial:
    case TriggerActionType::kPreferredTarget:
      return std::make_unique<LegacyAction>(Normalized(record));

    case TriggerActionType::kNone:
    case TriggerActionType::kWinLose:
    case TriggerActionType::kAllowWin:
    default:
      return std::make_unique<NoAction>(type);
  }
}

}  // namespace ra
