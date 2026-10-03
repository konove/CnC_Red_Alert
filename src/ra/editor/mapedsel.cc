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

/* $Header: /CounterStrike/MAPEDSEL.CPP 1     3/03/97 10:25a Joe_bostic $ */
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
 *                  Last Update : April 30, 1996 [JLB]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Object-selection & manipulation routines                                *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Change_House -- changes CurrentObject's house           *
 *   MapEditClass::Grab_Object -- grabs the current object                 *
 *   MapEditClass::Move_Grabbed_Object -- moves the grabbed object         *
 *   MapEditClass::Popup_Controls -- shows/hides the pop-up object controls*
 *   MapEditClass::Select_Next -- selects next object on the map           *
 *   MapEditClass::Select_Object -- selects an object for processing       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <array>
#include <cstdint>
#include <cstdio>
#include <iterator>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "game/core/cell_size.h"
#include "game/sim/enums.h"
#include "game/ui/input.h"
#include "ra/ai/base.h"
#include "ra/assets/screen.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/editor/mapedit.h"
#include "ra/map/selection.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/ui/direction_dial.h"
#include "ra/ui/list.h"

namespace ra {

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
int MapEditClass::Select_Object(int x, int y) {
  ObjectClass* object = nullptr;  // Generic object clicked on.
  int rc = 0;

  /*
  **	Get cell for x,y
  */
  const Cell cell = Click_Cell_Calc(x, y);  // Cell that was selected.

  /*
  **	Convert x,y to offset from cell upper-left
  */
  x = (x - TacPixelX) % game::kCellWidthPixels;
  y = (y - TacPixelY) % game::kCellHeightPixels;

  /*
  **	Get object at that x,y
  */
  object = Cell_Object(cell, x, y);

  /*
  **	If no object, unselect the current one
  */
  if (!object) {
    if (std::ssize(TheWorld().current_object())) {
      /*
      **	Unselect all current objects
      */
      Unselect_All();

      /*
      **	Turn off object controls
      */
      Popup_Controls();
    }
    rc = -1;
  } else {
    /*
    **	Select object only if it's different
    */
    if (!std::ssize(TheWorld().current_object()) ||
        (std::ssize(TheWorld().current_object()) &&
         object != base::At(TheWorld().current_object(), 0))) {
      /*
      **	Unselect all current objects
      */
      Unselect_All();
      object->Select();

      /*
      **	Set mouse shape back to normal
      */
      Set_Default_Mouse(MOUSE_NORMAL);
      Override_Mouse_Shape(MOUSE_NORMAL);

      /*
      **	Show the popup controls
      */
      Popup_Controls();
    }
  }

  /*
  **	Force map to redraw
  */
  TheScreen().hidden_view().Clear();
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
  **	Get next object on the map
  */
  ObjectClass* obj =
      MapEditClass::Next_Object(TheWorld().current_object().empty()
                                    ? nullptr
                                    : base::At(TheWorld().current_object(), 0));

  if (obj) {
    /*
    **	Unselect current object if there is one
    */
    Unselect_All();

    /*
    **	Select this object
    */
    obj->Select();
  }

  /*
  **	Restore mouse shape to normal
  */
  Set_Default_Mouse(MOUSE_NORMAL);
  Override_Mouse_Shape(MOUSE_NORMAL);

  /*
  **	Show pop-up controls
  */
  Popup_Controls();

  // Nothing on the map can be selected, so there is nothing to scroll to.
  if (TheWorld().current_object().empty()) {
    return;
  }

  /*
  **	Make sure object is shown on the screen
  */
  /*
  **	compute screen map dimensions
  */
  const int smap_w =
      Lepton_To_Cell(TacLeptonWidth);  // screen map width in icons
  const int smap_h =
      Lepton_To_Cell(TacLeptonHeight);  // screen map height in icons

  /*
  **	compute x,y of object's cell
  */
  const Cell obj_cell =
      Cell::Containing(base::At(TheWorld().current_object(), 0)->Coord);
  const int cell_x = obj_cell.x();           // cell-x of next object
  const int cell_y = obj_cell.y();           // cell-y of next object
  int tcell_x = TacticalCoord.cell_x();      // cell-x of TacticalCell
  int tcell_y = TacticalCoord.cell_y();      // cell-y of TacticalCell

  /*
  **	If object is off-screen, move map
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

  TheWorld().scenario_init()++;
  Set_Tactical_Position(Coordinate::FromLeptons(Cell_To_Lepton(tcell_x),
                                                Cell_To_Lepton(tcell_y)));
  TheWorld().scenario_init()--;

  /*
  **	Force map to redraw
  */
  TheScreen().hidden_view().Clear();
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
 *   04/30/1996 JLB : Revamped for new buttons and stuff.                  *
 *=========================================================================*/
void MapEditClass::Popup_Controls() {
  const TechnoTypeClass* objtype = nullptr;

  /*
  **	Remove all buttons from GScreen's button list (so none of them provide
  **	input any more); then, destroy the list by Zapping each button.  Then,
  **	we'll have to add at least the MapArea button back to the Input button
  **	list before we return, plus any other buttons to process input for.  We
  **	always must add MapArea LAST in the list, so it doesn't intercept the
  **	other buttons' input.
  */
  Remove_A_Button(*HouseList);
  Remove_A_Button(*MissionList);
  Remove_A_Button(*HealthGauge);
  Remove_A_Button(*HealthText);
  Remove_A_Button(*FacingDial);
  Remove_A_Button(*BaseGauge);
  Remove_A_Button(*BaseLabel);
  Remove_A_Button(*MapArea);

  Remove_A_Button(*Sellable);
  Remove_A_Button(*Rebuildable);

  /*
  **	If no current object, hide the list
  */
  if (!std::ssize(TheWorld().current_object())) {
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
    return;
  }

  /*
  **	If not Techno, no need for editing buttons
  */
  if (!base::At(TheWorld().current_object(), 0)->Is_Techno()) {
    Add_A_Button(*BaseGauge);
    Add_A_Button(*BaseLabel);
    Add_A_Button(*MapArea);
    return;
  }

  objtype = dynamic_cast<const TechnoTypeClass*>(
      &base::At(TheWorld().current_object(), 0)->Class_Of());
  const auto* techno = dynamic_cast<const TechnoClass*>(
      base::At(TheWorld().current_object(), 0));

  /*
  **	Get object's current values
  */
  const HousesType owner = base::At(TheWorld().current_object(), 0)
                               ->Owner();  // object's current owner
  int mission_index = 0;  // object's current mission
  for (int i = 0; i < std::ssize(MapEditMissions); i++) {
    if (base::At(TheWorld().current_object(), 0)->Get_Mission() ==
        MapEditMissions.at(base::ToSize(i))) {
      mission_index = i;
    }
  }
  const int strength =
      base::At(TheWorld().current_object(), 0)->Health_Ratio() *
      256;  // object's 0-255 strength value

  switch (objtype->What_Am_I()) {
    case RTTI_VESSELTYPE:
    case RTTI_UNITTYPE:
    case RTTI_INFANTRYTYPE:
    case RTTI_AIRCRAFTTYPE:
      MissionList->Set_Selected_Index(mission_index);
      HealthGauge->Set_Value(strength);
      absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d",
                     base::At(TheWorld().current_object(), 0)->Strength);
      FacingDial->set_direction(techno->PrimaryFacing);

      /*
      **	Make the list.
      */
      Add_A_Button(*HealthGauge);
      Add_A_Button(*HouseList);
      HouseList->Set_Selected_Index(static_cast<int>(owner));
      Add_A_Button(*MissionList);
      Add_A_Button(*HealthText);
      Add_A_Button(*FacingDial);
      break;

    case RTTI_BUILDINGTYPE: {
      const auto& building = dynamic_cast<const BuildingClass&>(
          *TheWorld().current_object().at(0));
      HealthGauge->Set_Value(strength);
      absl::SNPrintF(HealthBuf, sizeof(HealthBuf), "%d",
                     base::At(TheWorld().current_object(), 0)->Strength);
      Add_A_Button(*HealthGauge);
      Add_A_Button(*HouseList);
      HouseList->Set_Selected_Index(static_cast<int>(owner));
      Add_A_Button(*HealthText);

      Add_A_Button(*Sellable);
      if (building.IsAllowedToSell) {
        Sellable->Turn_On();
      } else {
        Sellable->Turn_Off();
      }
      Add_A_Button(*Rebuildable);
      if (building.IsToRebuild) {
        Rebuildable->Turn_On();
      } else {
        Rebuildable->Turn_Off();
      }

      if (objtype->IsTurretEquipped) {
        FacingDial->set_direction(techno->PrimaryFacing);
        Add_A_Button(*FacingDial);
      }
      break;
    }
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
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNIT:
    case RTTIType::RTTI_VESSEL:
    default:
      break;
  }

  /*
  **	Add the map area last, so it's "underneath" the other buttons, and won't
  **	intercept input for those buttons.
  */
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
  if (std::ssize(TheWorld().current_object())) {
    GrabbedObject = base::At(TheWorld().current_object(), 0);

    /*
    **	Find out which cell 'ZoneCell' is in relation to the object's current
    *cell
    */
    const Cell cell = Cell::Containing(GrabbedObject->Coord);
    GrabOffset = static_cast<int16_t>(cell.index() - ZoneCell.index());
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
  Coordinate new_coord{};
  int retval = -1;

  /*
  **	Lift up the object
  */
  GrabbedObject->Mark(MARK_UP);

  /*
  **	If infantry, use a free spot in this cell
  */
  if (GrabbedObject->Is_Infantry()) {
    if (Is_Spot_Free(
            Pixel_To_Coord(game::TheMouse().x(), game::TheMouse().y()))) {
      new_coord = Closest_Free_Spot(
          Pixel_To_Coord(game::TheMouse().x(), game::TheMouse().y()));

      /*
      **	Clear the occupied bit in this infantry's cell.
      */
      dynamic_cast<InfantryClass*>(GrabbedObject)
          ->Clear_Occupy_Bit(GrabbedObject->Coord);
    } else {
      new_coord = Coordinate{};
    }

  } else {
    /*
    **	Non-infantry: use cell's center coordinate
    */
    new_coord = ZoneCell.Offset(GrabOffset).Center();

    if (GrabbedObject->What_Am_I() == RTTI_BUILDING ||
        GrabbedObject->What_Am_I() == RTTI_TERRAIN) {
      new_coord = new_coord.Whole();
    }

    /*
    **	Try to place object at new coordinate
    */
    if (GrabbedObject->Can_Enter_Cell(Cell::Containing(new_coord)) !=
        game::MOVE_OK) {
      new_coord = Coordinate{};
    }
  }
  if (new_coord) {
    /*
    ** If this object is part of the AI's Base list, change the coordinate
    ** in the Base's Node list.
    */
    if (GrabbedObject->What_Am_I() == RTTI_BUILDING) {
      BaseNode* node = TheWorld().base().FindNode(
          dynamic_cast<const BuildingClass*>(GrabbedObject));
      if (node != nullptr) {
        node->cell = Cell::Containing(new_coord);
      }
    }

    GrabbedObject->Coord = new_coord;
    retval = 0;
  }
  GrabbedObject->Mark(MARK_DOWN);

  /*
  **	For infantry, set the bit in its new cell marking that spot as occupied.
  */
  if (GrabbedObject->Is_Infantry()) {
    dynamic_cast<InfantryClass*>(GrabbedObject)->Set_Occupy_Bit(new_coord);
  }

  /*
  **	Re-select the object, and reset the mouse pointer
  */
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

  /*
  **	Return if no current object
  */
  if (!std::ssize(TheWorld().current_object())) {
    return false;
  }

  /*
  **	Only techno objects can be owned by a house; return if not a techno
  */
  if (!base::At(TheWorld().current_object(), 0)->Is_Techno()) {
    return false;
  }

  /*
  **	You can't change the house if the object is part of the AI's Base.
  */
  if (base::At(TheWorld().current_object(), 0)->What_Am_I() == RTTI_BUILDING &&
      TheWorld().base().HasNodeFor(dynamic_cast<const BuildingClass*>(
          base::At(TheWorld().current_object(), 0)))) {
    return false;
  }

  /*
  **	Verify that the target house exists
  */
  if (HouseClass::As_Pointer(newhouse) == nullptr) {
    return false;
  }

  /*
  **	Verify that this is a valid owner
  */
  //	if (!Verify_House(newhouse, &CurrentObject[0]->Class_Of())) {
  //		return(false);
  //	}

  /*
  **	Change the house
  */
  auto* tp =
      dynamic_cast<TechnoClass*>(base::At(TheWorld().current_object(), 0));
  tp->House = HouseClass::As_Pointer(newhouse);

  tp->IsOwnedByPlayer = false;
  if (tp->House == ThePlayer()) {
    tp->IsOwnedByPlayer = true;
  }

  return true;
}

}  // namespace ra
