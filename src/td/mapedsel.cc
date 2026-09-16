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

/* $Header:   F:\projects\c&c\vcs\code\mapedsel.cpv   2.18   16 Oct 1995
 * 16:49:58   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDSEL.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : November 18, 1994                        *
 *                                                                         *
 *                  Last Update : February 2, 1995   [BR]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Object-selection & manipulation routines                                *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Select_Object -- selects an object for processing       *
 *   MapEditClass::Select_Next -- selects next object on the map           *
 *   MapEditClass::Popup_Controls -- shows/hides the pop-up object controls*
 *   MapEditClass::Grab_Object -- grabs the current object                 *
 *   MapEditClass::Move_Grabbed_Object -- moves the grabbed object         *
 *   MapEditClass::Change_House -- changes CurrentObject's house           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <array>
#include <cstdio>
#include <iterator>

#include "absl/strings/str_format.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "td/base.h"
#include "td/building.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dial8.h"
#include "td/display_constants.h"
#include "td/externs.h"
#include "td/gauge.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/inline.h"
#include "td/list.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/techno.h"
#include "td/type.h"
#include "td/vector.h"

/***************************************************************************
 * Select_Object -- selects an object for processing                       *
 *                                                                         *
 * INPUT:                                                                  *
 *   none.                                                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *   0 = object selected, -1 = none                                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *   none.                                                                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/04/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Select_Object() {
  ObjectClass* object = nullptr;  // Generic object clicked on.
  int rc = 0;

  /*
  -------------------- See if an object was clicked on ---------------------
  */
  int x = ActiveKeyboard->MouseQX;
  int y = ActiveKeyboard->MouseQY;

  /*
  ............................ Get cell for x,y ............................
  */
  CELL const cell = Click_Cell_Calc(x, y);  // Cell that was selected.

  /*
  ............... Convert x,y to offset from cell upper-left ...............
  */
  x = (x - TacPixelX) % ICON_PIXEL_W;
  y = (y - TacPixelY) % ICON_PIXEL_H;

  /*
  ......................... Get object at that x,y .........................
  */
  object = Cell_Object(cell, x, y);

  /*
  ----------------- If no object, unselect the current one -----------------
  */
  if (!object) {
    if (CurrentObject.Count()) {
      /*
      ................... Unselect all current objects ...................
      */
      Unselect_All();

      /*
      ..................... Turn off object controls .....................
      */
      Popup_Controls();
    }
    rc = -1;
  } else {
    /*
    ------------------ Select object only if it's different ------------------
    */
    if (!CurrentObject.Count() ||
        (CurrentObject.Count() && object != CurrentObject[0])) {
      /*
      ..................... Unselect all current objects ....................
      */
      Unselect_All();
      object->Select();

      /*
      ................... Set mouse shape back to normal ....................
      */
      Set_Default_Mouse(MOUSE_NORMAL);
      Override_Mouse_Shape(MOUSE_NORMAL);

      /*
      ....................... Show the popup controls .......................
      */
      Popup_Controls();
    }
  }

  /*
  -------------------------- Force map to redraw ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);

  return rc;
}

/***************************************************************************
 * MapEditClass::Select_Next -- selects next object on the map             *
 *                                                                         *
 * INPUT:                                                                  *
 *   none.                                                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *   none                                                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *   none.                                                                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/22/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Select_Next() {

  /*
  ----------------------- Get next object on the map -----------------------
  */
  ObjectClass* obj = MapEditClass::Next_Object(CurrentObject[0]);

  if (obj) {
    /*
    ............... Unselect current object if there is one ...............
    */
    Unselect_All();

    /*
    ......................... Select this object ..........................
    */
    obj->Select();
  }

  /*
  --------------------- Restore mouse shape to normal ----------------------
  */
  Set_Default_Mouse(MOUSE_NORMAL);
  Override_Mouse_Shape(MOUSE_NORMAL);

  /*
  -------------------------- Show pop-up controls --------------------------
  */
  Popup_Controls();

  /*
  ---------------- Make sure object is shown on the screen -----------------
  */
  /*
  ..................... compute screen map dimensions ......................
  */
  const int smap_w =
      Lepton_To_Cell(TacLeptonWidth);  // screen map width in icons
  const int smap_h =
      Lepton_To_Cell(TacLeptonHeight);  // screen map height in icons

  /*
  ...................... compute x,y of object's cell ......................
  */
  CELL const obj_cell = Coord_Cell(CurrentObject[0]->Coord);
  const int cell_x = Cell_X(obj_cell);       // cell-x of next object
  const int cell_y = Cell_Y(obj_cell);       // cell-y of next object
  int tcell_x = Coord_XCell(TacticalCoord);  // cell-x of TacticalCell
  int tcell_y = Coord_YCell(TacticalCoord);  // cell-y of TacticalCell

  /*
  ................... If object is off-screen, move map ....................
  */
  if (cell_x < tcell_x) {
    tcell_x = cell_x;
  } else {
    if (cell_x >= tcell_x + smap_w) {
      tcell_x = cell_x - smap_w + 1;
    }
  }

  if (cell_y < tcell_y) {
    tcell_y = cell_y;
  } else {
    if (cell_y >= tcell_y + smap_h) {
      tcell_y = cell_y - smap_h + 1;
    }
  }

  ScenarioInit++;
  Set_Tactical_Position(
      XY_Coord(Cell_To_Lepton(tcell_x), Cell_To_Lepton(tcell_y)));
  ScenarioInit--;

  /*
  -------------------------- Force map to redraw ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
}

/***************************************************************************
 * MapEditClass::Popup_Controls -- shows/hides the pop-up object controls  *
 *                                                                         *
 * Call this routine whenever the CurrentObject changes. The routine will  *
 * selectively enable or disable the popup controls based on whether       *
 * CurrentObject is NULL, or if it's a Techno object, or what type of      *
 * Techno object it is.                                                    *
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
 *   11/22/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Popup_Controls() {
  const TechnoTypeClass* objtype = nullptr;

  /*------------------------------------------------------------------------
  Remove all buttons from GScreen's button list (so none of them provide
  input any more); then, destroy the list by Zapping each button.  Then,
  we'll have to add at least the MapArea button back to the Input button
  list before we return, plus any other buttons to process input for.  We
  always must add MapArea LAST in the list, so it doesn't intercept the
  other buttons' input.
  ------------------------------------------------------------------------*/
  Remove_A_Button(*GDIButton);
  Remove_A_Button(*NODButton);
  Remove_A_Button(*NeutralButton);
  Remove_A_Button(*Multi1Button);
  Remove_A_Button(*Multi2Button);
  Remove_A_Button(*Multi3Button);
  Remove_A_Button(*Multi4Button);
  Remove_A_Button(*MissionList);
  Remove_A_Button(*HealthGauge);
  Remove_A_Button(*HealthText);
  Remove_A_Button(*FacingDial);
  Remove_A_Button(*BaseGauge);
  Remove_A_Button(*BaseLabel);
  Remove_A_Button(*MapArea);

  /*
  ------------------ If no current object, hide the list -------------------
  */
  if (!CurrentObject.Count()) {
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
    return;
  }

  /*
  --------------- If not Techno, no need for editing buttons ---------------
  */
  if (!CurrentObject[0]->Is_Techno()) {
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
    return;
  }

  objtype = dynamic_cast<const TechnoTypeClass*>(&CurrentObject[0]->Class_Of());
  const auto* techno = dynamic_cast<const TechnoClass*>(CurrentObject[0]);

  /*
  ---------------------- Get object's current values -----------------------
  */
  const HousesType owner = CurrentObject[0]->Owner();  // object's current owner
  int mission_index = 0;  // object's current mission
  // Not std::views::enumerate: Apple's libc++ does not ship it yet.
  // Distance rather than an iterator variable: std::array's iterator is a
  // pointer in libstdc++ and libc++ but a class in MSVC's STL.
  const auto index = std::ranges::distance(
      MapEditMissions.begin(),
      std::ranges::find(MapEditMissions, CurrentObject[0]->Get_Mission()));
  if (index < std::ssize(MapEditMissions)) {
    mission_index = static_cast<int>(index);
  }
  const int strength =
      CurrentObject[0]->Health_Ratio();  // object's 0-255 strength value

  /*
  ----------------------------- House buttons ------------------------------
  */
  if (ScenPlayer == SCEN_PLAYER_MPLAYER) {
    if (Verify_House(HOUSE_NEUTRAL, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*NeutralButton);
    }
    if (Verify_House(HOUSE_MULTI1, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*Multi1Button);
    }
    if (Verify_House(HOUSE_MULTI2, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*Multi2Button);
    }
    if (Verify_House(HOUSE_MULTI3, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*Multi3Button);
    }
    if (Verify_House(HOUSE_MULTI4, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*Multi4Button);
    }
  } else {
    if (Verify_House(HOUSE_NEUTRAL, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*NeutralButton);
    }
    if (Verify_House(HOUSE_BAD, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*NODButton);
    }
    if (Verify_House(HOUSE_GOOD, &CurrentObject[0]->Class_Of())) {
      Add_A_Button(*GDIButton);
    }
  }

  /*
  ........................ Set house button states .........................
  */
  if (Buttons) {
    Set_House_Buttons(owner, Buttons, kPopupGdi);
  }

  switch (objtype->What_Am_I()) {
    case RTTI_UNITTYPE:
    case RTTI_INFANTRYTYPE:
    case RTTI_AIRCRAFTTYPE:
      MissionList->Set_Selected_Index(mission_index);
      HealthGauge->Set_Value(strength);
      absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d",
                     CurrentObject[0]->Strength);
      FacingDial->Set_Direction(techno->PrimaryFacing);

      /*
      **	Make the list.
      */
      Add_A_Button(*MissionList);
      Add_A_Button(*HealthGauge);
      Add_A_Button(*HealthText);
      Add_A_Button(*FacingDial);
      break;

    case RTTI_BUILDINGTYPE:
      HealthGauge->Set_Value(strength);
      absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d",
                     CurrentObject[0]->Strength);
      Add_A_Button(*HealthGauge);
      Add_A_Button(*HealthText);

      if (objtype->IsTurretEquipped) {
        FacingDial->Set_Direction(techno->PrimaryFacing);
        Add_A_Button(*FacingDial);
      }
      break;
    default:
      break;
  }

  /*------------------------------------------------------------------------
  Add the map area last, so it's "underneath" the other buttons, and won't
  intercept input for those buttons.
  ------------------------------------------------------------------------*/
  Add_A_Button(*BaseGauge);
  Add_A_Button(*BaseLabel);
  Add_A_Button(*MapArea);
}

/***************************************************************************
 * MapEditClass::Grab_Object -- grabs the current object                   *
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
 *   11/07/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Grab_Object() {

  if (CurrentObject.Count()) {
    GrabbedObject = CurrentObject[0];

    /*------------------------------------------------------------------------
    Find out which cell 'ZoneCell' is in relation to the object's current cell
    ------------------------------------------------------------------------*/
    CELL const cell = Coord_Cell(GrabbedObject->Coord);
    GrabOffset = static_cast<CELL>(cell - ZoneCell);
  }
}

/***************************************************************************
 * MapEditClass::Move_Grabbed_Object -- moves the grabbed object           *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = object moved, -1 = it didn't                                   *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Move_Grabbed_Object() {
  COORDINATE new_coord = 0;
  int retval = -1;

  /*
  --------------------------- Lift up the object ---------------------------
  */
  GrabbedObject->Mark(MARK_UP);

  /*------------------------------------------------------------------------
  If infantry, use a free spot in this cell
  ------------------------------------------------------------------------*/
  if (GrabbedObject->Is_Infantry()) {
    if (Is_Spot_Free(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()))) {
      new_coord =
          Closest_Free_Spot(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
      /*..................................................................
      Clear the occupied bit in this infantry's cell.
      ..................................................................*/
      dynamic_cast<InfantryClass*>(GrabbedObject)
          ->Clear_Occupy_Bit(GrabbedObject->Coord);
      //			Map[Coord_Cell(GrabbedObject->Coord)].Flag.Composite
      //&=
      //				~(1 <<
      // CellClass::Spot_Index(GrabbedObject->Coord));
    } else {
      new_coord = 0;
    }

  } else {
    /*------------------------------------------------------------------------
    Non-infantry: use cell's center coordinate
    ------------------------------------------------------------------------*/
    new_coord = Cell_Coord(static_cast<CELL>(ZoneCell + GrabOffset));

    if (GrabbedObject->What_Am_I() == RTTI_BUILDING ||
        GrabbedObject->What_Am_I() == RTTI_TERRAIN) {
      new_coord &= 0xFF00FF00L;
    }

    /*
    ................ Try to place object at new coordinate ................
    */
    if (GrabbedObject->Can_Enter_Cell(Coord_Cell(new_coord)) != MOVE_OK) {
      new_coord = 0;
    }
  }
  if (new_coord != 0) {
    /*
    ** If this object is part of the AI's Base list, change the coordinate
    ** in the Base's Node list.
    */
    if (GrabbedObject->What_Am_I() == RTTI_BUILDING) {
      BaseNodeClass* node =
          Base.Get_Node(dynamic_cast<BuildingClass*>(GrabbedObject));
      if (node != nullptr) {
        node->Coord = new_coord;
      }
    }

    GrabbedObject->Coord = new_coord;
    retval = 0;
  }
  GrabbedObject->Mark(MARK_DOWN);

  /*------------------------------------------------------------------------
  For infantry, set the bit in its new cell marking that spot as occupied.
  ------------------------------------------------------------------------*/
  if (GrabbedObject->Is_Infantry()) {
    dynamic_cast<InfantryClass*>(GrabbedObject)->Set_Occupy_Bit(new_coord);
    //		Map[Coord_Cell(new_coord)].Flag.Composite |=
    //			(1 << CellClass::Spot_Index(new_coord));
  }

  /*------------------------------------------------------------------------
  Re-select the object, and reset the mouse pointer
  ------------------------------------------------------------------------*/
  Set_Default_Mouse(MOUSE_NORMAL);
  Override_Mouse_Shape(MOUSE_NORMAL);

  Flag_To_Redraw(true);

  return retval;
}

/***************************************************************************
 * MapEditClass::Change_House -- changes CurrentObject's house             *
 *                                                                         *
 * INPUT:                                                                  *
 *      newhouse      house to change to                                   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = house was changed, 0 = it wasn't                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/17/1994 BR : Created.                                              *
 *=========================================================================*/
bool MapEditClass::Change_House(HousesType newhouse) {

  /*------------------------------------------------------------------------
  Return if no current object
  ------------------------------------------------------------------------*/
  if (!CurrentObject.Count()) {
    return false;
  }

  /*------------------------------------------------------------------------
  Only techno objects can be owned by a house; return if not a techno
  ------------------------------------------------------------------------*/
  if (!CurrentObject[0]->Is_Techno()) {
    return false;
  }

  /*------------------------------------------------------------------------
  You can't change the house if the object is part of the AI's Base.
  ------------------------------------------------------------------------*/
  if (CurrentObject[0]->What_Am_I() == RTTI_BUILDING &&
      Base.Is_Node(dynamic_cast<BuildingClass*>(CurrentObject[0]))) {
    return false;
  }

  /*------------------------------------------------------------------------
  Verify that the target house exists
  ------------------------------------------------------------------------*/
  if (HouseClass::As_Pointer(newhouse) == nullptr) {
    return false;
  }

  /*------------------------------------------------------------------------
  Verify that this is a valid owner
  ------------------------------------------------------------------------*/
  if (!Verify_House(newhouse, &CurrentObject[0]->Class_Of())) {
    return false;
  }

  /*------------------------------------------------------------------------
  Change the house
  ------------------------------------------------------------------------*/
  auto* tp = dynamic_cast<TechnoClass*>(CurrentObject[0]);
  tp->House = HouseClass::As_Pointer(newhouse);

  return true;
}
