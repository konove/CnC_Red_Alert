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

/* $Header: /CounterStrike/MAPEDIT.CPP 2     3/13/97 2:05p Steve_tall $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDIT.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : October 20, 1994                         *
 *                                                                         *
 *                  Last Update : February 2, 1995   [BR]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *   Map Editor overloaded routines & utility routines                     *
 *-------------------------------------------------------------------------*
 * Map Editor modules:                                                     *
 * (Yes, they're all one huge class.)                                      *
 *      mapedit.cpp:   overloaded routines, utility routines               *
 *      mapeddlg.cpp:   map editor dialogs, most of the main menu options  *
 *      mapedplc.cpp:   object-placing routines                            *
 *      mapedsel.cpp:   object-selection & manipulation routines           *
 *      mapedtm.cpp:   team-editing routines                               *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::AI -- The map editor's main logic                       *
 *   MapEditClass::Read_INI -- overloaded Read_INI function                *
 *   MapEditClass::AI_Menu -- menu of AI options                           *
 *   MapEditClass::Add_To_List -- adds a TypeClass to the chooseable list  *
 *   MapEditClass::Clear_List -- clears the internal chooseable object list*
 *   MapEditClass::Cycle_House -- finds next valid house for object type   *
 *   MapEditClass::Draw_It -- overloaded Redraw routine                    *
 *   MapEditClass::Fatal -- exits with error message                       *
 *   MapEditClass::Main_Menu -- main menu processor for map editor         *
 *   MapEditClass::MapEditClass -- class constructor                       *
 *   MapEditClass::Mouse_Moved -- checks for mouse motion                  *
 *   MapEditClass::One_Time -- one-time initialization                     *
 *   MapEditClass::Verify_House -- sees if given house can own given obj   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "ra/mapedit.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/debug.h"
#include "ra/defines.h"
#include "ra/dial8.h"
#include "ra/dialog.h"
#include "ra/edit.h"
#include "ra/externs.h"
#include "ra/face.h"
#include "ra/facing.h"
#include "ra/gadget.h"
#include "ra/gauge.h"
#include "ra/globals.h"
#include "ra/hotkeys.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/menus.h"
#include "ra/mission.h"
#include "ra/mouse.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/scenario.h"
#include "ra/selection.h"
#include "ra/session.h"
#include "ra/startup.h"
#include "ra/target.h"
#include "ra/techno.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/txtlabel.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/mix_archive.h"

char MapEditClass::HealthBuf[20];

/***************************************************************************
 * MapEditClass::MapEditClass -- class constructor                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
MapEditClass::MapEditClass() {
  /*
  **	Init data members.
  */
  //	ScenVar = SCEN_VAR_A;

  for (int i = 0; i < kNumEditClasses; i++) {
    base::At(NumType, i) = 0;
    base::At(TypeOffset, i) = 0;
  }
  Scen.Waypoint[ScenarioClass::kHomeWaypoint] = 0;
  CurrentCell = 0;
  CurTeam = nullptr;
  CurTrigger = nullptr;
  Changed = false;
  LMouseDown = false;
  BaseBuilding = false;
  //	BasePercent = 100;
}

/***************************************************************************
 * MapEditClass::One_Time -- one-time initialization                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   02/02/1995 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::One_Time() {
  MouseClass::One_Time();

  /*
  **	The map: a single large "button"
  */
  MapArea = new ControlClass(
      kMapArea, 0, 8, 640 - 8, 400 - 8,
      GadgetClass::kLeftPress | GadgetClass::kLeftRelease, false);

  /*
  **	House buttons
  */
  HouseList = new ListClass(kPopupHouselist, kPopupHouseX, kPopupHouseY,
                            kPopupHouseW, kPopupHouseH, TPF_EFNT | TPF_NOSHADOW,
                            MixArchive::RetrieveData("EBTN-UP.SHP"),
                            MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    HouseList->Add_Item(HouseTypeClass::As_Reference(house).IniName);
  }

  /*
  **	The mission list box
  */
  MissionList =
      new ListClass(kPopupMissionlist, kPopupMissionX, kPopupMissionY,
                    kPopupMissionW, kPopupMissionH, TPF_EFNT | TPF_NOSHADOW,
                    MixArchive::RetrieveData("EBTN-UP.SHP"),
                    MixArchive::RetrieveData("EBTN-DN.SHP"));

  for (const auto mission : MapEditMissions) {
    MissionList->Add_Item(MissionClass::Mission_Name(mission));
  }

  /*
  **	The health bar
  */
  HealthGauge =
      new TriColorGaugeClass(kPopupHealthgauge, kPopupHealthX, kPopupHealthY,
                             kPopupHealthW, kPopupHealthH);
  HealthGauge->Use_Thumb(true);
  HealthGauge->Set_Maximum(0x100);
  HealthGauge->Set_Red_Limit(0x3f - 1);
  HealthGauge->Set_Yellow_Limit(0x7f - 1);

  /*
  **	The health text label
  */
  HealthBuf[0] = 0;
  HealthText = new TextLabelClass(
      HealthBuf, kPopupHealthX + (kPopupHealthW / 2),
      kPopupHealthY + kPopupHealthH + 1, GadgetClass::Get_Color_Scheme(),
      TPF_CENTER | TPF_FULLSHADOW | TPF_EFNT);

  /*
  **	Building attribute buttons.
  */
  Sellable = new TextButtonClass(kPopupSellable, TXT_SELLABLE, kTpfEButton,
                                 320 - 65, 200 - 25, 60);
  Rebuildable = new TextButtonClass(kPopupRebuildable, TXT_REBUILD, kTpfEButton,
                                    320 - 65, 200 - 15, 60);

  /*
  **	The facing dial
  */
  FacingDial =
      new Dial8Class(kPopupFacingdial, kPopupFaceboxX, kPopupFaceboxY,
                     kPopupFaceboxW, kPopupFaceboxH, static_cast<DirType>(0));

  /*
  **	The base percent-built slider & its label
  */
  BaseGauge = new GaugeClass(kPopupBasepercent, kPopupBaseX, kPopupBaseY,
                             kPopupBaseW, kPopupBaseH);
  // TextLabelClass keeps the pointer in its non-const Text member, so the
  // caption needs storage that outlives this call and is not a literal.
  static char base_caption[] = "Base:";
  BaseLabel = new TextLabelClass(base_caption, kPopupBaseX - 3, kPopupBaseY,
                                 GadgetClass::Get_Color_Scheme(),
                                 TPF_RIGHT | TPF_NOSHADOW | TPF_EFNT);
  BaseGauge->Set_Maximum(100);
  BaseGauge->Set_Value(Scen.Percent);
}

/***********************************************************************************************
 * MapeditClass::Init_IO -- Reinitializes the radar map at scenario start. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
void MapEditClass::Init_IO() {
  /*
  **	For normal game mode, jump to the parent's Init routine.
  */
  if (!MapEditorActive) {
    MouseClass::Init_IO();

  } else {
    /*
    **	For editor mode, add the map area to the button input list
    */
    Buttons = nullptr;
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
  }
}

/***************************************************************************
 * MapEditClass::Clear_List -- clears the internal chooseable object list   *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Clear_List() {
  /*
  **	Set # object type ptrs to 0, set NumType for each type to 0
  */
  ObjCount = 0;
  for (int& i : NumType) {
    i = 0;
  }
}

/***************************************************************************
 * MapEditClass::Add_To_List -- adds a TypeClass to the chooseable list     *
 *                                                                         *
 * Use this routine to add an object to the game object selection list.    *
 * This list is used by the Add_Object function. All items located in the  *
 * list will appear and be chooseable by that function. Make sure to       *
 * clear the list before adding a sequence of items to it. Clearing        *
 * the list is accomplished by the Clear_List() function.                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      object      ptr to ObjectTypeClass to add                          *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      bool: was the object added to the list?  A failure could occur if  *
 *      NULL were passed in or the list is full.                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/04/1994 JLB : Created.                                             *
 *=========================================================================*/
bool MapEditClass::Add_To_List(const ObjectTypeClass* object) {
  /*
  **	Add the object if there's room.
  */
  if (object && ObjCount < kMaxEditObjects) {
    base::At(Objects, ObjCount++) = object;

    /*
    **	Update type counters.
    */
    switch (object->What_Am_I()) {
      case RTTI_TEMPLATETYPE:
        NumType[0]++;
        break;

      case RTTI_OVERLAYTYPE:
        NumType[1]++;
        break;

      case RTTI_SMUDGETYPE:
        NumType[2]++;
        break;

      case RTTI_TERRAINTYPE:
        NumType[3]++;
        break;

      case RTTI_UNITTYPE:
        NumType[4]++;
        break;

      case RTTI_INFANTRYTYPE:
        NumType[5]++;
        break;

      case RTTI_VESSELTYPE:
        NumType[6]++;
        break;

      case RTTI_BUILDINGTYPE:
        NumType[7]++;
        break;

      case RTTI_AIRCRAFTTYPE:
        NumType[8]++;
        break;
      case RTTIType::RTTI_NONE:
      case RTTIType::RTTI_AIRCRAFT:
      case RTTIType::RTTI_ANIM:
      case RTTIType::RTTI_ANIMTYPE:
      case RTTIType::RTTI_BUILDING:
      case RTTIType::RTTI_BULLET:
      case RTTIType::RTTI_BULLETTYPE:
      case RTTIType::RTTI_CELL:
      case RTTIType::RTTI_FACTORY:
      case RTTIType::RTTI_HOUSE:
      case RTTIType::RTTI_HOUSETYPE:
      case RTTIType::RTTI_INFANTRY:
      case RTTIType::RTTI_OVERLAY:
      case RTTIType::RTTI_SMUDGE:
      case RTTIType::RTTI_SPECIAL:
      case RTTIType::RTTI_TEAM:
      case RTTIType::RTTI_TEAMTYPE:
      case RTTIType::RTTI_TEMPLATE:
      case RTTIType::RTTI_TERRAIN:
      case RTTIType::RTTI_TRIGGER:
      case RTTIType::RTTI_TRIGGERTYPE:
      case RTTIType::RTTI_UNIT:
      case RTTIType::RTTI_VESSEL:
      default:
        break;
    }
    return true;
  }

  return false;
}

/***************************************************************************
 * MapEditClass::AI -- The map editor's main logic                         *
 *                                                                         *
 * This routine overloads the parent's (DisplayClass) AI function.         *
 * It checks for any input specific to map editing, and calls the parent   *
 * AI routine to handle scrolling and other mainstream map stuff.          *
 *                                                                         *
 * If this detects one of its special input keys, it sets 'input' to 0     *
 * before calling the parent AI routine; this prevents input conflict.     *
 *                                                                         *
 * SUPPORTED INPUT:                                                        *
 * General:                                                                *
 *      F2/RMOUSE:            main menu                                    *
 *      F6:                  toggles show-passable mode                    *
 *      HOME:                  go to the Home Cell (scenario's start position)*
 *      SHIFT-HOME:            set the Home Cell to the current TacticalCell*
 *      ESC:                  exits to DOS                                 *
 * Object Placement:                                                       *
 *      INSERT:               go into placement mode                       *
 *      ESC:                  exit placement mode                          *
 *      LEFT/RIGHT:          prev/next placement object                    *
 *      PGUP/PGDN:            prev/next placement category                 *
 *      HOME:                  1st placement object (clear template)       *
 *      h/H:                  toggle house of placement object             *
 *      LMOUSE:               place the placement object                   *
 *      MOUSE MOTION:         "paint" with the placement object            *
 * Object selection:                                                       *
 *      LMOUSE:               select & "grab" current object               *
 *                           If no object is present where the mouse is    *
 *                           clicked, the current object is de-selected    *
 *                           If the same object is clicked on, it stays    *
 *                           selected. Also displays the object-editing    *
 *                           gadgets.                                      *
 *      LMOUSE RLSE:         release currently-grabbed object              *
 *      MOUSE MOTION:         if an object is grabbed, moves the object    *
 *      SHIFT|ALT|ARROW:      moves object in that direction               *
 *      DELETE               deletes currently-selected object             *
 * Object-editing controls:                                                *
 *      POPUP_GDI:            makes GDI the owner of this object           *
 *      POPUP_NOD:            makes NOD the owner of this object           *
 *      kPopupMissionlist:   sets that mission for this object             *
 *      kPopupHealthgauge:   sets that health value for this object        *
 *      kPopupFacingdial:      sets the object's facing                    *
 *                                                                         *
 * Changed is set when you:                                                *
 *      - place an object                                                  *
 *      - move a grabbed object                                            *
 *      - delete an object                                                 *
 *      - size the map                                                     *
 *      - create a new scenario                                            *
 *   Changed is cleared when you:                                          *
 *      - Save the scenario                                                *
 *      - Load a scenario                                                  *
 *      - Play the scenario                                                *
 *                                                                         *
 * INPUT:                                                                  *
 *      input      KN_ value, 0 if none                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::AI(KeyNumType& input, int x, int y) {
  int rc = 0;
  CELL cell = 0;
  int found = 0;  // for removing a waypoint label
  HousesType house = HOUSE_NONE;
  char wayname[4];

  /*
  **	Trap 'F2' regardless of whether we're in game or editor mode
  */
  if (Debug_Flag && ((input == KN_F2 && Session.Type == GAME_NORMAL) ||
                     input == (KN_F2 | KN_CTRL_BIT))) {
    ScenarioInit = 0;

    /*
    ** If we're in editor mode & Changed is set, prompt for saving changes
    */
    if (MapEditorActive && Changed) {
      rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
      HidPage.Clear();
      Flag_To_Redraw(true);
      Render();

      /*
      **	User wants to save
      */
      if (rc == 0) {
        /*
        **	If save cancelled, abort game
        */
        if (Save_Scenario() != 0) {
          input = KN_NONE;
        } else {
          Changed = false;
          Go_Editor(!MapEditorActive);
        }
      } else {
        /*
        **	User doesn't want to save
        */
        Go_Editor(!MapEditorActive);
      }
    } else {
      /*
      ** If we're in game mode, set Changed to 0 (so if we didn't save our
      ** changes above, they won't keep coming back to haunt us with continual
      ** Save Changes? prompts!)
      */
      if (!MapEditorActive) {
        Changed = false;
      }
      BaseGauge->Set_Value(Scen.Percent);
      Go_Editor(!MapEditorActive);
    }
  }

  /*
  **	For normal game mode, jump to the parent's AI routine.
  */
  if (!MapEditorActive) {
    MouseClass::AI(input, x, y);
    return;
  }

  ::Frame++;

  /*
  **	Do special mouse processing if the mouse is over the map
  */
  if (Get_Mouse_X() > TacPixelX &&
      Get_Mouse_X() < TacPixelX + Lepton_To_Pixel(TacLeptonWidth) &&
      Get_Mouse_Y() > TacPixelY &&
      Get_Mouse_Y() < TacPixelY + Lepton_To_Pixel(TacLeptonHeight)) {
    /*
    **	When the mouse moves over a scrolling edge, ScrollClass changes its
    **	shape to the appropriate arrow or NO symbol; it's our job to change it
    **	back to normal (or whatever the shape is set to by Set_Default_Mouse())
    **	when it re-enters the map area.
    */
    if (CurTrigger) {
      Override_Mouse_Shape(MOUSE_CAN_MOVE);
    } else {
      Override_Mouse_Shape(MOUSE_NORMAL);
    }
  }

  /*
  **	Set 'ZoneCell' to track the mouse cursor around over the map.  Do this
  **	even if the map is scrolling.
  */
  if (Get_Mouse_X() >= TacPixelX &&
      Get_Mouse_X() <= TacPixelX + Lepton_To_Pixel(TacLeptonWidth) &&
      Get_Mouse_Y() >= TacPixelY &&
      Get_Mouse_Y() <= TacPixelY + Lepton_To_Pixel(TacLeptonHeight)) {
    cell = Click_Cell_Calc(Get_Mouse_X(), Get_Mouse_Y());
    if (cell != -1) {
      Set_Cursor_Pos(cell);
      if (PendingObject) {
        Flag_To_Redraw(true);
      }
    }
  }

  /*
  **	Check for mouse motion while left button is down.
  */
  const bool moved = Mouse_Moved();
  if (LMouseDown && moved) {
    /*
    **	"Paint" mode: place current object, and restart placement
    */
    if (PendingObject) {
      Flag_To_Redraw(true);
      if (Place_Object() == 0) {
        Changed = true;
        Start_Placement();
      }
    } else {
      /*
      **	Move the currently-grabbed object
      */
      if (GrabbedObject) {
        GrabbedObject->Mark(MARK_CHANGE);
        if (Move_Grabbed_Object() == 0) {
          Changed = true;
        }
      }
    }
  }

  /*
  **	Trap special editing keys; if one is detected, set 'input' to 0 to
  **	prevent a conflict with parent's AI().
  */
  switch (static_cast<int>(input)) {
    /*
    ** F2/RMOUSE = pop up main menu
    */
    case KN_RMOUSE:

      /*
      **	Turn off placement mode
      */
      if (PendingObject) {
        if (BaseBuilding) {
          Cancel_Base_Building();
        } else {
          Cancel_Placement();
        }
      }

      /*
      **	Turn off trigger placement mode
      */
      if (CurTrigger) {
        Stop_Trigger_Placement();
      }

      /*
      **	Unselect object & hide popup controls
      */
      if (CurrentObject.Count()) {
        CurrentObject.at(0)->Unselect();
        Popup_Controls();
      }
      Main_Menu();
      input = KN_NONE;
      break;

    /*
    **	F6 = toggle passable/impassable display
    */
    case KN_F6:
      Debug_Passable = !Debug_Passable;
      HidPage.Clear();
      Flag_To_Redraw(true);
      input = KN_NONE;
      break;

    /*
    **	INSERT = go into object-placement mode
    */
    case KN_INSERT:
      if (!PendingObject) {
        /*
        **	Unselect current object, hide popup controls
        */
        if (CurrentObject.Count()) {
          CurrentObject.at(0)->Unselect();
          Popup_Controls();
        }

        /*
        **	Go into placement mode
        */
        Start_Placement();
      }
      input = KN_NONE;
      break;

    /*
    **	ESC = exit placement mode, or exit to DOS
    */
    case KN_ESC:

      /*
      **	Exit object placement mode
      */
      if (PendingObject) {
        if (BaseBuilding) {
          Cancel_Base_Building();
        } else {
          Cancel_Placement();
        }
        input = KN_NONE;
        break;
      }
      /*
      **	Exit trigger placement mode
      */
      if (CurTrigger) {
        Stop_Trigger_Placement();
        input = KN_NONE;
        break;
      }
      rc = WWMessageBox().Process("Exit Scenario Editor?", TXT_YES, TXT_NO);
      HidPage.Clear();
      Flag_To_Redraw(true);
      Render();

      /*
      **	User doesn't want to exit; return to editor
      */
      if (rc == 1) {
        input = KN_NONE;
        break;
      }

      /*
      **	If changed, prompt for saving
      */
      if (Changed) {
        rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
        HidPage.Clear();
        Flag_To_Redraw(true);
        Render();

        /*
        **	User wants to save
        */
        if (rc == 0) {
          /*
          **	If save cancelled, abort exit
          */
          if (Save_Scenario() != 0) {
            input = KN_NONE;
            break;
          }
          Changed = false;
        }
      }
      // Prog_End();
      Emergency_Exit(0);

    /*
    **	LEFT = go to previous placement object
    */
    case KN_LEFT:
      if (PendingObject) {
        Place_Prev();
      }
      input = KN_NONE;
      break;

    /*
    **	RIGHT = go to next placement object
    */
    case KN_RIGHT:
      if (PendingObject) {
        Place_Next();
      }
      input = KN_NONE;
      break;

    /*
    **	PGUP = go to previous placement category
    */
    case KN_PGUP:
      if (PendingObject) {
        Place_Prev_Category();
      }
      input = KN_NONE;
      break;

    /*
    **	PGDN = go to next placement category
    */
    case KN_PGDN:
      if (PendingObject) {
        Place_Next_Category();
      }
      input = KN_NONE;
      break;

    /*
    **	HOME = jump to first placement object, or go to Home Cell
    */
    case KN_HOME:
      if (PendingObject) {
        Place_Home();
      } else {
        /*
        **	Set map position
        */
        ScenarioInit++;
        Set_Tactical_Position(
            Cell_Coord(Scen.Waypoint[ScenarioClass::kHomeWaypoint]));
        ScenarioInit--;

        /*
        **	Force map to redraw
        */
        HidPage.Clear();
        Flag_To_Redraw(true);
        Render();
      }
      input = KN_NONE;
      break;

    /*
    **	SHIFT-HOME: set new Home Cell position
    */
    case (KN_HOME | KN_SHIFT_BIT):
      if (CurrentCell != 0) {
        /*
        ** Unflag the old Home Cell, if there are no other waypoints
        ** pointing to it
        */
        cell = Scen.Waypoint[ScenarioClass::kHomeWaypoint];

        if (cell != -1) {
          found = 0;
          for (int i = 0; i < ScenarioClass::kWaypointCount; i++) {
            if (i != ScenarioClass::kHomeWaypoint &&
                base::At(Scen.Waypoint, i) == cell) {
              found = 1;
            }
          }

          if (found == 0) {
            (*this).at(cell).IsWaypoint = false;
            Flag_Cell(cell);
          }
        }

        /*
        ** Now set the new Home cell
        */
        //			Scen.Waypoint[ScenarioClass::kHomeWaypoint] =
        // Coord_Cell(TacticalCoord);
        //			(*this)[TacticalCoord].IsWaypoint = 1;
        //			Flag_Cell(Coord_Cell(TacticalCoord));
        Scen.Waypoint[ScenarioClass::kHomeWaypoint] = CurrentCell;
        (*this).at(CurrentCell).IsWaypoint = true;
        Flag_Cell(CurrentCell);

        Changed = true;
        input = KN_NONE;
      }
      break;

    /*
    **	SHIFT-R: set new Reinforcement Cell position.  Don't allow setting
    **	the Reinf. Cell to the same as the Home Cell (for display purposes.)
    */
    case (KN_R | KN_SHIFT_BIT):
      if (CurrentCell == 0 ||
          CurrentCell == Scen.Waypoint[ScenarioClass::kHomeWaypoint]) {
        break;
      }

      /*
      ** Unflag the old Reinforcement Cell, if there are no other waypoints
      ** pointing to it
      */
      cell = Scen.Waypoint[ScenarioClass::kReinforcementWaypoint];

      if (cell != -1) {
        found = 0;
        for (int i = 0; i < ScenarioClass::kWaypointCount; i++) {
          if (i != ScenarioClass::kReinforcementWaypoint &&
              base::At(Scen.Waypoint, i) == cell) {
            found = 1;
          }
        }

        if (found == 0) {
          (*this).at(cell).IsWaypoint = false;
          Flag_Cell(cell);
        }
      }
      /*
      ** Now set the new Reinforcement cell
      */
      Scen.Waypoint[ScenarioClass::kReinforcementWaypoint] = CurrentCell;
      (*this).at(CurrentCell).IsWaypoint = true;
      Flag_Cell(CurrentCell);
      Changed = true;
      input = KN_NONE;
      break;

    /*
    **	ALT-Letter: Label a waypoint cell
    */
    case (KN_A | KN_ALT_BIT):
    case (KN_B | KN_ALT_BIT):
    case (KN_C | KN_ALT_BIT):
    case (KN_D | KN_ALT_BIT):
    case (KN_E | KN_ALT_BIT):
    case (KN_F | KN_ALT_BIT):
    case (KN_G | KN_ALT_BIT):
    case (KN_H | KN_ALT_BIT):
    case (KN_I | KN_ALT_BIT):
    case (KN_J | KN_ALT_BIT):
    case (KN_K | KN_ALT_BIT):
    case (KN_L | KN_ALT_BIT):
    case (KN_M | KN_ALT_BIT):
    case (KN_N | KN_ALT_BIT):
    case (KN_O | KN_ALT_BIT):
    case (KN_P | KN_ALT_BIT):
    case (KN_Q | KN_ALT_BIT):
    case (KN_R | KN_ALT_BIT):
    case (KN_S | KN_ALT_BIT):
    case (KN_T | KN_ALT_BIT):
    case (KN_U | KN_ALT_BIT):
    case (KN_V | KN_ALT_BIT):
    case (KN_W | KN_ALT_BIT):
    case (KN_X | KN_ALT_BIT):
    case (KN_Y | KN_ALT_BIT):
    case (KN_Z | KN_ALT_BIT):
      if (CurrentCell != 0) {
        const int waypt_idx =
            (input & ~KN_ALT_BIT) - KN_A;  // for labelling a waypoint
        Update_Waypoint(waypt_idx);
      }
      input = KN_NONE;
      break;

    /*
    ** ALT-. : Designate an extended (2-letter) waypoint name
    */
    case KN_PERIOD:
    case (KN_PERIOD | KN_ALT_BIT):
      if (CurrentCell != 0 && Get_Waypoint_Name(wayname)) {
        int waynm = 0;
        if (!std::string_view(wayname).empty()) {
          wayname[0] = static_cast<char>(toupper(wayname[0]));
          wayname[1] = static_cast<char>(toupper(wayname[1]));
          if (wayname[0] >= 'A' && wayname[0] <= 'Z') {
            waynm = wayname[0] - 'A';
            if (wayname[1] >= 'A' && wayname[1] <= 'Z') {
              waynm = ((waynm + 1) * 26) + (wayname[1] - 'A');
            }
            if (waynm < ScenarioClass::kHomeWaypoint) {
              Update_Waypoint(waynm);
            }
          }
        }
      }
      input = KN_NONE;
      break;

    /*
    **	ALT-Space: Remove a waypoint designation
    */
    case (KN_SPACE | KN_ALT_BIT):
      if (CurrentCell != 0) {
        /*
        **	Loop through letter waypoints; if this cell is one of them,
        **	clear that waypoint.
        */
        for (int i = 0; i < ScenarioClass::kHomeWaypoint; i++) {
          if (base::At(Scen.Waypoint, i) == CurrentCell) {
            base::At(Scen.Waypoint, i) = -1;
          }
        }

        /*
        **	Loop through flag home values; if this cell is one of them,
        *clear *	that waypoint.
        */
        for (int i = 0; i < kMaxPlayers; i++) {
          house = static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + i);
          if (HouseClass::As_Pointer(house) &&
              CurrentCell == HouseClass::As_Pointer(house)->FlagHome) {
            HouseClass::As_Pointer(house)->Flag_Remove(As_Target(CurrentCell),
                                                       true);
          }
        }

        /*
        **	If there are no more waypoints on this cell, clear the cell's
        **	waypoint designation.
        */
        if (Scen.Waypoint[ScenarioClass::kHomeWaypoint] != CurrentCell &&
            Scen.Waypoint[ScenarioClass::kReinforcementWaypoint] !=
                CurrentCell) {
          (*this).at(CurrentCell).IsWaypoint = false;
        }
        Changed = true;
        Flag_Cell(CurrentCell);
      }
      input = KN_NONE;
      break;

    /*
    **	'H' = toggle current placement object's house
    */
    case KN_H:
    case (KN_H | KN_SHIFT_BIT):
      if (PendingObject) {
        Toggle_House();
      }
      input = KN_NONE;
      break;

    /*
    **	Left-mouse click:
    **	Button DOWN:
    **	- Toggle LMouseDown
    **	- If we're in placement mode, try to place the current object
    **	- If success, re-enter placement mode
    **	- Otherwise, try to select an object, and "grab" it if there is one
    **	- If no object, then select that cell as the "current" cell
    **	Button UP:
    **	- Toggle LMouseDown
    **	- release any grabbed object
    */
    case ButtonKey(kMapArea):

      /*
      **	Left Button DOWN
      */
      if (KeyboardClass::Down(KN_LMOUSE)) {
        LMouseDown = true;

        /*
        **	Placement mode: place an object
        */
        if (PendingObject) {
          if (Place_Object() == 0) {
            Changed = true;
            Start_Placement();
          }
        } else {
          /*
          **	Place a trigger
          */
          if (CurTrigger) {
            Place_Trigger();
            Changed = true;
          } else {
            /*
            **	Select an object or a cell
            **	Check for double-click
            */
            if (CurrentObject.Count() &&
                ((TickCount.Value() - LastClickTime) < 15)) {
            } else {
              /*
              **	Single-click: select object
              */
              if (Select_Object() == 0) {
                CurrentCell = 0;
                Grab_Object();
              } else {
                /*
                **	No object: select the cell
                */
                CurrentCell =
                    Click_Cell_Calc(Keyboard->MouseQX, Keyboard->MouseQY);
                HidPage.Clear();
                Flag_To_Redraw(true);
                Render();
              }
            }
          }
        }
        LastClickTime = TickCount.Value();
        input = KN_NONE;
      } else {
        /*
        **	Left Button UP
        */
        LMouseDown = false;
        GrabbedObject = nullptr;
        input = KN_NONE;
      }
      break;

    /*
    **	SHIFT-ALT-Arrow: move the current object
    */
    case KN_UP | KN_ALT_BIT | KN_SHIFT_BIT:
    case KN_DOWN | KN_ALT_BIT | KN_SHIFT_BIT:
    case KN_LEFT | KN_ALT_BIT | KN_SHIFT_BIT:
    case KN_RIGHT | KN_ALT_BIT | KN_SHIFT_BIT:
      if (CurrentObject.Count()) {
        CurrentObject.at(0)->Move(KN_To_Facing(input));
        Changed = true;
      }
      input = KN_NONE;
      break;

    /*
    **	DELETE: delete currently-selected object
    */
    case KN_DELETE:

      /*
      **	Delete currently-selected object's trigger, or the object
      */
      if (CurrentObject.Count()) {
        /*
        **	Delete trigger
        */
        if (CurrentObject.at(0)->Trigger.Is_Valid()) {
          CurrentObject.at(0)->Trigger = nullptr;
        } else {
          /*
          ** If the current object is part of the AI's Base, remove it
          ** from the Base's Node list.
          */
          if (CurrentObject.at(0)->What_Am_I() == RTTI_BUILDING) {
            const auto* building =
                dynamic_cast<const BuildingClass*>(CurrentObject.at(0));
            if (Base.Is_Node(building)) {
              BaseNodeClass* node =
                  Base.Get_Node(building);  // for removing from an AI Base
              Base.Nodes.Delete(*node);
            }
          }

          /*
          **	Delete current object
          */
          delete CurrentObject.at(0);

          /*
          **	Hide the popup controls
          */
          Popup_Controls();
        }

        /*
        **	Force a redraw
        */
        HidPage.Clear();
        Flag_To_Redraw(true);
        Changed = true;
      } else {
        /*
        **	Remove trigger from current cell
        */
        if (CurrentCell && (*this).at(CurrentCell).Trigger.Is_Valid()) {
          (*this).at(CurrentCell).Trigger = nullptr;
          //						CellTriggers[CurrentCell]
          //= NULL;

          /*
          **	Force a redraw
          */
          HidPage.Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }
      }
      input = KN_NONE;
      break;

    /*
    **	TAB: select next object on the map
    */
    case KN_TAB:
      Select_Next();
      input = KN_NONE;
      break;

    /*
    **	Object-Editing button: House Button
    */
    case ButtonKey(kPopupHouselist):
      /*
      **	Determine the house desired by examining the currently
      **	selected index in the house list gadget.
      */
      house = HousesType(dynamic_cast<const ListClass*>(
                             Buttons->Extract_Gadget(kPopupHouselist))
                             ->Current_Index());

      /*
      **	If that house doesn't own this object, try to transfer it
      */
      if ((CurrentObject.at(0)->Owner() != house) && Change_House(house)) {
        Changed = true;
      }

      //			Set_House_Buttons(CurrentObject[0]->Owner(),
      // Buttons, kPopupFirst);
      HidPage.Clear();
      Buttons->Flag_List_To_Redraw();
      Flag_To_Redraw(true);
      input = KN_NONE;
      break;

      //		case (POPUP_GDI | KN_BUTTON):
      //		case (POPUP_NOD | KN_BUTTON):
      //		case (POPUP_NEUTRAL | KN_BUTTON):
      //		case (POPUP_MULTI1 | KN_BUTTON):
      //		case (POPUP_MULTI2 | KN_BUTTON):
      //		case (POPUP_MULTI3 | KN_BUTTON):
      //		case (POPUP_MULTI4 | KN_BUTTON):
      //
      //			/*
      //			**	Convert input value into a house value;
      // assume HOUSE_GOOD is 0
      //			*/
      //			house = (HousesType)( (input & (~KN_BUTTON)) -
      // kPopupFirst);
      //
      //			/*
      //			**	If that house doesn't own this object,
      // try to transfer it
      //			*/
      //			if (CurrentObject[0]->Owner()!=house) {
      //				if (Change_House(house)) {
      //					Changed = 1;
      //				}
      //			}
      //			Set_House_Buttons(CurrentObject[0]->Owner(),
      // Buttons, kPopupFirst); 			HidPage.Clear();
      // Flag_To_Redraw(true); 			input = KN_NONE;
      // break;

    case ButtonKey(kPopupSellable):
      if (CurrentObject.at(0)->What_Am_I() == RTTI_BUILDING) {
        auto* building = dynamic_cast<BuildingClass*>(CurrentObject.at(0));

        if (building->Class->Level != -1) {
          //				if (building->Class->IsBuildable) {
          building->IsAllowedToSell =
              !static_cast<bool>(building->IsAllowedToSell);
          building->Mark(MARK_CHANGE);
        }
        if (building->IsAllowedToSell) {
          Sellable->Turn_On();
        } else {
          Sellable->Turn_Off();
        }
      }
      break;

    case ButtonKey(kPopupRebuildable):
      if (CurrentObject.at(0)->What_Am_I() == RTTI_BUILDING) {
        auto* building = dynamic_cast<BuildingClass*>(CurrentObject.at(0));

        if (building->Class->Level != -1) {
          //				if (building->Class->IsBuildable) {
          building->IsToRebuild = !static_cast<bool>(building->IsToRebuild);
          building->Mark(MARK_CHANGE);
        }
        if (building->IsToRebuild) {
          Rebuildable->Turn_On();
        } else {
          Rebuildable->Turn_Off();
        }
      }
      break;

    /*
    **	Object-Editing button: Mission
    */
    case ButtonKey(kPopupMissionlist):
      if (CurrentObject.at(0)->Is_Techno()) {
        /*
        **	Set new mission
        */
        const MissionType mission =
            MapEditMissions.at(base::ToSize(MissionList->Current_Index()));
        if (CurrentObject.at(0)->Get_Mission() != mission) {
          dynamic_cast<TechnoClass*>(CurrentObject.at(0))->Set_Mission(mission);
          Changed = true;
          Buttons->Flag_List_To_Redraw();
          Flag_To_Redraw(true);
        }
      }
      input = KN_NONE;
      break;

    /*
    **	Object-Editing button: Health
    */
    case ButtonKey(kPopupHealthgauge):
      if (CurrentObject.at(0)->Is_Techno()) {
        /*
        **	Derive strength from current gauge reading
        */
        int strength = CurrentObject.at(0)->Class_Of().MaxStrength *
                       fixed(HealthGauge->Get_Value(), 256);
        //				strength =
        // Fixed_To_Cardinal((unsigned)CurrentObject[0]->Class_Of().MaxStrength,
        //(unsigned)HealthGauge->Get_Value());

        /*
        **	Clip to 1
        */
        if (strength <= 0) {
          strength = 1;
        }

        /*
        **	Set new strength
        */
        if (strength != CurrentObject.at(0)->Strength) {
          CurrentObject.at(0)->Strength = static_cast<int16_t>(strength);
          HidPage.Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }

        /*
        **	Update text label
        */
        absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d", strength);
      }
      input = KN_NONE;
      break;

    /*
    **	Object-Editing button: Facing
    */
    case ButtonKey(kPopupFacingdial):
      if (CurrentObject.at(0)->Is_Techno()) {
        auto* techno = dynamic_cast<TechnoClass*>(CurrentObject.at(0));
        if (FacingDial->direction() != techno->PrimaryFacing.Get()) {
          /*
          **	Set body's facing
          */
          techno->PrimaryFacing.Set(FacingDial->direction());

          /*
          **	Set turret facing, if there is one
          */
          if (techno->What_Am_I() == RTTI_UNIT) {
            dynamic_cast<UnitClass&>(*techno).SecondaryFacing.Set(
                FacingDial->direction());
          }

          HidPage.Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }
      }

      input = KN_NONE;
      break;

    /*
    **	Object-Editing button: Facing
    */
    case ButtonKey(kPopupBasepercent):
      if (BaseGauge->Get_Value() != Scen.Percent) {
        Scen.Percent = BaseGauge->Get_Value();
        Build_Base_To(Scen.Percent);
        HidPage.Clear();
        Flag_To_Redraw(true);
      }
      input = KN_NONE;
      break;

    default:
      break;
  }

  /*
  **	Call parent's AI routine
  */
  MouseClass::AI(input, x, y);
}

/***************************************************************************
 * MapEditClass::Draw_It -- overloaded Redraw routine                      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/17/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Draw_It(bool forced) {
  char buf[40];

  MouseClass::Draw_It(forced);

  if (!MapEditorActive) {
    return;
  }

  /*
  **	Display the total value of all Tiberium on the map.
  */
  Fancy_Text_Print("Tiberium=%ld   ", 0, 0, GadgetClass::Get_Color_Scheme(),
                   kBlack, TPF_EFNT | TPF_NOSHADOW, TotalValue);

  /*
  **	If there are no object controls displayed, just invoke parent's Redraw
  **	and return.
  */
  if (!Buttons) {
    return;
  }

  /*
  **	Otherwise, if 'display' is set, invoke the parent's Redraw to refresh
  **	the HIDPAGE; then, update the buttons & text labels onto HIDPAGE;
  **	then invoke the parent's Redraw to blit the HIDPAGE to SEENPAGE.
  */
  /*
  **	Update the text labels
  */
  if (forced && CurrentObject.Count()) {
    /*
    **	Display the object's name & ID
    */
    const char* label = Text_String(CurrentObject.at(0)->Full_Name());
    const char* tptr = label;
    absl::SNPrintF(buf, sizeof(buf), "%s (%d)", tptr,
                   CurrentObject.at(0)->As_Target());

    /*
    **	print the label
    */
    Fancy_Text_Print(buf, 160, 0, &ColorRemaps.at(PCOLOR_BROWN), kTBlack,
                     TPF_CENTER | TPF_NOSHADOW | TPF_EFNT);
  }
}

/***************************************************************************
 * MapEditClass::Mouse_Moved -- checks for mouse motion                    *
 *                                                                         *
 * Reports whether the mouse has moved or not. This varies based on the    *
 * type of object currently selected. If there's an infantry object        *
 *   selected, mouse motion counts even within a cell; for all other types,*
 *   mouse motion counts only if the mouse changes cells.                  *
 *                                                                         *
 *   The reason this routine is needed is to prevent Paint-Mode from putting*
 *   gobs of trees and such into the same cell if the mouse moves just     *
 *   a little bit.                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/08/1994 BR : Created.                                              *
 *=========================================================================*/
bool MapEditClass::Mouse_Moved() {
  static int old_mx = 0;
  static int old_my = 0;
  static CELL old_zonecell = 0;
  const ObjectTypeClass* objtype = nullptr;
  bool retcode = false;

  /*
  **	Return if no motion
  */
  if (old_mx == Get_Mouse_X() && old_my == Get_Mouse_Y()) {
    return false;
  }

  /*
  **	Get a ptr to ObjectTypeClass
  */
  if (PendingObject) {
    objtype = PendingObject;
  } else {
    if (GrabbedObject) {
      objtype = &GrabbedObject->Class_Of();
    } else {
      old_mx = Get_Mouse_X();
      old_my = Get_Mouse_Y();
      old_zonecell = ZoneCell;
      return false;
    }
  }

  /*
  **	Infantry: mouse moved if any motion at all
  */
  if (objtype->What_Am_I() == RTTI_INFANTRYTYPE) {
    retcode = true;
  } else {
    /*
    **	Others: mouse moved only if cell changed
    */
    retcode = old_zonecell != ZoneCell;
  }

  old_mx = Get_Mouse_X();
  old_my = Get_Mouse_Y();
  old_zonecell = ZoneCell;
  return retcode;
}

/***************************************************************************
 * MapEditClass::Main_Menu -- main menu processor for map editor           *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/20/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Main_Menu() {
  const char* _menus[kMaxMainMenuNum + 1];
  int rc = 0;

  /*
  **	Fill in menu items
  */
  _menus[0] = "New Scenario";
  _menus[1] = "Load Scenario";
  _menus[2] = "Save Scenario";
  _menus[3] = "Size Map";
  _menus[4] = "Add Game Object";
  _menus[5] = "Scenario Options";
  _menus[6] = "AI Options";
  _menus[7] = "Play Scenario";
  _menus[8] = nullptr;

  /*
  **	Main Menu loop
  */
  Override_Mouse_Shape(MOUSE_NORMAL);  // display default mouse cursor
  bool process = true;                 // menu stays up while true
  while (process) {
    /*
    **	Invoke game callback, to update music
    */
    ServiceRealTime();

    /*
    **	Invoke menu
    */
    Hide_Mouse();  // Do_Menu assumes the mouse is already hidden
    const int selection = Do_Menu(_menus, true);  // option the user picks
    Show_Mouse();
    if (UnknownKey == KN_ESC || UnknownKey == KN_LMOUSE ||
        UnknownKey == KN_RMOUSE) {
      break;
    }

    /*
    **	Process selection
    */
    switch (selection) {
      /*
      **	New scenario
      */
      case 0:
        if (Changed) {
          rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
          HidPage.Clear();
          Flag_To_Redraw(true);
          Render();
          if (rc == 0) {
            if (Save_Scenario() != 0) {
              break;
            }
            Changed = false;
          }
        }
        if (New_Scenario() == 0) {
          Scen.CarryOverMoney = 0;
          Changed = true;
        }
        process = false;
        break;

      /*
      **	Load scenario
      */
      case 1:
        if (Changed) {
          rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
          HidPage.Clear();
          Flag_To_Redraw(true);
          Render();
          if (rc == 0) {
            if (Save_Scenario() != 0) {
              break;
            }
            Changed = false;
          }
        }
        if (Load_Scenario() == 0) {
          Scen.CarryOverMoney = 0;
          Changed = false;
        }
        process = false;
        break;

      /*
      **	Save scenario
      */
      case 2:
        if (Save_Scenario() == 0) {
          Changed = false;
        }
        process = false;
        break;

      /*
      **	Edit map size
      */
      case 3:
        if (Size_Map(MapCellX, MapCellY, MapCellWidth, MapCellHeight) == 0) {
          process = false;
          Changed = true;
        }
        break;

      /*
      **	Add an object
      */
      case 4:
        if (Placement_Dialog() == 0) {
          Start_Placement();
          process = false;
        }
        break;

      /*
      **	Scenario options
      */
      case 5:
        if (Scenario_Dialog() == 0) {
          Changed = true;
          process = false;
        }
        break;

      /*
      **	Other options
      */
      case 6:
        AI_Menu();
        process = false;
        break;

      /*
      **	Test-drive this scenario
      */
      case 7:
        if (Changed) {
          rc = WWMessageBox().Process("Save Changes?", TXT_YES, TXT_NO,
                                      TXT_CANCEL);
          HidPage.Clear();
          Flag_To_Redraw(true);
          Render();
          if (rc == 2) {
            return;
          }
          if (rc == 0) {
            if (Save_Scenario() != 0) {
              break;
            }
            Changed = false;
          }
        }
        Changed = false;
        MapEditorActive = false;
        Start_Scenario(Scen.ScenarioName);
        return;
      default:
        break;
    }
  }

  /*
  **	Restore the display:
  **	- Clear HIDPAGE to erase any spurious drawing done by the menu system
  **	- Invoke Flag_To_Redraw to tell DisplayClass to re-render the whole
  *screen *	- Invoke Redraw() to update the display
  */
  HidPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::AI_Menu -- menu of AI options                             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::AI_Menu() {
  const char* _menus[kMaxAiMenuNum + 1];

  /*
  **	Fill in menu strings
  */
  _menus[0] = "Pre-Build a Base";
  _menus[1] = "Edit Triggers";
  _menus[2] = "Edit Teams";
  _menus[3] = nullptr;

  /*
  **	Main Menu loop
  */
  Override_Mouse_Shape(MOUSE_NORMAL);  // display default mouse cursor
  bool process = true;                 // menu stays up while true
  while (process) {
    /*
    **	Invoke game callback, to update music
    */
    ServiceRealTime();

    /*
    **	Invoke menu
    */
    Hide_Mouse();  // Do_Menu assumes the mouse is already hidden
    const int selection = Do_Menu(_menus, true);  // option the user picks
    Show_Mouse();
    if (UnknownKey == KN_ESC || UnknownKey == KN_LMOUSE ||
        UnknownKey == KN_RMOUSE) {
      break;
    }

    /*
    **	Process selection
    */
    switch (selection) {
      /*
      **	Pre-Build a Base
      */
      case 0:
        Start_Base_Building();
        process = false;
        break;

      /*
      **	Trigger Editing
      */
      case 1:
        Handle_Triggers();
        /*
        **	Go into trigger placement mode
        */
        if (CurTrigger) {
          Start_Trigger_Placement();
        }
        process = false;
        break;

      /*
      **	Team Editing
      */
      case 2:
        Handle_Teams("Teams");
        process = false;
        break;
      default:
        break;
    }
  }
}

/***************************************************************************
 * MapEditClass::Verify_House -- is this objtype ownable by this house?    *
 *                                                                         *
 * INPUT:                                                                  *
 *      house         house to check                                       *
 *      objtype      ObjectTypeClass to check                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = isn't ownable, 1 = it is                                       *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool MapEditClass::Verify_House(HousesType house,
                                const ObjectTypeClass* objtype) {
  /*
  **	Verify that new house can own this object
  */
  return ((objtype->Get_Ownable() & base::Bit<uint32_t>(house)) != 0);
}

/***************************************************************************
 * MapEditClass::Cycle_House -- finds next valid house for object type     *
 *                                                                         *
 * INPUT:                                                                  *
 *      objtype      ObjectTypeClass ptr to get house for                  *
 *      curhouse      current house value to start with                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      HousesType that's valid for this object type                       *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/23/1994 BR : Created.                                              *
 *=========================================================================*/
HousesType MapEditClass::Cycle_House(HousesType curhouse,
                                     const ObjectTypeClass* /*unused*/) {

  /*
  **	Loop through all house types, starting with the one after 'curhouse';
  **	return the first one that's valid
  */
  int count = 0;  // prevents an infinite loop
  while (true) {
    /*
    **	Go to next house
    */
    if (curhouse == magic_enum::enum_values<HousesType>().back()) {
      curhouse = magic_enum::enum_values<HousesType>().front();
    } else {
      curhouse++;
    }

    /*
    **	Count # iterations; don't go forever
    */
    count++;
    if (std::cmp_equal(count, magic_enum::enum_count<HousesType>())) {
      curhouse = HOUSE_NONE;
      break;
    }

    /*
    **	Break if this is a valid house
    */
    //		if (HouseClass::As_Pointer(curhouse) && Verify_House(curhouse,
    // objtype)) {
    break;
    //		}
  }

  return curhouse;
}

/***************************************************************************
 * MapEditClass::Fatal -- exits with error message                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      code      tells which message to display; this minimizes the       *
 *               use of character strings in the code.                     *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/12/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Fatal(int txt) {
  // Prog_End();
  absl::PrintF("%s\n", Text_String(txt));
  Emergency_Exit(EXIT_FAILURE);
}

bool MapEditClass::Scroll_Map(DirType facing, int& distance, bool really) {
  /*
  ** The popup gadgets require the entire map to be redrawn if we scroll.
  */
  if (MapEditorActive && really) {
    Flag_To_Redraw(true);
  }

  return MouseClass::Scroll_Map(facing, distance, really);
}

void MapEditClass::Detach(ObjectClass* object) {
  if (GrabbedObject == object) {
    GrabbedObject = nullptr;
  }
}

bool MapEditClass::Get_Waypoint_Name(std::span<char> wayptname) {
  /*
  **	Dialog & button dimensions
  */
  constexpr int kDialogW = 100;                     // dialog width
  constexpr int kDialogH = 56;                      // dialog height
  constexpr int kDialogX = ((320 - kDialogW) / 2);  // centered x-coord
  constexpr int kDialogY = ((200 - kDialogH) / 2);  // centered y-coord
  constexpr int kMargin = 7;                        // margin width/height
  constexpr int kEditW = kDialogW - (kMargin * 2);
  constexpr int kEditX = kDialogX + kMargin;
  constexpr int kEditY = kDialogY + 20;
  constexpr int kButtonX = kDialogX + kMargin;
  constexpr int kButtonY = kDialogY + 40;
  constexpr int kButtonW = 40;
  constexpr int kCancelX = kDialogX + 53;
  constexpr int kCancelY = kDialogY + 40;
  constexpr int kCancelW = 40;

  /*
  **	Button enumerations
  */
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;
  constexpr int kButtonEdit = 102;

  /*
  **	Dialog variables
  */
  bool cancel = false;  // true = user cancels
  base::At(wayptname, 0) = 0;

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass button(kButtonOk, TXT_OK, kTpfEButton, kButtonX, kButtonY,
                         kButtonW);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfEButton, kCancelX,
                            kCancelY, kCancelW);
  EditClass editbtn(kButtonEdit, wayptname, 3, TPF_EFNT | TPF_NOSHADOW, kEditX,
                    kEditY, kEditW, -1, EditClass::kAlphanumeric);

  /*
  **	Initialize.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Create the button list.
  */
  commands = &button;
  cancelbtn.Add_Tail(*commands);
  editbtn.Add_Tail(*commands);
  editbtn.Set_Focus();

  /*
  **	Main Processing Loop.
  */
  bool firsttime = true;
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (Session.Type == GAME_NORMAL) {
      ServiceRealTime();
    } else if (RunFrame()) {
      process = false;
      cancel = true;
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      //				Draw_Caption(caption, kDialogX,
      // kDialogY, kDialogW);

      /*
      **	Redraw the buttons.
      */
      commands->Flag_List_To_Redraw();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    KeyNumType input = commands->Input();

    /*
    **	The first time through the processing loop, set the edit
    **	gadget to have the focus. The
    **	focus must be set here since the gadget list has changed
    **	and this change will cause any previous focus setting to be
    **	cleared by the input processing routine.
    */
    if (firsttime) {
      firsttime = false;
      editbtn.Set_Focus();
      editbtn.Flag_To_Redraw();
    }

    /*
    **	If the <RETURN> key was pressed, then default to the appropriate
    **	action button according to the style of this dialog box.
    */
    if (input == KN_RETURN) {
      input = ButtonKey(kButtonOk);
    }

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
      /*
      ** Load: if load fails, present a message, and stay in the dialog
      ** to allow the user to try another game
      */
      case ButtonKey(kButtonOk):
        Hide_Mouse();
        SeenBuff.Clear();
        GamePalette.Set();
        Show_Mouse();
        process = false;
        cancel = false;
        break;

      /*
      ** ESC/Cancel: break
      */
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        Hide_Mouse();
        SeenBuff.Clear();
        GamePalette.Set();
        Show_Mouse();
        cancel = true;
        process = false;
        break;

      default:
        break;
    }
  }

  Map.Flag_To_Redraw(true);
  return !cancel;
}

void MapEditClass::Update_Waypoint(int waypt_idx) {

  /*
  **	Unflag cell for this waypoint if there is one
  */
  CELL const cell = base::At(Scen.Waypoint, waypt_idx);
  if (cell != -1) {
    if (Scen.Waypoint[ScenarioClass::kHomeWaypoint] != cell &&
        Scen.Waypoint[ScenarioClass::kReinforcementWaypoint] != cell) {
      (*this).at(cell).IsWaypoint = false;
    }
    Flag_Cell(cell);
  }
  base::At(Scen.Waypoint, waypt_idx) = CurrentCell;
  (*this).at(CurrentCell).IsWaypoint = true;
  Changed = true;
  Flag_Cell(CurrentCell);
}

/***************************************************************************
 * MapEditClass::Read_INI -- overloaded Read_INI function                  *
 *                                                                         *
 * Overloading this function gives the map editor a chance to initialize   *
 * certain values every time a new INI is read.                            *
 *                                                                         *
 * INPUT:                                                                  *
 *      buffer      INI staging area                                       *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/16/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Read_INI(CCINIClass& ini) {
  DLOG(INFO) << "Map editor reading scenario INI";

  MouseClass::Read_INI(ini);
  BaseGauge->Set_Value(Scen.Percent);
}

void Go_Editor(const bool flag) {
  // Go into Scenario Editor mode
  if (flag) {
    MapEditorActive = true;
    Debug_Unshroud = true;

    // Un-select any selected objects
    Unselect_All();

    // Turn off the sidebar if it's on
    Map.Activate(0);

    // Reset the map's Button list for the new mode
    Map.Init_IO();

    // Force a complete redraw of the screen
    HiddenPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();

  } else {
    // Go into normal game mode
    MapEditorActive = false;
    Debug_Unshroud = false;

    // Un-select any selected objects
    Unselect_All();

    // Reset the map's Button list for the new mode
    Map.Init_IO();

    // Force a complete redraw of the screen
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();
  }
}
