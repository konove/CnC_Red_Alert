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

// File: Dispatches keyboard commands while the tactical map is displayed.

#include "ra/hotkeys.h"

#include <array>
#include <iterator>
#include <span>
#include <string>
#include <vector>

#include "base/array.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/building.h"
#include "ra/ccptr.h"
#include "ra/chat.h"
#include "ra/config.h"
#include "ra/coord.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/display_constants.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/face.h"
#include "ra/goptions.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/queue.h"
#include "ra/scenario.h"
#include "ra/selection.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/target.h"
#include "ra/text_ids.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "sdllib/keyboard.h"

// Records or restores one of the player's tactical-view bookmarks.
// action: 0 = jump the view back to the remembered location,
//         1 = remember the current view location.
//
// Bookmarks are stored biased by half a screen -- eight rows down and ten
// columns across from the tactical corner -- so the cell recorded is the middle
// of what the player was looking at, not its top left corner.
static void Handle_View(const int view, const int action) {
  if (static_cast<unsigned>(view) < std::ssize(Scen.Views)) {
    if (action == 0) {
      Map.Set_Tactical_Position(Coord_Whole(Cell_Coord(static_cast<CELL>(
          base::At(Scen.Views, view) - (MAP_CELL_W * 8) - 10))));

      // Win95 scrolling logic cant handle just jumps in screen position so
      // redraw the lot.
      Map.Flag_To_Redraw(true);
    } else {
      base::At(Scen.Views, view) = static_cast<CELL>(
          Coord_Cell(Map.TacticalCoord) + (MAP_CELL_W * 8) + 10);
    }
  }
}

// Handles keyboard input while the tactical map is displayed.
//
// Every clause that recognizes a key sets input to KN_NONE, so a key is only
// ever acted on once no matter how many clauses could match it. Message input
// gets first refusal for that reason: a player typing chat must not also be
// commanding their units.
void Keyboard_Process(KeyNumType& input) {
  ObjectClass* obj = nullptr;

  // Don't do anything if there is not keyboard event.
  if (input == KN_NONE) {
    return;
  }
  // For network & modem, process user input for inter-player messages.
  Message_Input(input);

  // The VK_BIT must be stripped from the "plain" value of the key so that a
  // comparison to KN_1, for example, will yield true if in fact the "1"
  // key was pressed.

  constexpr unsigned kModifierBits =
      unsigned{WWKEY_SHIFT_BIT} | unsigned{WWKEY_ALT_BIT} |
      unsigned{WWKEY_CTRL_BIT} | unsigned{WWKEY_VK_BIT};
  const auto plain =
      static_cast<KeyNumType>(static_cast<unsigned>(input) & ~kModifierBits);
  const auto key = static_cast<KeyNumType>(static_cast<unsigned>(input) &
                                           ~unsigned{WWKEY_VK_BIT});

  if constexpr (config::kCheatKeysEnabled) {
    if (Debug_Flag) {
      switch (static_cast<unsigned>(input)) {
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_SHIFT_BIT):
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_ALT_BIT):
        case static_cast<unsigned>(KN_M) | static_cast<unsigned>(KN_CTRL_BIT):
          for (const HousesType house : magic_enum::enum_values<HousesType>()) {
            HouseClass::As_Pointer(house)->Refund_Money(10000);
          }
          break;

        default:
          break;
      }
    }
  }

  if constexpr (config::kCheatKeysEnabled) {
    if (Debug_Playtest &&
        static_cast<unsigned>(input) ==
            (static_cast<unsigned>(KN_W) | static_cast<unsigned>(KN_ALT_BIT))) {
      PlayerPtr->Blockage = 0;
      PlayerPtr->Flag_To_Win();
    }

    if (((Debug_Flag || Debug_Playtest) && plain == KN_F4) &&
        (Session.Type == GAME_NORMAL)) {
      Debug_Unshroud = !Debug_Unshroud;
      Map.Flag_To_Redraw(true);
    }

    if (Debug_Flag && input == KN_SLASH) {
      if (Session.Type != GAME_NORMAL) {
        SpecialDialog = SDLG_SPECIAL;
        input = KN_NONE;
      } else {
        Special_Dialog();
      }
    }
  }

  // Process prerecorded team selection. This will be an additive select
  // if the SHIFT key is held down. It will create the team if the
  // CTRL or ALT key is held down.
  int action = 0;
  if ((static_cast<unsigned>(input) & unsigned{WWKEY_SHIFT_BIT}) != 0U) {
    action = 1;
  }
  if ((static_cast<unsigned>(input) & unsigned{WWKEY_ALT_BIT}) != 0U) {
    action = 3;
  }
  if ((static_cast<unsigned>(input) & unsigned{WWKEY_CTRL_BIT}) != 0U) {
    action = 2;
  }

  // If the "N" key is pressed, then select the next object.
  if (key != 0 && key == Options.KeyNext) {
    if (action) {
      obj = MapEditClass::Prev_Object(
          CurrentObject.Count() ? CurrentObject.at(0) : nullptr);
    } else {
      obj = MapEditClass::Next_Object(
          CurrentObject.Count() ? CurrentObject.at(0) : nullptr);
    }
    if (obj != nullptr) {
      Unselect_All();
      obj->Select();
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
    }
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyPrevious) {
    if (action) {
      obj = MapEditClass::Next_Object(
          CurrentObject.Count() ? CurrentObject.at(0) : nullptr);
    } else {
      obj = MapEditClass::Prev_Object(
          CurrentObject.Count() ? CurrentObject.at(0) : nullptr);
    }
    if (obj != nullptr) {
      Unselect_All();
      obj->Select();
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
    }
    input = KN_NONE;
  }

  // All selected units will go into idle mode.
  if (key != 0 && key == Options.KeyStop) {
    if (CurrentObject.Count()) {
      for (int index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject.at(index);

        if (tech != nullptr &&
            (tech->Can_Player_Move() ||
             (tech->Can_Player_Fire() && tech->What_Am_I() != RTTI_BUILDING))) {
          OutList.Add(EventClass(EventClass::IDLE, TargetClass(tech)));
        }
      }
    }
    input = KN_NONE;
  }

  // All selected units will attempt to go into guard area mode.
  if (key != 0 && key == Options.KeyGuard) {
    if (CurrentObject.Count()) {
      for (int index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject.at(index);

        if (tech != nullptr && tech->Can_Player_Move() &&
            tech->Can_Player_Fire()) {
          OutList.Add(EventClass(TargetClass(tech), MISSION_GUARD_AREA));
        }
      }
    }
    input = KN_NONE;
  }

  // All selected units will attempt to scatter.
  if (key != 0 && key == Options.KeyScatter) {
    if (CurrentObject.Count()) {
      for (int index = 0; index < CurrentObject.Count(); index++) {
        const ObjectClass* tech = CurrentObject.at(index);

        if (tech != nullptr && tech->Can_Player_Move()) {
          OutList.Add(EventClass(EventClass::SCATTER, TargetClass(tech)));
        }
      }
    }
    input = KN_NONE;
  }

  // Center the map around the currently selected objects. If no
  // objects are selected, then fall into the home case.
  if (key != 0 && (key == Options.KeyHome1 || key == Options.KeyHome2)) {
    if (CurrentObject.Count()) {
      Map.Center_Map();
      Map.Flag_To_Redraw(true);
      input = KN_NONE;
    } else {
      input = Options.KeyBase;
    }
  }

  // Center the map about the construction yard or construction vehicle
  // if one is present.
  if (key != 0 && key == Options.KeyBase) {
    Unselect_All();
    if (PlayerPtr->CurBuildings) {
      for (int index = 0; index < Buildings.Count(); index++) {
        BuildingClass* building = Buildings.Ptr(index);

        if (building != nullptr && !building->IsInLimbo &&
            building->House == PlayerPtr && *building == STRUCT_CONST) {
          Unselect_All();
          building->Select();
          if (building->IsLeader) {
            break;
          }
        }
      }
    }
    if (CurrentObject.Count() == 0 && PlayerPtr->CurUnits) {
      for (int index = 0; index < Units.Count(); index++) {
        UnitClass* unit = Units.Ptr(index);

        if (unit != nullptr && !unit->IsInLimbo && unit->House == PlayerPtr &&
            *unit == UNIT_MCV) {
          Unselect_All();
          unit->Select();
          break;
        }
      }
    }
    if (CurrentObject.Count()) {
      Map.Center_Map();
    } else {
      if (PlayerPtr->Center != 0) {
        Map.Center_Map(PlayerPtr->Center);
      }
    }
    Map.Flag_To_Redraw(true);
    input = KN_NONE;
  }

  // Toggle the status of formation for the current team
  if (key != 0 && key == Options.KeyFormation) {
    Toggle_Formation();
    input = KN_NONE;
  }

  // In multiplayer the resign key brings up the surrender dialog. Single
  // player has the mission abort in the options menu instead: surrendering
  // there would only self-destruct the base and lose the mission.
  if (key != 0 && key == Options.KeyResign) {
    if (Session.Type != GAME_NORMAL && !PlayerLoses && !PlayerPtr->IsDefeated) {
      SpecialDialog = SDLG_SURRENDER;
    }
    input = KN_NONE;
  }

  // Handle making and breaking alliances.
  if (key != 0 && key == Options.KeyAlliance) {
    if ((Session.Type != GAME_NORMAL || Debug_Flag) &&
        (CurrentObject.Count() && !PlayerPtr->IsDefeated) &&
        (CurrentObject.at(0)->Owner() != PlayerPtr->Class->House)) {
      OutList.Add(EventClass(EventClass::ALLY,
                             static_cast<int>(CurrentObject.at(0)->Owner())));
    }

    input = KN_NONE;
  }

  // Select all the units on the current display. This is equivalent to
  // drag selecting the whole view.
  if (key != 0 && key == Options.KeySelectView) {
    // The corners are leptons relative to the tactical view, so 0 is its top
    // left and the size below is its full extent -- a drag select of everything
    // on screen.
    Map.Select_These(0x00000000,
                     XY_Coord(Map.TacLeptonWidth, Map.TacLeptonHeight));
    input = KN_NONE;
  }

  // Toggles the repair state similarly to pressing the repair button.
  if (key != 0 && key == Options.KeyRepair) {
    Map.Repair_Mode_Control(-1);
    input = KN_NONE;
  }

  // Toggles the sell state similarly to pressing the sell button.
  if (key != 0 && key == Options.KeySell) {
    Map.Sell_Mode_Control(-1);
    input = KN_NONE;
  }

  // Toggles the map zoom mode similarly to pressing the map button.
  if (key != 0 && key == Options.KeyMap) {
    Map.Zoom_Mode_Control();
    input = KN_NONE;
  }

  // Scrolls the sidebar up one slot.
  if (key != 0 && key == Options.KeySidebarUp) {
    Map.Scroll(true, -1);
    input = KN_NONE;
  }

  // Scrolls the sidebar down one slot.
  if (key != 0 && key == Options.KeySidebarDown) {
    Map.Scroll(false, -1);
    input = KN_NONE;
  }

  // Brings up the options dialog box.
  if (key != 0 && (key == Options.KeyOption1 || key == Options.KeyOption2)) {
    Map.Help_Text(TXT_NONE);  // Turns off help text.
    Queue_Options();
    input = KN_NONE;
  }

  // Scrolls the tactical map in the direction specified.
  int distance = CELL_LEPTON_W;
  if (key != 0 && key == Options.KeyScrollLeft) {
    Map.Scroll_Map(DIR_W, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollRight) {
    Map.Scroll_Map(DIR_E, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollUp) {
    Map.Scroll_Map(DIR_N, distance, true);
    input = KN_NONE;
  }
  if (key != 0 && key == Options.KeyScrollDown) {
    Map.Scroll_Map(DIR_S, distance, true);
    input = KN_NONE;
  }

  // Teams are handled by the 10 special team keys. The manual comparison
  // to the KN numbers is because the Windows keyboard driver can vary
  // the base code number for the key depending on the shift or alt key
  // state!
  if (input != 0 && (plain == Options.KeyTeam1 || plain == KN_1)) {
    Handle_Team(0, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam2 || plain == KN_2)) {
    Handle_Team(1, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam3 || plain == KN_3)) {
    Handle_Team(2, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam4 || plain == KN_4)) {
    Handle_Team(3, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam5 || plain == KN_5)) {
    Handle_Team(4, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam6 || plain == KN_6)) {
    Handle_Team(5, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam7 || plain == KN_7)) {
    Handle_Team(6, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam8 || plain == KN_8)) {
    Handle_Team(7, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam9 || plain == KN_9)) {
    Handle_Team(8, action);
    input = KN_NONE;
  }
  if (input != 0 && (plain == Options.KeyTeam10 || plain == KN_0)) {
    Handle_Team(9, action);
    input = KN_NONE;
  }

  // Handle the bookmark hotkeys.
  if (input != 0 && plain == Options.KeyBookmark1 && !MapEditorActive) {
    Handle_View(0, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark2 && !MapEditorActive) {
    Handle_View(1, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark3 && !MapEditorActive) {
    Handle_View(2, action);
    input = KN_NONE;
  }
  if (input != 0 && plain == Options.KeyBookmark4 && !MapEditorActive) {
    Handle_View(3, action);
    input = KN_NONE;
  }

  if constexpr (config::kCheatKeysEnabled) {
    if (input != 0 && Debug_Flag &&
        (static_cast<unsigned>(input) & static_cast<unsigned>(KN_RLSE_BIT)) ==
            0U) {
      Debug_Key(input);
    }
  }
}

FacingType KN_To_Facing(const unsigned input) {
  constexpr unsigned kModifierBits = static_cast<unsigned>(KN_ALT_BIT) |
                                     static_cast<unsigned>(KN_SHIFT_BIT) |
                                     static_cast<unsigned>(KN_CTRL_BIT);
  // C++17 init-statement: key exists only for the switch.
  switch (const unsigned key = input & ~kModifierBits; key) {
    case KN_LEFT:
      return FACING_W;

    case KN_RIGHT:
      return FACING_E;

    case KN_UP:
      return FACING_N;

    case KN_DOWN:
      return FACING_S;

    case KN_UPLEFT:
      return FACING_NW;

    case KN_UPRIGHT:
      return FACING_NE;

    case KN_DOWNLEFT:
      return FACING_SW;

    case KN_DOWNRIGHT:
      return FACING_SE;

    default:
      break;
  }
  return FACING_NONE;
}
