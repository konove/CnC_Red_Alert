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
#include <string>
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

}  // namespace

void TriggerAction::Detach(const Target target) {
  if (target.kind() == RTTI_TEAMTYPE && team_type_ == target.AsTeamType()) {
    team_type_ = nullptr;
  }
  if (target.kind() == RTTI_TRIGGERTYPE &&
      trigger_type_ == target.AsTriggerType()) {
    trigger_type_ = nullptr;
  }
}

void TriggerAction::ReadIni(base::Tokenizer& tokens) {
  type_ = static_cast<TriggerActionType>(
      base::ParseIntegerOr<int>(tokens.Next(), 0));

  if (const int format = TheWorld().new_ini_format();
      format != 0 && format != 1) {
    // Format 2 and later: the team and the trigger are positions in their
    // type_ lists (ToIniEntry() writes this form).
    team_type_ = TheObjectHeaps().team_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
    trigger_type_ = TheObjectHeaps().trigger_type().SlotOrNull(
        base::ParseIntegerOr<int>(tokens.Next(), 0));
  } else {
    // The shipped scenarios: the team and the trigger are given by name.
    team_type_ = TeamTypeClass::From_Name(tokens.Next());
    if (ActionNeed(type_) == NEED_TEAM && team_type_ == nullptr) {
      DLOG(WARNING) << "ReadIni: " << ActionName(type_)
                    << " names a team that does not exist";
    }

    // The named trigger may not have been read yet, so keep the name;
    // TriggerType::ReadIni() resolves it once every trigger exists.
    pending_trigger_name_ = tokens.Next();
  }

  data_.value = base::ParseIntegerOr<int>(tokens.Next(), 0);

  // Fix corrupted sound values from original game scenario files. Some INI
  // files contain values like -65399 (0xFFFF0089) where only the low byte (137)
  // is the valid VocType index. The original read the value through a one-byte
  // enum; VocType is wider here, so the byte is taken by hand. (The one-byte
  // data_ members - movie, special, speech, house - still get it from the
  // union.)
  if (type_ == TriggerActionType::kPlaySound &&
      (data_.value < 0 ||
       std::cmp_greater_equal(data_.value,
                              magic_enum::enum_count<VocType>()))) {
    const int low_byte =
        static_cast<int>(static_cast<uint32_t>(data_.value) & 0xFFU);
    if (std::cmp_less(low_byte, magic_enum::enum_count<VocType>())) {
      DLOG(WARNING) << "ReadIni: Fixed corrupted sound value " << data_.value
                    << " -> " << low_byte;
      data_.value = low_byte;
    }
  }
}

std::string TriggerAction::ToIniEntry() const {
  return std::format("{},{},{},{}", std::to_underlying(type_),
                     TheObjectHeaps().team_type().PositionOf(team_type_),
                     TheObjectHeaps().trigger_type().PositionOf(trigger_type_),
                     data_.value);
}

// Not const: carries out the action.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool TriggerAction::Perform(const HousesType house, ObjectClass* object,
                            const int trigger_id, const Cell cell) {
  HouseClass* owner = HouseClass::As_Pointer(house);
  bool success = true;

  // The object may already be dead: a trigger with two actions passes the same
  // object to both, and the first may have killed it.
  if (object && !object->IsActive) {
    object = nullptr;
  }

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

    // Launch nuclear missiles (duds) from every missile silo, whoever owns it.
    case TriggerActionType::kLaunchNukes: {
      for (int index = 0; index < TheObjectHeaps().building().Count();
           index++) {
        BuildingClass* silo = TheObjectHeaps().building().ActiveAt(index);
        if (*silo == STRUCT_MSLO) {
          silo->Assign_Mission(MISSION_MISSILE);
        }
      }
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

    // Cause the shadow to creep back one step.
    case TriggerActionType::kCreepShadow:
      TheMap().Encroach_Shadow();
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

    // Reveal the entire map.
    case TriggerActionType::kRevealAll:
      if (!ThePlayer()->IsVisionary) {
        ThePlayer()->IsVisionary = true;
        for (int map_cell_index = 0; map_cell_index < MAP_CELL_TOTAL;
             ++map_cell_index) {
          const Cell map_cell = Cell::FromIndex(map_cell_index);
          TheMap().Map_Cell(map_cell, ThePlayer());
        }
      }
      break;

    // Start the mission timer.
    case TriggerActionType::kStartTimer:
      if (!TheScenario().MissionTimer.IsRunning()) {
        TheScenario().MissionTimer.Start();
        TheMap().Redraw_Tab();
      }
      break;

    // Stop the mission timer. This will really just suspend the timer.
    case TriggerActionType::kStopTimer:
      if (TheScenario().MissionTimer.IsRunning()) {
        TheScenario().MissionTimer.Stop();
        TheMap().Redraw_Tab();
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

    // Flag the house specified as the winner. Only the player is ever flagged:
    // the house says whether the player is the one who won or lost.
    case TriggerActionType::kWin:
      if (data_.house == ThePlayer()->Class->House) {
        ThePlayer()->Flag_To_Win();
      } else {
        ThePlayer()->Flag_To_Lose();
      }
      break;

    // Flag the house specified as the loser; the mirror image of the above.
    case TriggerActionType::kLose:
      if (data_.house != ThePlayer()->Class->House) {
        ThePlayer()->Flag_To_Win();
      } else {
        ThePlayer()->Flag_To_Lose();
      }
      break;

    // This will enable production to begin for the house specified.
    case TriggerActionType::kBeginProduction:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data_.house);
          specified_house != nullptr) {
        specified_house->Begin_Production();
      }
      break;

    // Cause all buildings to be sold and all units to go into hunt mode. The
    // house's AI does both once it is in the endgame state.
    case TriggerActionType::kFireSale:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data_.house);
          specified_house != nullptr) {
        specified_house->State = STATE_ENDGAME;
      }
      break;

    // Begin the team autocreate logic for the house specified.
    case TriggerActionType::kAutocreate:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data_.house);
          specified_house != nullptr) {
        specified_house->IsAlerted = true;
      }
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

    // Force all units of the house specified to go into hunt mode.
    case TriggerActionType::kAllHunt:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data_.house);
          specified_house != nullptr) {
        specified_house->Do_All_To_Hunt();
      }
      break;

    // Destroy the object the trigger sprang on, and report failure when there
    // is none (so the trigger is not used up).
    case TriggerActionType::kDestroyObject:
      if (object != nullptr) {
        Destroy(*object);
      } else {
        success = false;
      }

      // If the trigger is attached to a bridge, then the bridge gets destroyed
      // regardless of whether the trigger was a forced or natural spring event.
      // Cell 0 means the trigger is not attached to a cell.
      if (cell.index() != 0) {
        TheMap().Destroy_Bridge_At(cell);
      }

      // Destroy all objects that have this trigger attached to them.
      if (trigger_id != -1) {
        const TriggerClass* sprung_trigger =
            TheObjectHeaps().trigger().Slot(trigger_id);
        DestroyObjectsAttachedTo(TheObjectHeaps().unit(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().infantry(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().aircraft(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().building(), sprung_trigger);
      }
      break;

    // No action. TriggerActionType::kAllowWin is not carried out here:
    // TriggerClass and the scenario's win check look for it by type_.
    case TriggerActionType::kNone:
    case TriggerActionType::kWinLose:
    case TriggerActionType::kAllowWin:
    default:
      break;
  }
  return success;
}

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

}  // namespace ra
