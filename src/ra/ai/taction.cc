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

#include "ra/ai/taction.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
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
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/msglist.h"

namespace ra {

namespace {

// The unit of the timer actions' values, a tenth of a minute, in game ticks.
constexpr int64_t kTicksPerTimerUnit = kTicksPerMinute / 10;

// Destroys every object in `heap` that `trigger` is attached to. Each is
// detached first so its death does not spring the trigger again.
// TODO: an object that Take_Damage() deletes on the spot closes its gap in the
// heap's in-use list, and the loop then steps over the object that moved into
// it. Stepping back (as TACTION_DESTROY_TRIGGER does) would kill objects that
// survive today.
template <class Heap>
void DestroyObjectsAttachedTo(Heap& heap, const TriggerClass* trigger) {
  for (int index = 0; index < heap.Count(); index++) {
    auto* object = heap.ActiveAt(index);
    if (object->Trigger == trigger) {
      object->Trigger = nullptr;
      // Take_Damage() takes the damage by reference.
      int damage = object->Strength;
      object->Take_Damage(damage, 0, WARHEAD_AP, nullptr, true);
    }
  }
}

}  // namespace

// The scenario editor's name for each action. A name ending in "..." tells the
// designer the action takes a value (see ActionNeed()).
static constexpr base::EnumArray<TriggerActionType, const char*> kActionNames =
    {"-No Action-",
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

// One choice per action, in enum order. The editor's drop lists hold pointers
// to these, so they need storage that outlives the dialog.
base::EnumArray<TriggerActionType, ActionChoice> action_choices = {
    {{TACTION_NONE},
     {TACTION_WIN},
     {TACTION_LOSE},
     {TACTION_BEGIN_PRODUCTION},
     {TACTION_CREATE_TEAM},
     {TACTION_DESTROY_TEAM},
     {TACTION_ALL_HUNT},
     {TACTION_REINFORCEMENTS},
     {TACTION_DZ},
     {TACTION_FIRE_SALE},
     {TACTION_PLAY_MOVIE},
     {TACTION_TEXT_TRIGGER},
     {TACTION_DESTROY_TRIGGER},
     {TACTION_AUTOCREATE},
     {TACTION_WINLOSE},
     {TACTION_ALLOWWIN},
     {TACTION_REVEAL_ALL},
     {TACTION_REVEAL_SOME},
     {TACTION_REVEAL_ZONE},
     {TACTION_PLAY_SOUND},
     {TACTION_PLAY_MUSIC},
     {TACTION_PLAY_SPEECH},
     {TACTION_FORCE_TRIGGER},
     {TACTION_START_TIMER},
     {TACTION_STOP_TIMER},
     {TACTION_ADD_TIMER},
     {TACTION_SUB_TIMER},
     {TACTION_SET_TIMER},
     {TACTION_SET_GLOBAL},
     {TACTION_CLEAR_GLOBAL},
     {TACTION_BASE_BUILDING},
     {TACTION_CREEP_SHADOW},
     {TACTION_DESTROY_OBJECT},
     {TACTION_1_SPECIAL},
     {TACTION_FULL_SPECIAL},
     {TACTION_PREFERRED_TARGET},
     {TACTION_LAUNCH_NUKES}}};

void ActionChoice::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                           int width, int height, bool selected, FontType font,
                           TextPrintType flags) const {
  RemapControlType* scheme = Get_Color_Scheme();
  // Tab stops in pixels; the action names hold no tabs, so they only matter to
  // the shared print call.
  static constexpr std::array<int, 2> kTabStops = {13, 40};
  if (font == FontType::k6PointGradient || font == FontType::kEditor) {
    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      view.FillRect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
    } else {
      if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
        flags = flags | TPF_MEDIUM_COLOR;
      }
    }

    Conquer_Clip_Text_Print(view, Description(), x, y, scheme, kTBlack, font,
                            flags, width, kTabStops);
  } else {
    Conquer_Clip_Text_Print(
        view, Description(), x, y,
        selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                 : &ThePalettes().color_remaps().at(PCOLOR_GREY),
        kTBlack, font, flags, width, kTabStops);
  }
}

void TriggerAction::Detach(Target target) {
  if (target.kind() == RTTI_TEAMTYPE && team_type == target.AsTeamType()) {
    team_type = nullptr;
  }
  if (target.kind() == RTTI_TRIGGERTYPE &&
      trigger_type == target.AsTriggerType()) {
    trigger_type = nullptr;
  }
}

std::string TriggerAction::ToIniEntry() const {
  return std::format("{},{},{},{}", std::to_underlying(type),
                     TheObjectHeaps().team_type().PositionOf(team_type),
                     TheObjectHeaps().trigger_type().PositionOf(trigger_type),
                     data.value);
}

void TriggerAction::ReadIni(base::Tokenizer& tokens) {
  switch (TheWorld().new_ini_format()) {
    // Format 2 and later: the team and the trigger are positions in their
    // type lists (ToIniEntry() writes this form).
    default: {
      type = static_cast<TriggerActionType>(
          base::ParseIntegerOr<int>(tokens.Next(), 0));
      team_type = TheObjectHeaps().team_type().SlotOrNull(
          base::ParseIntegerOr<int>(tokens.Next(), 0));
      trigger_type = TheObjectHeaps().trigger_type().SlotOrNull(
          base::ParseIntegerOr<int>(tokens.Next(), 0));
      data.value = base::ParseIntegerOr<int>(tokens.Next(), 0);
      break;
    }

    // The shipped scenarios: the team and the trigger are given by name.
    case 1:
    case 0:
      type = static_cast<TriggerActionType>(
          base::ParseIntegerOr<int>(tokens.Next(), 0));

      team_type = TeamTypeClass::From_Name(tokens.Next());
      DCHECK(ActionNeed(type) != NEED_TEAM || team_type != nullptr);

      // The named trigger may not have been read yet, so keep the name;
      // TriggerTypeClass::Read_INI() resolves it once every trigger exists.
      pending_trigger_name = tokens.Next();

      data.value = base::ParseIntegerOr<int>(tokens.Next(), 0);
      break;
  }

  // Fix corrupted sound values from original game scenario files. Some INI
  // files contain values like -65399 (0xFFFF0089) where only the low byte (137)
  // is the valid VocType index. The original read the value through a one-byte
  // enum; VocType is wider here, so the byte is taken by hand. (The one-byte
  // Data members - Movie, Special, Speech, House - still get it from the
  // union.)
  if (type == TACTION_PLAY_SOUND &&
      (data.value < 0 ||
       std::cmp_greater_equal(data.value, magic_enum::enum_count<VocType>()))) {
    const int low_byte =
        static_cast<int>(static_cast<uint32_t>(data.value) & 0xFFU);
    if (std::cmp_less(low_byte, magic_enum::enum_count<VocType>())) {
      DLOG(WARNING) << "ReadIni: Fixed corrupted sound value " << data.value
                    << " -> " << low_byte;
      data.value = low_byte;
    }
  }
}

// Not const: carries out the action.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool TriggerAction::operator()(HousesType house, ObjectClass* object,
                               int trigger_id, Cell cell) {
  HouseClass* owner = HouseClass::As_Pointer(house);
  TriggerClass* sprung_trigger = nullptr;
  if (trigger_id != -1) {
    sprung_trigger = TheObjectHeaps().trigger().Slot(trigger_id);
  }
  bool success = true;

  // The object may already be dead: a trigger with two actions passes the same
  // object to both, and the first may have killed it.
  if (object && !object->IsActive) {
    object = nullptr;
  }

  switch (type) {
    // Display a line of the mission's tutorial text over the tactical map.
    // An ID with no text displays nothing.
    case TACTION_TEXT_TRIGGER: {
      const char* message = TheAssets().tutorial_text(data.value);
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
    case TACTION_LAUNCH_NUKES: {
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
    case TACTION_PREFERRED_TARGET:
      if (owner) {
        owner->PreferredTarget = data.quarry;
      }
      break;

    // Turn the house's computer base building on or off. When on, the computer
    // builds bases and units. As for every house action here, a house the
    // scenario does not have (or HOUSE_NONE) is left alone.
    case TACTION_BASE_BUILDING:
      if (owner != nullptr) {
        owner->IsBaseBuilding = data.flag;
      }
      break;

    // Cause the shadow to creep back one step.
    case TACTION_CREEP_SHADOW:
      TheMap().Encroach_Shadow();
      break;

    // Set a scenario global.
    case TACTION_SET_GLOBAL:
      TheScenario().Set_Global_To(data.value, true);
      break;

    // Clear a scenario global.
    case TACTION_CLEAR_GLOBAL:
      TheScenario().Set_Global_To(data.value, false);
      break;

    // Reveal the map around the waypoint to the player. Nothing to do once the
    // player sees everything.
    case TACTION_REVEAL_SOME:
      if (!ThePlayer()->IsVisionary) {
        TheMap().Sight_From(base::At(TheScenario().Waypoint, data.value),
                            TheRules().GapShroudRadius, ThePlayer(), false);
      }
      break;

    // Reveal every cell of the movement zone the waypoint is in, which shows
    // whole islands or bodies of water. Zones are compared as the heaviest
    // vehicles see them (MZONE_CRUSHER), where crushable walls do not divide
    // the land.
    case TACTION_REVEAL_ZONE:
      if (!ThePlayer()->IsVisionary) {
        const int zone = TheMap()
                             .at(base::At(TheScenario().Waypoint, data.value))
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
    case TACTION_REVEAL_ALL:
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
    case TACTION_START_TIMER:
      if (!TheScenario().MissionTimer.IsRunning()) {
        TheScenario().MissionTimer.Start();
        TheMap().Redraw_Tab();
      }
      break;

    // Stop the mission timer. This will really just suspend the timer.
    case TACTION_STOP_TIMER:
      if (TheScenario().MissionTimer.IsRunning()) {
        TheScenario().MissionTimer.Stop();
        TheMap().Redraw_Tab();
      }
      break;

    // Add time to the mission timer. The value is in tenths of a minute, as
    // for the two actions below.
    case TACTION_ADD_TIMER:
      TheScenario().MissionTimer.Set(TheScenario().MissionTimer.Remaining() +
                                     (data.value * kTicksPerTimerUnit));
      TheMap().Redraw_Tab();
      break;

    // Remove time from the mission timer, stopping at zero.
    case TACTION_SUB_TIMER:
      if (TheScenario().MissionTimer.Remaining() <=
          data.value * kTicksPerTimerUnit) {
        TheScenario().MissionTimer.Set(0);
      } else {
        TheScenario().MissionTimer.Set(TheScenario().MissionTimer.Remaining() -
                                       (data.value * kTicksPerTimerUnit));
      }
      TheMap().Redraw_Tab();
      break;

    // Set the mission timer to the value specified.
    case TACTION_SET_TIMER:
      TheScenario().MissionTimer.Set(data.value * kTicksPerTimerUnit);
      TheScenario().MissionTimer.Start();
      TheMap().Redraw_Tab();
      break;

    // Play a movie immediately. The game is suspended while it plays; the
    // movie leaves its own palette behind, so the game's is restored and the
    // whole map redrawn.
    case TACTION_PLAY_MOVIE:
      game::TheMouse().Hide();
      TheScreen().visible_view().Clear();
      Play_Movie(data.movie, THEME_NONE, true);
      ThePalettes().game_palette().Set();
      TheMap().Flag_To_Redraw(true);
      game::TheMouse().Show();
      break;

    // Play a sound effect.
    case TACTION_PLAY_SOUND:
      PlaySoundEffect(data.sound);
      break;

    // Play a musical theme.
    case TACTION_PLAY_MUSIC:
      TheTheme().Queue_Song(data.theme);
      break;

    // Play the speech data specified.
    case TACTION_PLAY_SPEECH:
      Speak(data.speech);
      break;

    // Give the special weapon to the house, for one use or for good, and put
    // its button on the player's sidebar.
    case TACTION_1_SPECIAL:
    case TACTION_FULL_SPECIAL:
      if (owner == nullptr) {
        break;
      }
      owner->SuperWeapon.at(data.special)
          .Enable(type == TACTION_1_SPECIAL, false);

      if (ThePlayer() == owner) {
        TheMap().Add(RTTI_SPECIAL, static_cast<int>(data.special));
        TheMap().Column[1].Flag_To_Redraw();
      }
      break;

    // Destroying a trigger means that all triggers of that type will be
    // destroyed. Deleting one closes the gap in the heap's in-use list, so the
    // index is stepped back to look at the trigger that moved into it. The
    // springing trigger may be among them; TriggerClass::Spring() checks for
    // that when this returns.
    case TACTION_DESTROY_TRIGGER:
      if (trigger_type != nullptr) {
        for (int index = 0; index < TheObjectHeaps().trigger().Count();
             index++) {
          if (TheObjectHeaps().trigger().ActiveAt(index)->Class ==
              trigger_type) {
            Detach_This_From_All(
                TheObjectHeaps().trigger().ActiveAt(index)->As_Target());
            base::DeletePooled(TheObjectHeaps().trigger().ActiveAt(index));
            index--;
          }
        }
      }
      break;

    // A forced trigger will force an existing trigger of that type or will
    // create a trigger of that type and then force it to be sprung. Nothing
    // is sprung when the trigger heap is full and none could be made.
    case TACTION_FORCE_TRIGGER:
      if (TriggerClass* forced_trigger = Find_Or_Make(trigger_type);
          forced_trigger != nullptr) {
        forced_trigger->Spring(TEVENT_ANY, nullptr, Cell{}, true);
      }
      break;

    // Place a smoke marker at the waypoint specified.
    case TACTION_DZ:
      base::NewPooled<AnimClass>(
          ANIM_LZ_SMOKE, base::At(TheScenario().Waypoint, data.value).Center());
      break;

    // Flag the house specified as the winner. Only the player is ever flagged:
    // the house says whether the player is the one who won or lost.
    case TACTION_WIN:
      if (data.house == ThePlayer()->Class->House) {
        ThePlayer()->Flag_To_Win();
      } else {
        ThePlayer()->Flag_To_Lose();
      }
      break;

    // Flag the house specified as the loser; the mirror image of the above.
    case TACTION_LOSE:
      if (data.house != ThePlayer()->Class->House) {
        ThePlayer()->Flag_To_Win();
      } else {
        ThePlayer()->Flag_To_Lose();
      }
      break;

    // This will enable production to begin for the house specified.
    case TACTION_BEGIN_PRODUCTION:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data.house);
          specified_house != nullptr) {
        specified_house->Begin_Production();
      }
      break;

    // Cause all buildings to be sold and all units to go into hunt mode. The
    // house's AI does both once it is in the endgame state.
    case TACTION_FIRE_SALE:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data.house);
          specified_house != nullptr) {
        specified_house->State = STATE_ENDGAME;
      }
      break;

    // Begin the team autocreate logic for the house specified.
    case TACTION_AUTOCREATE:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data.house);
          specified_house != nullptr) {
        specified_house->IsAlerted = true;
      }
      break;

    // Manually create the team specified. Counting as scenario setup makes
    // Create_One_Of() ignore the type's limit on how many teams may exist.
    // team_type is null when the INI named a team that does not exist; the
    // three team actions then do nothing.
    case TACTION_CREATE_TEAM:
      if (team_type != nullptr) {
        TheWorld().scenario_init()++;
        team_type->Create_One_Of();
        TheWorld().scenario_init()--;
      }
      break;

    // Destroy all teams of the type specified.
    case TACTION_DESTROY_TEAM:
      if (team_type != nullptr) {
        team_type->Destroy_All_Of();
      }
      break;

    // Create a reinforcement of the team specified.
    case TACTION_REINFORCEMENTS:
      success = team_type != nullptr && DeliverReinforcements(team_type);
      break;

    // Force all units of the house specified to go into hunt mode.
    case TACTION_ALL_HUNT:
      if (HouseClass* specified_house = HouseClass::As_Pointer(data.house);
          specified_house != nullptr) {
        specified_house->Do_All_To_Hunt();
      }
      break;

    // Destroy the object the trigger sprang on, and report failure when there
    // is none (so the trigger is not used up). Take_Damage() takes the damage
    // by reference, hence the locals.
    case TACTION_DESTROY_OBJECT:
      if (object) {
        int damage = object->Strength;
        object->Take_Damage(damage, 0, WARHEAD_AP, nullptr, true);
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
      if (sprung_trigger != nullptr) {
        DestroyObjectsAttachedTo(TheObjectHeaps().unit(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().infantry(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().aircraft(), sprung_trigger);
        DestroyObjectsAttachedTo(TheObjectHeaps().building(), sprung_trigger);
      }
      break;

    // No action. TACTION_ALLOWWIN is not carried out here: TriggerClass and
    // the scenario's win check look for it by type.
    case TACTION_NONE:
    case TriggerActionType::TACTION_WINLOSE:
    case TriggerActionType::TACTION_ALLOWWIN:
    default:
      break;
  }
  return success;
}

const char* ActionName(TriggerActionType action) {
  return kActionNames.at(action);
}

NeedType ActionNeed(TriggerActionType action) {
  switch (action) {
    case TACTION_1_SPECIAL:
    case TACTION_FULL_SPECIAL:
      return NEED_SPECIAL;

    case TACTION_FIRE_SALE:
    case TACTION_WIN:
    case TACTION_LOSE:
    case TACTION_ALL_HUNT:
    case TACTION_BEGIN_PRODUCTION:
    case TACTION_AUTOCREATE:
      return NEED_HOUSE;

    case TACTION_BASE_BUILDING:
      return NEED_BOOL;

    case TACTION_CREATE_TEAM:
    case TACTION_DESTROY_TEAM:
    case TACTION_REINFORCEMENTS:
      return NEED_TEAM;

    case TACTION_FORCE_TRIGGER:
    case TACTION_DESTROY_TRIGGER:
      return NEED_TRIGGER;

    case TACTION_DZ:
    case TACTION_REVEAL_SOME:
    case TACTION_REVEAL_ZONE:
      return NEED_WAYPOINT;

    case TACTION_PLAY_MUSIC:
      return NEED_THEME;

    case TACTION_PLAY_MOVIE:
      return NEED_MOVIE;

    case TACTION_PLAY_SOUND:
      return NEED_SOUND;

    case TACTION_PLAY_SPEECH:
      return NEED_SPEECH;

    case TACTION_TEXT_TRIGGER:
    case TACTION_ADD_TIMER:
    case TACTION_SUB_TIMER:
    case TACTION_SET_TIMER:
    case TACTION_SET_GLOBAL:
    case TACTION_CLEAR_GLOBAL:
      return NEED_NUMBER;

    case TACTION_PREFERRED_TARGET:
      return NEED_QUARRY;

    case TriggerActionType::TACTION_NONE:
    case TriggerActionType::TACTION_WINLOSE:
    case TriggerActionType::TACTION_ALLOWWIN:
    case TriggerActionType::TACTION_REVEAL_ALL:
    case TriggerActionType::TACTION_START_TIMER:
    case TriggerActionType::TACTION_STOP_TIMER:
    case TriggerActionType::TACTION_CREEP_SHADOW:
    case TriggerActionType::TACTION_DESTROY_OBJECT:
    case TriggerActionType::TACTION_LAUNCH_NUKES:
    default:
      break;
  }
  return NEED_NONE;
}

}  // namespace ra
