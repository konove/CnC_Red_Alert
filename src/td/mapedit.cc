/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\mapedit.cpv   2.18   16 Oct 1995 16:48:40
 * JOE_BOSTIC  $ */
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
 *   MapEditClass::MapEditClass -- class constructor                       *
 *   MapEditClass::One_Time -- one-time initialization                     *
 *   MapEditClass::Read_INI -- overloaded Read_INI function                *
 *   MapEditClass::Clear_List -- clears the internal choosable object list *
 *   MapEditClass::Add_To_List -- adds a TypeClass to the choosable list   *
 *   MapEditClass::AI -- The map editor's main logic                       *
 *   MapEditClass::Draw_It -- overloaded Redraw routine                    *
 *   MapEditClass::Main_Menu -- main menu processor for map editor         *
 *   MapEditClass::AI_Menu -- menu of AI options                           *
 *   MapEditClass::Mouse_Moved -- checks for mouse motion                  *
 *   MapEditClass::Verify_House -- sees if given house can own given obj   *
 *   MapEditClass::Cycle_House -- finds next valid house for object type   *
 *   MapEditClass::Trigger_Needs_Team -- tells if a trigger needs a team   *
 *   MapEditClass::Fatal -- exits with error message                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/mapedit.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "td/base.h"
#include "td/building.h"
#include "td/cell.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/dial8.h"
#include "td/dialog.h"
#include "td/facing.h"
#include "td/gadget.h"
#include "td/game_clock.h"
#include "td/game_state.h"
#include "td/gauge.h"
#include "td/house.h"
#include "td/inline.h"
#include "td/input.h"
#include "td/jshell.h"
#include "td/list.h"
#include "td/menus.h"
#include "td/mission.h"
#include "td/mouse.h"
#include "td/msgbox.h"
#include "td/profile.h"
#include "td/scenario.h"
#include "td/screen.h"
#include "td/startup.h"
#include "td/target.h"
#include "td/techno.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "td/txtlabel.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"
#include "td/world.h"

using enum engine::window::KeyNumber;

/*
****************************** Globals/Externs ******************************
*/

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

  for (int i = 0; i < kNumEditClasses; i++) {
    base::At(NumType, i) = 0;
    base::At(TypeOffset, i) = 0;
  }
  // The home waypoint and the editor's current cell start at zero. World's
  // constructor does that now: this one runs while World is still building
  // its members, so it cannot reach back through TheWorld().
  CurTrigger = nullptr;
  Changed = false;
  LMouseDown = false;
  BaseBuilding = false;
  BasePercent = 100;
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

  /*------------------------------------------------------------------------
  Create the pop-up controls
  ------------------------------------------------------------------------*/
  /*........................................................................
  The map: a single large "button"
  ........................................................................*/
  // MapArea = new ControlClass(kMapArea,0,8,312,192, GadgetClass::kLeftPress |
  // GadgetClass::kLeftRelease, false);
  MapArea = new ControlClass(
      kMapArea, 0, 16, 624, 384,
      GadgetClass::kLeftPress | GadgetClass::kLeftRelease, false);

  /*........................................................................
  House buttons
  ........................................................................*/
  GDIButton = new TextButtonClass(
      kPopupGdi, "GDI",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kPopupGdiX,
      kPopupGdiY, kPopupGdiW, kPopupGdiH);

  NODButton = new TextButtonClass(
      kPopupNod, "NOD",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kPopupNodX,
      kPopupNodY, kPopupNodW, kPopupNodH);

  NeutralButton = new TextButtonClass(
      kPopupNeutral, "Neutral",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kPopupNeutralX, kPopupNeutralY, kPopupNeutralW, kPopupNeutralH);

  Multi1Button = new TextButtonClass(
      kPopupMulti1, "M1",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kPopupMulti1X, kPopupMulti1Y, kPopupMulti1W, kPopupMulti1H);

  Multi2Button = new TextButtonClass(
      kPopupMulti2, "M2",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kPopupMulti2X, kPopupMulti2Y, kPopupMulti2W, kPopupMulti2H);

  Multi3Button = new TextButtonClass(
      kPopupMulti3, "M3",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kPopupMulti3X, kPopupMulti3Y, kPopupMulti3W, kPopupMulti3H);

  Multi4Button = new TextButtonClass(
      kPopupMulti4, "M4",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kPopupMulti4X, kPopupMulti4Y, kPopupMulti4W, kPopupMulti4H);

  /*........................................................................
  The mission list box
  ........................................................................*/
  MissionList = new ListClass(
      kPopupMissionlist, kPopupMissionX, kPopupMissionY, kPopupMissionW,
      kPopupMissionH, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      Hires_Retrieve("BTN-UP.SHP"), Hires_Retrieve("BTN-DN.SHP"));

  for (const auto mission : MapEditMissions) {
    MissionList->Add_Item(MissionClass::Mission_Name(mission));
  }

  /*........................................................................
  The health bar
  ........................................................................*/
  HealthGauge =
      new TriColorGaugeClass(kPopupHealthgauge, kPopupHealthX, kPopupHealthY,
                             kPopupHealthW, kPopupHealthH);
  HealthGauge->Use_Thumb(true);
  HealthGauge->Set_Maximum(0x100);
  HealthGauge->Set_Red_Limit(0x3f - 1);
  HealthGauge->Set_Yellow_Limit(0x7f - 1);

  /*........................................................................
  The health text label
  ........................................................................*/
  base::At(HealthBuf, 0) = 0;
  HealthText = new TextLabelClass(
      HealthBuf, kPopupHealthX + (kPopupHealthW / 2),
      kPopupHealthY + kPopupHealthH + 1, kCcGreen,
      TPF_CENTER | TPF_FULLSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);

  /*........................................................................
  The facing dial
  ........................................................................*/
  FacingDial =
      new Dial8Class(kPopupFacingdial, kPopupFaceboxX, kPopupFaceboxY,
                     kPopupFaceboxW, kPopupFaceboxH, static_cast<DirType>(0));

  /*........................................................................
  The base percent-built slider & its label
  ........................................................................*/
  BaseGauge = new GaugeClass(kPopupBasepercent, kPopupBaseX, kPopupBaseY,
                             kPopupBaseW, kPopupBaseH);
  // TextLabelClass keeps the pointer in its non-const Text member, so the
  // caption needs storage that outlives this call and is not a literal.
  static char base_caption[] = "Base:";
  BaseLabel = new TextLabelClass(
      base_caption, kPopupBaseX - 3, kPopupBaseY, kCcGreen,
      TPF_RIGHT | TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
  BaseGauge->Set_Maximum(100);
  BaseGauge->Set_Value(BasePercent);
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
  /*------------------------------------------------------------------------
  For normal game mode, jump to the parent's Init routine.
  ------------------------------------------------------------------------*/
  if (!TheDebugState().map_editor_active()) {
    MouseClass::Init_IO();

  } else {
    /*------------------------------------------------------------------------
    For editor mode, add the map area to the button input list
    ------------------------------------------------------------------------*/
    Buttons = nullptr;
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
  }
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
void MapEditClass::Read_INI(char* buffer) {
  /*
  ------------------------ Invoke parent's Read_INI ------------------------
  */
  MouseClass::Read_INI(buffer);

  BasePercent = WWGetPrivateProfileInt("Basic", "Percent", 0, buffer);
  BaseGauge->Set_Value(BasePercent);
}

/***************************************************************************
 * MapEditClass::Write_INI -- overloaded Read_INI function                 *
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
void MapEditClass::Write_INI(std::span<char> buffer) {
  /*
  ----------------------- Invoke parent's Write_INI ------------------------
  */
  MouseClass::Write_INI(buffer);

  /*
  ** Save the base's percent-built value; this must be saved into the BASIC
  ** section of the INI, since the Base section will be entirely erased
  ** by the Base's Write_INI routine.
  */
  WWWritePrivateProfileInt("Basic", "Percent", BasePercent, buffer);
}

/***************************************************************************
 * MapEditClass::Clear_List -- clears the internal choosable object list   *
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
  /*------------------------------------------------------------------------
  Set # object type ptrs to 0, set NumType for each type to 0
  ------------------------------------------------------------------------*/
  ObjCount = 0;
  for (int& i : NumType) {
    i = 0;
  }
}

/***************************************************************************
 * MapEditClass::Add_To_List -- adds a TypeClass to the choosable list     *
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
        base::At(NumType, 0)++;
        break;

      case RTTI_OVERLAYTYPE:
        base::At(NumType, 1)++;
        break;

      case RTTI_SMUDGETYPE:
        base::At(NumType, 2)++;
        break;

      case RTTI_TERRAINTYPE:
        base::At(NumType, 3)++;
        break;

      case RTTI_UNITTYPE:
        base::At(NumType, 4)++;
        break;

      case RTTI_INFANTRYTYPE:
        base::At(NumType, 5)++;
        break;

      case RTTI_AIRCRAFTTYPE:
        base::At(NumType, 6)++;
        break;

      case RTTI_BUILDINGTYPE:
        base::At(NumType, 7)++;
        break;
      case RTTIType::RTTI_NONE:
      case RTTIType::RTTI_INFANTRY:
      case RTTIType::RTTI_UNIT:
      case RTTIType::RTTI_AIRCRAFT:
      case RTTIType::RTTI_BUILDING:
      case RTTIType::RTTI_TERRAIN:
      case RTTIType::RTTI_ABSTRACTTYPE:
      case RTTIType::RTTI_ANIM:
      case RTTIType::RTTI_ANIMTYPE:
      case RTTIType::RTTI_BULLET:
      case RTTIType::RTTI_BULLETTYPE:
      case RTTIType::RTTI_OVERLAY:
      case RTTIType::RTTI_SMUDGE:
      case RTTIType::RTTI_TEAM:
      case RTTIType::RTTI_TEMPLATE:
      case RTTIType::RTTI_OBJECT:
      case RTTIType::RTTI_SPECIAL:
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
 *      kPopupGdi:            makes GDI the owner of this object           *
 *      kPopupNod:            makes NOD the owner of this object           *
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
void MapEditClass::AI(engine::window::InputEvent& event, int x, int y) {
  engine::window::KeyNumber& input = event.key;
  int rc = 0;
  CELL cell = 0;
  int found = 0;      // for removing a waypoint label
  int waypt_idx = 0;  // for labelling a waypoint
  HousesType house = HOUSE_NONE;
  /*------------------------------------------------------------------------
  Trap 'F2' regardless of whether we're in game or editor mode
  ------------------------------------------------------------------------*/
  if (TheDebugState().developer_mode() &&
      (/*(input == KN_F2 && Session == GAME_SOLO) ||*/ input == Ctrl(KN_F2))) {
    TheWorld().scenario_init() = 0;

    /*
    ** If we're in editor mode & Changed is set, prompt for saving changes
    */
    if (TheDebugState().map_editor_active() && Changed) {
      rc = CCMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
      TheScreen().hidden_page().view().Clear();
      Flag_To_Redraw(true);
      Render();
      /*
      ........................ User wants to save ........................
      */
      if (rc == 0) {
        /*
        ................ If save cancelled, abort game ..................
        */
        if (Save_Scenario() != 0) {
          input = KN_NONE;
        } else {
          Changed = false;
          Go_Editor(!TheDebugState().map_editor_active());
        }
      } else {
        /*
        .................... User doesn't want to save .....................
        */
        Go_Editor(!TheDebugState().map_editor_active());
      }
    } else {
      /*
      ** If we're in game mode, set Changed to 0 (so if we didn't save our
      ** changes above, they won't keep coming back to haunt us with continual
      ** Save Changes? prompts!)
      */
      if (!TheDebugState().map_editor_active()) {
        Changed = false;
      }
      Go_Editor(!TheDebugState().map_editor_active());
    }
  }

  /*------------------------------------------------------------------------
  For normal game mode, jump to the parent's AI routine.
  ------------------------------------------------------------------------*/
  if (!TheDebugState().map_editor_active()) {
    MouseClass::AI(event, x, y);
    return;
  }

  TheGameClock().Advance();

  /*------------------------------------------------------------------------
  Do special mouse processing if the mouse is over the map
  ------------------------------------------------------------------------*/
  if (Get_Mouse_X() > TacPixelX &&
      Get_Mouse_X() < TacPixelX + Lepton_To_Pixel(TacLeptonWidth) &&
      Get_Mouse_Y() > TacPixelY &&
      Get_Mouse_Y() < TacPixelY + Lepton_To_Pixel(TacLeptonHeight)) {
    /*.....................................................................
    When the mouse moves over a scrolling edge, ScrollClass changes its
    shape to the appropriate arrow or NO symbol; it's our job to change it
    back to normal (or whatever the shape is set to by Set_Default_Mouse())
    when it re-enters the map area.
    .....................................................................*/
    if (CurTrigger) {
      Override_Mouse_Shape(MOUSE_CAN_MOVE);
    } else {
      Override_Mouse_Shape(MOUSE_NORMAL);
    }
  }

  /*.....................................................................
  Set 'ZoneCell' to track the mouse cursor around over the map.  Do this
  even if the map is scrolling.
  .....................................................................*/
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

  /*------------------------------------------------------------------------
  Check for mouse motion while left button is down.
  ------------------------------------------------------------------------*/
  const bool moved = Mouse_Moved();
  if (LMouseDown && moved) {
    /*.....................................................................
    "Paint" mode: place current object, and restart placement
    .....................................................................*/
    if (PendingObject) {
      Flag_To_Redraw(true);
      if (Place_Object() == 0) {
        Changed = true;
        Start_Placement();
      }
    } else {
      /*.....................................................................
      Move the currently-grabbed object
      .....................................................................*/
      if (GrabbedObject) {
        GrabbedObject->Mark(MARK_CHANGE);
        if (Move_Grabbed_Object() == 0) {
          Changed = true;
        }
      }
    }
  }

  /*------------------------------------------------------------------------
  Trap special editing keys; if one is detected, set 'input' to 0 to
  prevent a conflict with parent's AI().
  ------------------------------------------------------------------------*/
  // A right-click pops up the main menu.
  if (event.IsPress(engine::window::MouseButton::kRight)) {
    /*
    ..................... Turn off placement mode ......................
    */
    if (PendingObject) {
      if (BaseBuilding) {
        Cancel_Base_Building();
      } else {
        Cancel_Placement();
      }
    }

    /*
    ................. Turn off trigger placement mode ..................
    */
    if (CurTrigger) {
      Stop_Trigger_Placement();
    }

    /*
    .............. Unselect object & hide popup controls ...............
    */
    if (TheWorld().current_object().Count()) {
      TheWorld().current_object().at(0)->Unselect();
      Popup_Controls();
    }
    Main_Menu();
    event = {};
  }

  switch (static_cast<int>(input)) {
    /*---------------------------------------------------------------------
    F6 = toggle passable/impassable display
    ---------------------------------------------------------------------*/
    case KN_F6:
      TheDebugState().set_show_passability(!TheDebugState().show_passability());
      TheScreen().hidden_page().view().Clear();
      Flag_To_Redraw(true);
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    INSERT = go into object-placement mode
    ---------------------------------------------------------------------*/
    case KN_INSERT:
      if (!PendingObject) {
        /*
        ......... Unselect current object, hide popup controls ..........
        */
        if (TheWorld().current_object().Count()) {
          TheWorld().current_object().at(0)->Unselect();
          Popup_Controls();
        }
        /*
        .................... Go into placement mode .....................
        */
        Start_Placement();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    ESC = exit placement mode, or exit to DOS
    ---------------------------------------------------------------------*/
    case KN_ESC:

      /*
      .................... Exit object placement mode ....................
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
      ................... Exit trigger placement mode ....................
      */
      if (CurTrigger) {
        Stop_Trigger_Placement();
        input = KN_NONE;
        break;
      }
      rc = CCMessageBox().Process("Exit Scenario Editor?", TXT_YES, TXT_NO);
      TheScreen().hidden_page().view().Clear();
      Flag_To_Redraw(true);
      Render();

      /*
      .......... User doesn't want to exit; return to editor ..........
      */
      if (rc == 1) {
        input = KN_NONE;
        break;
      }

      /*
      ................. If changed, prompt for saving .................
      */
      if (Changed) {
        rc = CCMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
        TheScreen().hidden_page().view().Clear();
        Flag_To_Redraw(true);
        Render();

        /*
        ..................... User wants to save .....................
        */
        if (rc == 0) {
          /*
          .............. If save cancelled, abort exit ..............
          */
          if (Save_Scenario() != 0) {
            input = KN_NONE;
            break;
          }
          Changed = false;
        }
      }
      ShutDown();
      exit(0);

    /*---------------------------------------------------------------------
    LEFT = go to previous placement object
    ---------------------------------------------------------------------*/
    case KN_LEFT:
      if (PendingObject) {
        Place_Prev();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    RIGHT = go to next placement object
    ---------------------------------------------------------------------*/
    case KN_RIGHT:
      if (PendingObject) {
        Place_Next();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    PGUP = go to previous placement category
    ---------------------------------------------------------------------*/
    case KN_PGUP:
      if (PendingObject) {
        Place_Prev_Category();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    PGDN = go to next placement category
    ---------------------------------------------------------------------*/
    case KN_PGDN:
      if (PendingObject) {
        Place_Next_Category();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    HOME = jump to first placement object, or go to Home Cell
    ---------------------------------------------------------------------*/
    case KN_HOME:
      if (PendingObject) {
        Place_Home();
      } else {
        /*
        ....................... Set map position ........................
        */
        TheWorld().scenario_init()++;
        Set_Tactical_Position(
            Cell_Coord(base::At(TheWorld().waypoint(), kWayptHome)));
        TheWorld().scenario_init()--;

        /*
        ...................... Force map to redraw ......................
        */
        TheScreen().hidden_page().view().Clear();
        Flag_To_Redraw(true);
        Render();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    SHIFT-HOME: set new Home Cell position
    ---------------------------------------------------------------------*/
    case Shift(KN_HOME):
      /*
      ** Unflag the old Home Cell, if there are no other waypoints
      ** pointing to it
      */
      cell = base::At(TheWorld().waypoint(), kWayptHome);

      if (cell != -1) {
        found = 0;
        for (int i = 0; i < kWayptCount; i++) {
          if (i != kWayptHome && base::At(TheWorld().waypoint(), i) == cell) {
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
      base::At(TheWorld().waypoint(), kWayptHome) = Coord_Cell(TacticalCoord);
      (*this).at(Coord_Cell(TacticalCoord)).IsWaypoint = true;
      Flag_Cell(Coord_Cell(TacticalCoord));
      Changed = true;
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    SHIFT-R: set new Reinforcement Cell position.  Don't allow setting
    the Reinf. Cell to the same as the Home Cell (for display purposes.)
    ---------------------------------------------------------------------*/
    case Shift(KN_R):
      if (TheWorld().current_cell() == 0 ||
          TheWorld().current_cell() ==
              base::At(TheWorld().waypoint(), kWayptHome)) {
        break;
      }

      /*
      ** Unflag the old Reinforcement Cell, if there are no other waypoints
      ** pointing to it
      */
      cell = base::At(TheWorld().waypoint(), kWayptReinf);

      if (cell != -1) {
        found = 0;
        for (int i = 0; i < kWayptCount; i++) {
          if (i != kWayptReinf && base::At(TheWorld().waypoint(), i) == cell) {
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
      base::At(TheWorld().waypoint(), kWayptReinf) = TheWorld().current_cell();
      (*this).at(TheWorld().current_cell()).IsWaypoint = true;
      Flag_Cell(TheWorld().current_cell());
      Changed = true;
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    ALT-Letter: Label a waypoint cell
    ---------------------------------------------------------------------*/
    case Alt(KN_A):
    case Alt(KN_B):
    case Alt(KN_C):
    case Alt(KN_D):
    case Alt(KN_E):
    case Alt(KN_F):
    case Alt(KN_G):
    case Alt(KN_H):
    case Alt(KN_I):
    case Alt(KN_J):
    case Alt(KN_K):
    case Alt(KN_L):
    case Alt(KN_M):
    case Alt(KN_N):
    case Alt(KN_O):
    case Alt(KN_P):
    case Alt(KN_Q):
    case Alt(KN_R):
    case Alt(KN_S):
    case Alt(KN_T):
    case Alt(KN_U):
    case Alt(KN_V):
    case Alt(KN_W):
    case Alt(KN_X):
    case Alt(KN_Y):
    case Alt(KN_Z):
      if (TheWorld().current_cell() != 0) {
        waypt_idx = engine::window::KeyBuffer::ToAscii(KeyCode(input)) - 'a';
        /*...............................................................
        Unflag cell for this waypoint if there is one
        ...............................................................*/
        cell = base::At(TheWorld().waypoint(), waypt_idx);
        if (cell != -1) {
          if (base::At(TheWorld().waypoint(), kWayptHome) != cell &&
              base::At(TheWorld().waypoint(), kWayptReinf) != cell) {
            (*this).at(cell).IsWaypoint = false;
          }
          Flag_Cell(cell);
        }
        base::At(TheWorld().waypoint(), waypt_idx) = TheWorld().current_cell();
        (*this).at(TheWorld().current_cell()).IsWaypoint = true;
        Changed = true;
        Flag_Cell(TheWorld().current_cell());
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    ALT-1-4: Designate a cell as a capture-the-flag cell.
    ---------------------------------------------------------------------*/
    case Alt(KN_1):
    case Alt(KN_2):
    case Alt(KN_3):
    case Alt(KN_4):
      /*------------------------------------------------------------------
      If there's a current cell, place the flag & waypoint there.
      ------------------------------------------------------------------*/
      if (TheWorld().current_cell() != 0) {
        waypt_idx = (engine::window::KeyBuffer::ToAscii(KeyCode(input)) - '1');
        house =
            static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + waypt_idx);
        if (HouseClass::As_Pointer(house)) {
          HouseClass::As_Pointer(house)->Flag_Attach(TheWorld().current_cell(),
                                                     true);
        }
      } else {
        /*------------------------------------------------------------------
        If there's a current object, attach the flag to it and clear the
        waypoint.
        ------------------------------------------------------------------*/
        if (TheWorld().current_object().at(0) != nullptr) {
          waypt_idx =
              (engine::window::KeyBuffer::ToAscii(KeyCode(input)) - '1');
          house = static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) +
                                          waypt_idx);
          if (HouseClass::As_Pointer(house) &&
              TheWorld().current_object().at(0)->What_Am_I() == RTTI_UNIT) {
            HouseClass::As_Pointer(house)->Flag_Attach(
                dynamic_cast<UnitClass*>(TheWorld().current_object().at(0)),
                true);
          }
        }
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    ALT-Space: Remove a waypoint designation
    ---------------------------------------------------------------------*/
    case Alt(KN_SPACE):
      if (TheWorld().current_cell() != 0) {
        /*...............................................................
        Loop through letter waypoints; if this cell is one of them,
        clear that waypoint.
        ...............................................................*/
        for (int i = 0; i < 26; i++) {
          if (base::At(TheWorld().waypoint(), i) == TheWorld().current_cell()) {
            base::At(TheWorld().waypoint(), i) = -1;
          }
        }

        /*...............................................................
        Loop through flag home values; if this cell is one of them, clear
        that waypoint.
        ...............................................................*/
        for (int i = 0; i < MAX_PLAYERS; i++) {
          house = static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + i);
          if (HouseClass::As_Pointer(house) &&
              TheWorld().current_cell() ==
                  HouseClass::As_Pointer(house)->FlagHome) {
            HouseClass::As_Pointer(house)->Flag_Remove(
                As_Target(TheWorld().current_cell()), true);
          }
        }

        /*...............................................................
        If there are no more waypoints on this cell, clear the cell's
        waypoint designation.
        ...............................................................*/
        if (base::At(TheWorld().waypoint(), kWayptHome) !=
                TheWorld().current_cell() &&
            base::At(TheWorld().waypoint(), kWayptReinf) !=
                TheWorld().current_cell()) {
          (*this).at(TheWorld().current_cell()).IsWaypoint = false;
        }
        Changed = true;
        Flag_Cell(TheWorld().current_cell());
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    'H' = toggle current placement object's house
    ---------------------------------------------------------------------*/
    case KN_H:
    case Shift(KN_H):
      if (PendingObject) {
        Toggle_House();
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Left-mouse click:
    Button DOWN:
    - Toggle LMouseDown
    - If we're in placement mode, try to place the current object
      - If success, re-enter placement mode
    - Otherwise, try to select an object, and "grab" it if there is one
    - If no object, then select that cell as the "current" cell
    Button UP:
    - Toggle LMouseDown
    - release any grabbed object
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kMapArea):
      /*
      ------------------------- Left Button DOWN -------------------------
      */
      if (IsLeftButtonDown()) {
        LMouseDown = true;
        /*
        ............... Placement mode: place an object .................
        */
        if (PendingObject) {
          if (Place_Object() == 0) {
            Changed = true;
            Start_Placement();
          }
        } else {
          /*
          ....................... Place a trigger .........................
          */
          if (CurTrigger) {
            Place_Trigger(event.x, event.y);
            Changed = true;
          } else {
            /*
            ................. Select an object or a cell .................
            .................. Check for double-click ....................
            */
            if (TheWorld().current_object().Count() &&
                ((SystemTicks() - LastClickTime) < 15)) {
            } else {
              /*
              ................ Single-click: select object .................
              */
              if (Select_Object(event.x, event.y) == 0) {
                TheWorld().current_cell() = 0;
                Grab_Object();
              } else {
                /*
                ................ No object: select the cell ..................
                */
                TheWorld().current_cell() = Click_Cell_Calc(event.x, event.y);
                TheScreen().hidden_page().view().Clear();
                Flag_To_Redraw(true);
                Render();
              }
            }
          }
        }
        LastClickTime = SystemTicks();
        input = KN_NONE;
      } else {
        /*
        -------------------------- Left Button UP --------------------------
        */
        LMouseDown = false;
        GrabbedObject = nullptr;
        input = KN_NONE;
      }
      break;

    /*---------------------------------------------------------------------
    SHIFT-ALT-Arrow: move the current object
    ---------------------------------------------------------------------*/
    case Shift(Alt(KN_UP)):
    case Shift(Alt(KN_DOWN)):
    case Shift(Alt(KN_LEFT)):
    case Shift(Alt(KN_RIGHT)):
      if (TheWorld().current_object().Count()) {
        TheWorld().current_object().at(0)->Move(KN_To_Facing(input));
        Changed = true;
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    DELETE: delete currently-selected object
    ---------------------------------------------------------------------*/
    case KN_DELETE:
      /*..................................................................
      Delete currently-selected object's trigger, or the object
      ..................................................................*/
      if (TheWorld().current_object().Count()) {
        /*
        ........................ Delete trigger .........................
        */
        if (TheWorld().current_object().at(0)->Trigger) {
          TheWorld().current_object().at(0)->Trigger = nullptr;
        } else {
          /*
          ** If the current object is part of the AI's Base, remove it
          ** from the Base's Node list.
          */
          if (TheWorld().current_object().at(0)->What_Am_I() == RTTI_BUILDING) {
            auto* building =
                dynamic_cast<BuildingClass*>(TheWorld().current_object().at(0));
            if (TheWorld().base().Is_Node(building)) {
              BaseNodeClass* node = TheWorld().base().Get_Node(
                  building);  // for removing from an AI Base
              TheWorld().base().Nodes.Delete(*node);
            }
          }

          /*
          ................... Delete current object ....................
          */
          delete TheWorld().current_object().at(0);

          /*
          .................. Hide the popup controls ...................
          */
          Popup_Controls();
        }

        /*
        ........................ Force a redraw .........................
        */
        TheScreen().hidden_page().view().Clear();
        Flag_To_Redraw(true);
        Changed = true;
      } else {
        /*
        ................. Remove trigger from current cell .................
        */
        if (TheWorld().current_cell() &&
            (*this).at(TheWorld().current_cell()).IsTrigger) {
          (*this).at(TheWorld().current_cell()).IsTrigger = false;
          TheWorld().cell_triggers().at(TheWorld().current_cell()) = nullptr;
          /*
          ...................... Force a redraw ........................
          */
          TheScreen().hidden_page().view().Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    TAB: select next object on the map
    ---------------------------------------------------------------------*/
    case KN_TAB:
      Select_Next();
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Object-Editing button: House Button
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kPopupGdi):
    case engine::window::ButtonKey(kPopupNod):
    case engine::window::ButtonKey(kPopupNeutral):
    case engine::window::ButtonKey(kPopupMulti1):
    case engine::window::ButtonKey(kPopupMulti2):
    case engine::window::ButtonKey(kPopupMulti3):
    case engine::window::ButtonKey(kPopupMulti4):
      /*..................................................................
      Convert input value into a house value; assume HOUSE_GOOD is 0
      ..................................................................*/
      house = static_cast<HousesType>(ButtonId(input) - kPopupGdi);
      /*..................................................................
      If that house doesn't own this object, try to transfer it
      ..................................................................*/
      if ((TheWorld().current_object().at(0)->Owner() != house) &&
          Change_House(house)) {
        Changed = true;
      }

      Set_House_Buttons(TheWorld().current_object().at(0)->Owner(), Buttons,
                        kPopupGdi);
      TheScreen().hidden_page().view().Clear();
      Flag_To_Redraw(true);
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Object-Editing button: Mission
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kPopupMissionlist):
      if (TheWorld().current_object().at(0)->Is_Techno()) {
        /*
        ........................ Set new mission ........................
        */
        const MissionType mission =
            MapEditMissions.at(base::ToSize(MissionList->Current_Index()));
        if (TheWorld().current_object().at(0)->Get_Mission() != mission) {
          dynamic_cast<TechnoClass*>(TheWorld().current_object().at(0))
              ->Set_Mission(mission);
          Changed = true;
        }
      }
      Flag_To_Redraw(true);
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Object-Editing button: Health
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kPopupHealthgauge):
      if (TheWorld().current_object().at(0)->Is_Techno()) {
        /*
        .......... Derive strength from current gauge reading ...........
        */
        int strength = Fixed_To_Cardinal(
            TheWorld().current_object().at(0)->Class_Of().MaxStrength,
            HealthGauge->Get_Value());

        /*
        ........................... Clip to 1 ...........................
        */
        if (strength <= 0) {
          strength = 1;
        }

        /*
        ....................... Set new strength ........................
        */
        if (strength != TheWorld().current_object().at(0)->Strength) {
          TheWorld().current_object().at(0)->Strength =
              static_cast<int16_t>(strength);
          TheScreen().hidden_page().view().Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }

        /*
        ....................... Update text label .......................
        */
        absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d", strength);
      }
      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Object-Editing button: Facing
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kPopupFacingdial):
      if (TheWorld().current_object().at(0)->Is_Techno()) {
        auto* techno =
            dynamic_cast<TechnoClass*>(TheWorld().current_object().at(0));
        if (FacingDial->Get_Direction() != techno->PrimaryFacing.Get()) {
          /*
          ..................... Set body's facing ......................
          */
          techno->PrimaryFacing.Set(FacingDial->Get_Direction());

          /*
          ..................... Set turret facing, if there is one
          ......................
          */
          if (techno->What_Am_I() == RTTI_UNIT) {
            dynamic_cast<UnitClass&>(*techno).SecondaryFacing.Set(
                FacingDial->Get_Direction());
          }

          TheScreen().hidden_page().view().Clear();
          Flag_To_Redraw(true);
          Changed = true;
        }
      }

      input = KN_NONE;
      break;

    /*---------------------------------------------------------------------
    Object-Editing button: Facing
    ---------------------------------------------------------------------*/
    case engine::window::ButtonKey(kPopupBasepercent):
      if (BaseGauge->Get_Value() != BasePercent) {
        BasePercent = BaseGauge->Get_Value();
        Build_Base_To(BasePercent);
        TheScreen().hidden_page().view().Clear();
        Flag_To_Redraw(true);
      }
      input = KN_NONE;
      break;

    default:
      break;
  }

  // A left click no gadget took stops here.
  if (event.IsPress(engine::window::MouseButton::kLeft)) {
    event = {};
  }

  /*
  ------------------------ Call parent's AI routine ------------------------
  */
  MouseClass::AI(event, x, y);
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
void MapEditClass::Draw_It(PixelView& view, bool forced) {
  char buf[40];

  MouseClass::Draw_It(view, forced);

  if (!TheDebugState().map_editor_active()) {
    return;
  }

  //
  // Erase scrags at top of screen
  //
  view.FillRect(0, 0, 640, 16, kBlack);

  /*
  **	Display the total value of all Tiberium on the map.
  */
  Fancy_Text_Print(view, "Tiberium=%ld   ", 0, 0, kCcGreen, kBlack,
                   TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, TotalValue);

  /*------------------------------------------------------------------------
  If there are no object controls displayed, just invoke parent's Redraw
  and return.
  ------------------------------------------------------------------------*/
  if (!Buttons) {
    return;
  }

  /*------------------------------------------------------------------------
  Otherwise, if 'display' is set, invoke the parent's Redraw to refresh
  the HIDPAGE; then, update the buttons & text labels onto HIDPAGE;
  then invoke the parent's Redraw to blit the HIDPAGE to SEENPAGE.
  ------------------------------------------------------------------------*/
  if (forced && TheWorld().current_object().Count()) {
    /*
    ....................... Update the text labels ........................
    */
    /*
    ------------------ Display the object's name & ID ------------------
    */
    const char* label =
        Text_String(TheWorld().current_object().at(0)->Full_Name());
    const char* tptr = label;
    absl::SNPrintF(buf, sizeof(buf), "%s (%d)", tptr,
                   TheWorld().current_object().at(0)->As_Target());

    /*
    ......................... print the label ..........................
    */
    Fancy_Text_Print(
        view, buf, 320, 0, kCcTan, kTBlack,
        TPF_CENTER | TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
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
  -------------------------- Return if no motion ---------------------------
  */
  if (old_mx == Get_Mouse_X() && old_my == Get_Mouse_Y()) {
    return false;
  }

  /*
  ---------------------- Get a ptr to ObjectTypeClass ----------------------
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
  --------------------- Check for motion based on type ---------------------
  */
  /*
  ............... Infantry: mouse moved if any motion at all ...............
  */
  if (objtype->What_Am_I() == RTTI_INFANTRYTYPE) {
    retcode = true;
  } else {
    /*
    ................ Others: mouse moved only if cell changed ................
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
  --------------------------- Fill in menu items ---------------------------
  */
  base::At(_menus, 0) = "New Scenario";
  base::At(_menus, 1) = "Load Scenario";
  base::At(_menus, 2) = "Save Scenario";
  base::At(_menus, 3) = "Size Map";
  base::At(_menus, 4) = "Add Game Object";
  base::At(_menus, 5) = "Scenario Options";
  base::At(_menus, 6) = "AI Options";
  base::At(_menus, 7) = "Play Scenario";
  base::At(_menus, 8) = nullptr;

  /*
  ----------------------------- Main Menu loop -----------------------------
  */
  Override_Mouse_Shape(MOUSE_NORMAL);  // display default mouse cursor
  bool process = true;                 // menu stays up while true
  while (process) {
    /*
    ................ Invoke game callback, to update music ................
    */
    Call_Back();

    /*
    ............................. Invoke menu .............................
    */
    Hide_Mouse();  // Do_Menu assumes the mouse is already hidden
    const int selection = Do_Menu(_menus, true);  // option the user picks
    Show_Mouse();
    // Esc, or a click away from the menu, closes it.
    if (const engine::window::InputEvent& unknown =
            TheGameState().unknown_input();
        unknown.key == KN_ESC ||
        unknown.IsPress(engine::window::MouseButton::kLeft) ||
        unknown.IsPress(engine::window::MouseButton::kRight)) {
      break;
    }

    /*
    .......................... Process selection ..........................
    */
    switch (selection) {
      /*
      ........................... New scenario ...........................
      */
      case 0:
        if (Changed) {
          rc = CCMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
          TheScreen().hidden_page().view().Clear();
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
          TheWorld().carry_over_money() = 0;
          Changed = true;
        }
        process = false;
        break;

      /*
      .......................... Load scenario ...........................
      */
      case 1:
        if (Changed) {
          rc = CCMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
          TheScreen().hidden_page().view().Clear();
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
          TheWorld().carry_over_money() = 0;
          Changed = false;
        }
        process = false;
        break;

      /*
      .......................... Save scenario ...........................
      */
      case 2:
        if (Save_Scenario() == 0) {
          Changed = false;
        }
        process = false;
        break;

      /*
      .......................... Edit map size ...........................
      */
      case 3:
        if (Size_Map(MapCellX, MapCellY, MapCellWidth, MapCellHeight) == 0) {
          process = false;
          Changed = true;
        }
        break;

      /*
      .......................... Add an object ...........................
      */
      case 4:
        if (Placement_Dialog() == 0) {
          Start_Placement();
          process = false;
        }
        break;

      /*
      ......................... Scenario options .........................
      */
      case 5:
        if (Scenario_Dialog() == 0) {
          Changed = true;
          process = false;
        }
        break;

      /*
      .......................... Other options ...........................
      */
      case 6:
        AI_Menu();
        process = false;
        break;

      /*
      ...................... Test-drive this scenario ....................
      */
      case 7:
        if (Changed) {
          rc = CCMessageBox().Process("Save Changes?", TXT_YES, TXT_NO);
          TheScreen().hidden_page().view().Clear();
          Flag_To_Redraw(true);
          Render();
          if (rc == 0) {
            if (Save_Scenario() != 0) {
              break;
            }
            Changed = false;
          }
        }
        Changed = false;
        TheDebugState().set_map_editor_active(false);
        Start_Scenario(TheWorld().scenario_name());
        return;
      default:
        break;
    }
  }

  /*------------------------------------------------------------------------
  Restore the display:
  - Clear HIDPAGE to erase any spurious drawing done by the menu system
  - Invoke Flag_To_Redraw to tell DisplayClass to re-render the whole screen
  - Invoke Redraw() to update the display
  ------------------------------------------------------------------------*/
  TheScreen().hidden_page().view().Clear();
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
  -------------------------- Fill in menu strings --------------------------
  */
  base::At(_menus, 0) = "Pre-Build a Base";
  base::At(_menus, 1) = "Import Triggers";
  base::At(_menus, 2) = "Edit Triggers";
  base::At(_menus, 3) = "Import Teams";
  base::At(_menus, 4) = "Edit Teams";
  base::At(_menus, 5) = nullptr;

  /*
  ----------------------------- Main Menu loop -----------------------------
  */
  Override_Mouse_Shape(MOUSE_NORMAL);  // display default mouse cursor
  bool process = true;                 // menu stays up while true
  while (process) {
    /*
    ................ Invoke game callback, to update music ................
    */
    Call_Back();

    /*
    ............................. Invoke menu .............................
    */
    Hide_Mouse();  // Do_Menu assumes the mouse is already hidden
    const int selection = Do_Menu(_menus, true);  // option the user picks
    Show_Mouse();
    // Esc, or a click away from the menu, closes it.
    if (const engine::window::InputEvent& unknown =
            TheGameState().unknown_input();
        unknown.key == KN_ESC ||
        unknown.IsPress(engine::window::MouseButton::kLeft) ||
        unknown.IsPress(engine::window::MouseButton::kRight)) {
      break;
    }

    /*
    .......................... Process selection ..........................
    */
    switch (selection) {
      /*
      ......................... Pre-Build a Base .........................
      */
      case 0:
        Start_Base_Building();
        process = false;
        break;

      /*
      ......................... Import Triggers ..........................
      */
      case 1:
        if (Import_Triggers() == 0) {
          process = false;
        }
        break;

      /*
      ......................... Trigger Editing ..........................
      */
      case 2:
        Handle_Triggers();
        /*
        ................ Go into trigger placement mode .................
        */
        if (CurTrigger) {
          Start_Trigger_Placement();
        }
        process = false;
        break;

      /*
      ........................... Import Teams ...........................
      */
      case 3:
        if (Import_Teams() == 0) {
          process = false;
        }
        break;

      /*
      ........................... Team Editing ...........................
      */
      case 4:
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
  --------------- Verify that new house can own this object ----------------
  */
  return (objtype->Get_Ownable() & base::Bit<uint32_t>(house)) != 0;
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
                                     const ObjectTypeClass* objtype) {

  /*------------------------------------------------------------------------
  Loop through all house types, starting with the one after 'curhouse';
  return the first one that's valid
  ------------------------------------------------------------------------*/
  HousesType count = HOUSE_NONE;  // prevents an infinite loop
  while (true) {
    /*
    .......................... Go to next house ...........................
    */
    curhouse++;
    if (curhouse == HOUSE_COUNT) {
      curhouse = HOUSE_FIRST;
    }

    /*
    ................ Count # iterations; don't go forever .................
    */
    count++;
    if (count == HOUSE_COUNT) {
      curhouse = HOUSE_NONE;
      break;
    }

    /*
    ................... Break if this is a valid house ....................
    */
    if (HouseClass::As_Pointer(curhouse) && Verify_House(curhouse, objtype)) {
      break;
    }
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
  ShutDown();
  absl::PrintF("%s\n", Text_String(txt));
  exit(EXIT_FAILURE);
}

bool MapEditClass::Scroll_Map(DirType facing, int& distance, bool really) {
  /*
  ** The popup gadgets require the entire map to be redrawn if we scroll.
  */
  if (TheDebugState().map_editor_active() && really) {
    Flag_To_Redraw(true);
  }

  return MouseClass::Scroll_Map(facing, distance, really);
}

void MapEditClass::Detach(ObjectClass* object) {
  if (GrabbedObject == object) {
    GrabbedObject = nullptr;
  }
}
