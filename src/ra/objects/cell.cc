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

/* $Header: /CounterStrike/CELL.CPP 4     3/14/97 1:15p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CELL.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 29, 1994 *
 *                                                                                             *
 *                  Last Update : October 6, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CellClass::Adjacent_Offset -- Determines the offset to the
 * adjacent cell according to facing.             * CellClass::Adjust_Threat --
 * Allows adjustment of threat at cell level                     *
 *   CellClass::Can_Tiberium_Germinate -- Determines if Tiberium can begin
 *growth in the cell. * CellClass::Can_Tiberium_Grow -- Determines if Tiberium
 *can grow in this cell.             * CellClass::Can_Tiberium_Spread --
 *Determines if Tiberium can spread from this cell.       * CellClass::CellClass
 *-- Constructor for cell objects.                                     *
 *   CellClass::Cell_Building -- Return with building at specified cell. *
 *   CellClass::Cell_Color   -- Determine what radar color to use for this cell.
 ** CellClass::Center_Coord -- Returns the coordinate of this cell. *
 *   CellClass::Cell_Find_Object -- Returns ptr to RTTI type occupying cell *
 *   CellClass::Cell_Infantry -- Returns with pointer of first infantry unit. *
 *   CellClass::Cell_Object -- Returns with clickable object in cell. *
 *   CellClass::Cell_Techno -- Return with the unit/building at specified cell.
 ** CellClass::Cell_Terrain -- Determines terrain object in cell. *
 *   CellClass::Cell_Unit -- Returns with pointer to unit occupying cell. *
 *   CellClass::Cell_Vessel -- Returns with pointer to a vessel located in the
 *cell.           * CellClass::Clear_Icon -- Calculates what the clear icon
 *number should be.                 * CellClass::Closest_Free_Spot -- returns
 *free spot closest to given coord                  * CellClass::Concrete_Calc
 *-- Calculates the concrete icon to use for the cell.             *
 *   CellClass::Draw_It -- Draws the cell imagery at the location specified. *
 *   CellClass::Flag_Place -- Places a house flag down on the cell. *
 *   CellClass::Flag_Remove -- Removes the house flag from the cell. *
 *   CellClass::Goodie_Check -- Performs crate discovery logic. *
 *   CellClass::Grow_Tiberium -- Grows the tiberium in the cell. *
 *   CellClass::Incoming -- Causes objects in cell to "run for cover". *
 *   CellClass::Is_Bridge_Here -- Checks to see if this is a bridge occupied
 *cell.             * CellClass::Is_Clear_To_Build -- Determines if cell can be
 *built upon.                     * CellClass::Is_Clear_To_Move -- Determines if
 *the cell is generally clear for travel       * CellClass::Occupy_Down -- Flag
 *occupation of specified cell.                              *
 *   CellClass::Occupy_Up -- Removes occupation flag from the specified cell. *
 *   CellClass::Overlap_Down -- This routine is used to mark a cell as being
 *spilled over (over* CellClass::Overlap_Unit -- Marks cell as being overlapped
 *by unit.                        * CellClass::Overlap_Up -- Removes overlap
 *flag for the cell.                               * CellClass::Read -- Reads a
 *particular cell value from a save game file.                   *
 *   CellClass::Recalc_Attributes -- Recalculates the ground type attributes for
 *the cell.     * CellClass::Redraw_Objects -- Redraws all objects overlapping
 *this cell.                   * CellClass::Reduce_Tiberium -- Reduces the
 *tiberium in the cell by the amount specified.   * CellClass::Reduce_Wall --
 *Damages a wall, if damage is high enough.                       *
 *   CellClass::Reserve_Cell -- Marks a cell as being occupied by the specified
 *unit ID.       * CellClass::Shimmer -- Causes all objects in the cell to
 *shimmer.                          * CellClass::Spot_Index -- returns cell
 *sub-coord index for given Coordinate                *
 *   CellClass::Spread_Tiberium -- Spread Tiberium from this cell to an adjacent
 *cell.         * CellClass::Tiberium_Adjust -- Adjust the look of the Tiberium
 *for smooth.                 * CellClass::Wall_Update -- Updates the imagery
 *for wall objects in cell.                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/cell.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "game/core/display_constants.h"
#include "game/core/game_clock.h"
#include "game/sim/direction.h"
#include "game/sim/utracker.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigtype.h"
#include "ra/assets/audio.h"
#include "ra/assets/palettes.h"
#include "ra/assets/shape_draw.h"
#include "ra/assets/window_rect.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/core/target.h"
#include "ra/data/rules.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/display.h"
#include "ra/map/layer.h"
#include "ra/map/logic.h"
#include "ra/map/vortex.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/combat.h"
#include "ra/objects/foot.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/object.h"
#include "ra/objects/overlay.h"
#include "ra/objects/super.h"
#include "ra/objects/techno.h"
#include "ra/objects/terrain.h"
#include "ra/objects/tracker.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"
#include "ra/screens/score.h"
#include "ra/ui/dialog.h"

namespace ra {

using engine::gfx::ShapeFlags;

/***********************************************************************************************
 * CellClass::CellClass -- Constructor for cell objects. *
 *                                                                                             *
 *    A cell object is constructed into an empty state. It contains no specific
 *objects,       * templates, or overlays. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/09/1994 JLB : Created. * 02/20/1996 JLB : Uses initializer
 *list.                                                   *
 *=============================================================================================*/
CellClass::CellClass() : ID(static_cast<int16_t>(TheMap().ID(this))) {
  for (const MZoneType zone : magic_enum::enum_values<MZoneType>()) {
    Zones.at(zone) = 0;
  }
  Flag.Composite = 0;
  for (int index = 0; index < std::ssize(Overlappers); index++) {
    base::At(Overlappers, index) = nullptr;
  }
}

/***********************************************************************************************
 * CellClass::Cell_Color   -- Determine what radar color to use for this cell. *
 *                                                                                             *
 *    Use this routine to determine what radar color to render a radar * pixel
 *with. This routine is called many many times to render the * radar map, so it
 *must be fast.                                                           *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the color to display the radar pixel with. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/01/1994 JLB : Created. * 04/30/1994 JLB : Converted to member
 *function.                                            * 05/31/1994 JLB : Takes
 *into account any stealth characteristics of object.                *
 *=============================================================================================*/
int CellClass::Cell_Color(bool override) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const BuildingClass* object = Cell_Building();
  if (object && !object->Class->IsInvisible) {
    return ThePalettes().color_remaps().at(object->House->RemapColor).Bar;
  }

  if (override) {
    return kTBlack;
  }
  if (TheWorld().last_theater() == THEATER_SNOW) {
    return SnowColor.at(Land_Type());
  }
  return GroundColor.at(Land_Type());
}

/***********************************************************************************************
 * CellClass::Cell_Techno -- Return with the unit/building at specified cell. *
 *                                                                                             *
 *    Returns an object located in the cell. If there is a * building present,
 *it returns a pointer to that, otherwise it returns                     * a
 *pointer to one of the units there. If nothing is present in the * specified
 *cell, then it returns nullptr. *
 *                                                                                             *
 * INPUT:   x,y   -- Coordinate offset (from upper left corner) to use as an aid
 *in selecting  * the desired object within the cell. *
 *                                                                                             *
 * OUTPUT:  Returns a pointer to a building or unit located in cell. If *
 *          nothing present, just returns nullptr. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. * 04/30/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
TechnoClass* CellClass::Cell_Techno(int x, int y) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  TechnoClass* close = nullptr;
  int32_t distance = 0;  // Recorded closest distance.

  /*
  **	Create a coordinate value that represent the pixel location within the
  *cell. This is *	actually the lower significant bits (leptons) of a
  *regular coordinate value.
  */
  const Coordinate click = Coordinate::FromLeptons(
      Pixel_To_Lepton(x),
      Pixel_To_Lepton(y));  // Coordinate of click relative to cell corner.

  if (Cell_Occupier()) {
    ObjectClass* object = Cell_Occupier();
    while (object) {
      if (object->Is_Techno()) {
        const Coordinate coord = object->Center_Coord().Fraction();
        const int32_t dist = Distance(coord, click);
        if (!close || dist < distance) {
          close = dynamic_cast<TechnoClass*>(object);
          distance = dist;
        }
      }
      object = object->Next;
    }
  }
  return close;
}

/***************************************************************************
 * CellClass::Cell_Find_Object -- Returns ptr to RTTI type occupying cell  *
 *                                                                         *
 * INPUT:		RTTIType the RTTI type we are searching for *
 *                                                                         *
 * OUTPUT:		none *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   03/17/1995 PWG : Created.                                             *
 *   06/12/1995 JLB : Returns object class pointer.                        *
 *=========================================================================*/
ObjectClass* CellClass::Cell_Find_Object(RTTIType rtti) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  DCHECK(rtti != RTTI_NONE);

  ObjectClass* object = Cell_Occupier();

  while (object != nullptr) {
    if (object->What_Am_I() == rtti) {
      return object;
    }
    object = object->Next;
  }
  return nullptr;
}

/***********************************************************************************************
 * CellClass::Cell_Building -- Return with building at specified cell. *
 *                                                                                             *
 *    Given a cell, determine if there is a building associated * and return
 *with a pointer to this building. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the building associated with the * cell.
 *If there is no building associated, then nullptr is   * returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. * 04/30/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
BuildingClass* CellClass::Cell_Building() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return dynamic_cast<BuildingClass*>(Cell_Find_Object(RTTI_BUILDING));
}

/***********************************************************************************************
 * CellClass::Cell_Terrain -- Determines terrain object in cell. *
 *                                                                                             *
 *    This routine is used to determine the terrain object (if any) that *
 *    overlaps this cell. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the terrain object that overlaps * this
 *cell. If there is no terrain object present, then nullptr * is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1994 JLB : Created. *
 *=============================================================================================*/
TerrainClass* CellClass::Cell_Terrain() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return dynamic_cast<TerrainClass*>(Cell_Find_Object(RTTI_TERRAIN));
}

/***********************************************************************************************
 * CellClass::Cell_Object -- Returns with clickable object in cell. *
 *                                                                                             *
 *    This routine is used to determine which object is to be selected * by a
 *player click upon the cell. Not all objects that overlap the * cell are
 *selectable by the player. This routine sorts out which * is which and returns
 *with the appropriate object pointer.                                *
 *                                                                                             *
 * INPUT:   x,y   -- Coordinate (from upper left corner of cell) to use as a
 *guide when        * selecting the object within the cell. This plays a role in
 *those cases    * where several objects (such as infantry) exist within the
 *same cell.      *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to the object clickable within the * cell.
 * nullptr is returned if there is no clickable object    * present. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/13/1994 JLB : Created. *
 *=============================================================================================*/
ObjectClass* CellClass::Cell_Object(int x, int y) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  /*
  **	Hack so that aircraft landed on helipads can still be selected if
  *directly *	clicked on.
  */
  ObjectClass* ptr = Cell_Find_Object(RTTI_AIRCRAFT);
  if (ptr) {
    return ptr;
  }

  ptr = Cell_Techno(x, y);
  if (ptr) {
    return ptr;
  }
  ptr = Cell_Terrain();
  if (ptr) {
    return ptr;
  }
  return ptr;
}

// Besides flagging the cell, this cuts the occupier chain at an object that
// died without leaving it. That cleanup is game state (saves hold the chain),
// so it keeps happening exactly where it always has: in cells that are in view
// and not flagged yet. Not const: it changes the map's redraw state and the
// chain.
// NOLINTNEXTLINE(readability-make-member-function-const)
void CellClass::Redraw_Objects(bool forced) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const Cell cell = Cell_Number();
  if (!TheMap().In_View(cell) ||
      (!forced && MapEditClass::Is_Cell_Flagged(cell))) {
    return;
  }
  TheMap().Flag_Cell(cell);

  for (ObjectClass* optr = Cell_Occupier(); optr != nullptr && optr->IsActive;
       optr = optr->Next) {
    if (optr->Next != nullptr && !optr->Next->IsActive) {
      optr->Next = nullptr;
    }
  }
}

/***********************************************************************************************
 * CellClass::Is_Clear_To_Build -- Determines if cell can be built upon. *
 *                                                                                             *
 *    This determines if the cell can become a proper foundation for * building
 *placement. *
 *                                                                                             *
 * INPUT:   loco     -- The locomotion of the object trying to consider if this
 *cell is        * generally clear. Buildings use the value of SPEED_NONE. *
 *                                                                                             *
 * OUTPUT:  bool; Is this cell generally clear (usually for building purposes)?
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/18/1994 JLB : Created. * 06/25/1996 JLB : Handles different
 *locomotion types.                                      * 10/05/1996 JLB :
 *Checks for crushable walls and crushable object.                         *
 *=============================================================================================*/
bool CellClass::Is_Clear_To_Build(SpeedType loco) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  /*
  **	During scenario initialization, passability is always guaranteed.
  */
  if (TheWorld().scenario_init()) {
    return true;
  }

  /*
  **	If there is an object there, then don't allow building.
  */
  if (Cell_Object() != nullptr) {
    return false;
  }

  /*
  **	Prevents a building from being placed over a flag object.
  */
  if (IsFlagged) {
    return false;
  }

  /*
  **	Walls are always considered to block the terrain for general passability
  **	purposes. In normal game mode, all overlays are not buildable.
  */
  if (Overlay != OVERLAY_NONE &&
      (Overlay == OVERLAY_FLAG_SPOT || !TheDebugState().map_editor_active() ||
       OverlayTypeClass::As_Reference(Overlay).IsWall)) {
    return false;
  }

  /*
  **	Building over a bib is not allowed.
  */
  if (Smudge != SMUDGE_NONE &&
      SmudgeTypeClass::As_Reference(Smudge).IsBib /* && Owner != HOUSE_NONE*/) {
    return false;
  }

  /*
  **	Building on certain kinds of terrain is prohibited -- bridges in
  *particular. *	If the locomotion type is SPEED_NONE, then this check is
  *presumed to be *	for the purposes of building.
  */
  if (loco == SPEED_NONE) {
    if (Is_Bridge_Here()) {
      return false;
    }

    return TheRules().ground().at(Land_Type()).Build;
  }
  if (TheRules().ground().at(Land_Type()).Cost.at(loco) == Fixed(0)) {
    return false;
  }
  return true;
}

/***********************************************************************************************
 * CellClass::Recalc_Attributes -- Recalculates the ground type attributes for
 *the cell.       *
 *                                                                                             *
 *    This routine recalculates the ground type in the cell. The speeds the find
 *path          * algorithm and other determinations of the cell type. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/29/1994 JLB : Created. * 06/20/1994 JLB : Knows about template
 *pointer in cell object.                             *
 *=============================================================================================*/
void CellClass::Recalc_Attributes() {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  /*
  **	Special override for interior terrain set so that a non-template or a
  *clear template *	is equivalent to impassable rock.
  */
  if ((TheWorld().last_theater() == THEATER_INTERIOR) &&
      (TType == TEMPLATE_NONE || TType == TEMPLATE_CLEAR1)) {
    Land = LAND_ROCK;
    return;
  }

  /*
  **	Check for wall effects.
  */
  if (Overlay != OVERLAY_NONE) {
    Land = OverlayTypeClass::As_Reference(Overlay).Land;
    if (Land != LAND_CLEAR) {
      return;
    }
  }

  /*
  **	If there is a template associated with this cell, then fetch the
  **	land type given the template type and icon number.
  */
  if (TType != TEMPLATE_NONE && TType != static_cast<TemplateType>(255)) {
    const TemplateTypeClass* ttype = &TemplateTypeClass::As_Reference(TType);
    Land = ttype->Land_Type(TIcon);
    return;
  }

  /*
  **	No template is the same as clear terrain.
  */
  Land = LAND_CLEAR;
}

/***********************************************************************************************
 * CellClass::Occupy_Down -- Flag occupation of specified cell. *
 *                                                                                             *
 *    This routine is used to mark the cell as being occupied by the specified
 *object.         *
 *                                                                                             *
 * INPUT:   object   -- The object that is to occupy the cell *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. * 11/29/1994 JLB : Simplified. *
 *=============================================================================================*/
void CellClass::Occupy_Down(ObjectClass* object) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  DCHECK(object != nullptr && object->IsActive);

  if (object == nullptr) {
    return;
  }

  /*
  **	Always add buildings to the end of the occupation chain. This is
  *necessary because *	the occupation chain is a single list even though
  *buildings occupy more than one *	cell. If more than one building is
  *allowed to occupy the same cell, then this chain *	logic will fail.
  */
  if (object->What_Am_I() == RTTI_BUILDING && Cell_Occupier()) {
    ObjectClass* optr = Cell_Occupier();
    while (optr->Next != nullptr) {
      DCHECK(optr != object);
      DCHECK(optr->What_Am_I() != RTTI_BUILDING);
      optr = optr->Next;
    }
    optr->Next = object;
    object->Next = nullptr;
  } else {
    object->Next = Cell_Occupier();
    OccupierPtr = object;
  }
  TheMap().Radar_Pixel(Cell_Number());

  /*
  **	If being placed down on a visible square, then flag this
  **	techno object as being revealed to the player.
  */
  if (IsMapped || TheSession().Type != GAME_NORMAL) {
    object->Revealed(ThePlayer());
  }

  /*
  **	Special occupy bit set.
  */
  switch (object->What_Am_I()) {
    case RTTI_BUILDING:
      Flag.Occupy.Building = true;
      break;

    case RTTI_VESSEL:
    case RTTI_AIRCRAFT:
    case RTTI_UNIT:
      Flag.Occupy.Vehicle = true;
      break;

    case RTTI_TERRAIN:
      Flag.Occupy.Monolith = true;
      break;

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRY:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }
}

/***********************************************************************************************
 * CellClass::Occupy_Up -- Removes occupation flag from the specified cell. *
 *                                                                                             *
 *    This routine will lift the object from the cell and free the cell to be
 *occupied by      * another object. Only if the cell was previously marked with
 *the object specified, will   * the object be lifted off. This routine is the
 *counterpart to Occupy_Down().              *
 *                                                                                             *
 * INPUT:   object   -- The object that is being lifted off. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. * 11/29/1994 JLB : Fixed to handle next
 *pointer in previous object.                         *
 *=============================================================================================*/
void CellClass::Occupy_Up(ObjectClass* object) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  DCHECK(object != nullptr && object->IsActive);

  if (object == nullptr) {
    return;
  }

  ObjectClass* optr =
      Cell_Occupier();  // Working pointer to the objects in the chain.

  if (optr == object) {
    OccupierPtr = object->Next;
    object->Next = nullptr;
  } else {
    while (optr != nullptr) {
      if (optr->Next == object) {
        optr->Next = object->Next;
        object->Next = nullptr;
        break;
      }
      optr = optr->Next;
    }
  }
  TheMap().Radar_Pixel(Cell_Number());

  /*
  **	Special occupy bit clear.
  */
  switch (object->What_Am_I()) {
    case RTTI_BUILDING:
      Flag.Occupy.Building = false;
      break;

    case RTTI_VESSEL:
    case RTTI_AIRCRAFT:
    case RTTI_UNIT:
      Flag.Occupy.Vehicle = false;
      break;

    case RTTI_TERRAIN:
      Flag.Occupy.Monolith = false;
      break;

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRY:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }
}

/***********************************************************************************************
 * CellClass::Overlap_Down -- This routine is used to mark a cell as being
 *spilled over (overla*
 *                                                                                             *
 *    Most game objects can often have their graphic imagery spill into more
 *than one cell     * even though they are considered to "occupy" only one cell.
 *All cells overlapped are      * flagged by this routine. Using this
 *information it is possible to keep the tactical map  * display correct. *
 *                                                                                             *
 * INPUT:   object   -- The object to mark as overlapping this cell. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. * 07/04/1995 JLB : Ensures that
 *buildings are always marked down.                           *
 *=============================================================================================*/
void CellClass::Overlap_Down(ObjectClass* object) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  DCHECK(object != nullptr && object->IsActive);

  ObjectClass** ptr = nullptr;

  if (!object) {
    return;
  }

  for (int index = 0; index < std::ssize(Overlappers); index++) {
    if (base::At(Overlappers, index) == object) {
      return;
    }
    if (!base::At(Overlappers, index)) {
      ptr = base::Suffix(Overlappers, index).data();
    }
  }

  /*
  **	Buildings must ALWAYS succeed in marking the cell as overlapped. Bump
  *somebody *	else out in this case.
  */
  if (!ptr && object->What_Am_I() == RTTI_BUILDING) {
    for (int index = 0; index < std::ssize(Overlappers); index++) {
      switch (base::At(Overlappers, index)->What_Am_I()) {
        case RTTI_BUILDING:
        case RTTI_TERRAIN:
          break;

        case RTTIType::RTTI_NONE:
        case RTTIType::RTTI_AIRCRAFT:
        case RTTIType::RTTI_AIRCRAFTTYPE:
        case RTTIType::RTTI_ANIM:
        case RTTIType::RTTI_ANIMTYPE:
        case RTTIType::RTTI_BUILDINGTYPE:
        case RTTIType::RTTI_BULLET:
        case RTTIType::RTTI_BULLETTYPE:
        case RTTIType::RTTI_CELL:
        case RTTIType::RTTI_FACTORY:
        case RTTIType::RTTI_HOUSE:
        case RTTIType::RTTI_HOUSETYPE:
        case RTTIType::RTTI_INFANTRY:
        case RTTIType::RTTI_INFANTRYTYPE:
        case RTTIType::RTTI_OVERLAY:
        case RTTIType::RTTI_OVERLAYTYPE:
        case RTTIType::RTTI_SMUDGE:
        case RTTIType::RTTI_SMUDGETYPE:
        case RTTIType::RTTI_SPECIAL:
        case RTTIType::RTTI_TEAM:
        case RTTIType::RTTI_TEAMTYPE:
        case RTTIType::RTTI_TEMPLATE:
        case RTTIType::RTTI_TEMPLATETYPE:
        case RTTIType::RTTI_TERRAINTYPE:
        case RTTIType::RTTI_TRIGGER:
        case RTTIType::RTTI_TRIGGERTYPE:
        case RTTIType::RTTI_UNIT:
        case RTTIType::RTTI_UNITTYPE:
        case RTTIType::RTTI_VESSEL:
        case RTTIType::RTTI_VESSELTYPE:
        default:
          base::At(Overlappers, index) = object;
          index = static_cast<int>(std::ssize(Overlappers));
          break;
      }
    }
  }
  if (ptr) {
    *ptr = object;
  }

  /*
  **	If being placed down on a visible square, then flag this
  **	techno object as being revealed to the player.
  */
  if (IsMapped) {
    object->Revealed(ThePlayer());
  }
}

/***********************************************************************************************
 * CellClass::Overlap_Up -- Removes overlap flag for the cell. *
 *                                                                                             *
 *    This is the counterpart to Overlap_Down and is used to remove the overlap
 *flag for the   * specified unit on the cell. *
 *                                                                                             *
 * INPUT:   object   -- The object to remove the overlap flag for. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. *
 *=============================================================================================*/
void CellClass::Overlap_Up(ObjectClass* object) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  DCHECK(object != nullptr && object->IsActive);

  for (int index = 0; index < std::ssize(Overlappers); index++) {
    if (base::At(Overlappers, index) == object) {
      base::At(Overlappers, index) = nullptr;
      break;
    }
  }
}

/***********************************************************************************************
 * CellClass::Cell_Unit -- Returns with pointer to unit occupying cell. *
 *                                                                                             *
 *    This routine will determine if a unit is occupying the cell and if so,
 *return a pointer  * to it. If there is no unit occupying the cell, then
 * nullptr is returned.                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to unit occupying cell, else nullptr. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. *
 *=============================================================================================*/
UnitClass* CellClass::Cell_Unit() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return dynamic_cast<UnitClass*>(Cell_Find_Object(RTTI_UNIT));
}

/***********************************************************************************************
 * CellClass::Cell_Vessel -- Returns with pointer to a vessel located in the
 *cell.             *
 *                                                                                             *
 *    Call this routine to query and return a pointer to a vessel located in the
 *cell. If      * there is no vessel present, then this routine will return
 *nullptr.                          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the vessel class object if one is present.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/20/1996 JLB : Created. *
 *=============================================================================================*/
VesselClass* CellClass::Cell_Vessel() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return dynamic_cast<VesselClass*>(Cell_Find_Object(RTTI_VESSEL));
}

/***********************************************************************************************
 * CellClass::Cell_Infantry -- Returns with pointer of first infantry unit
 *occupying the cell. *
 *                                                                                             *
 *    This routine examines the cell and returns a pointer to the first infantry
 *unit          * that occupies it. If there is no infantry unit in the cell,
 *then nullptr is returned.       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to infantry unit occupying the cell or nullptr
 * if none are       * present. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/21/1994 JLB : Created. *
 *=============================================================================================*/
InfantryClass* CellClass::Cell_Infantry() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return dynamic_cast<InfantryClass*>(Cell_Find_Object(RTTI_INFANTRY));
}

/***********************************************************************************************
 * CellClass::Draw_It -- Draws the cell imagery at the location specified. *
 *                                                                                             *
 *    This is the gruntwork cell rendering code. It draws the cell at the screen
 *location      * specified. This routine doesn't draw any overlapping or
 *occupying units. It only         * deals with the ground (cell) layer -- icon
 *level.                                        *
 *                                                                                             *
 * INPUT:   x,y   -- The screen coordinates to render the cell imagery at. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/18/1994 JLB : Created. * 08/21/1994 JLB : Revised for simple
 *template objects.                                     * 11/01/1994 BRR :
 *Updated placement cursor; draws actual object                            *
 *   11/14/1994 BRR : Added remapping code to show passable areas * 12/02/1994
 *BRR : Added trigger display * 12/11/1994 JLB : Mixes up clear terrain through
 *pseudo-random table.                      * 04/25/1995 JLB : Smudges drawn
 *BELOW overlays.                                            * 07/22/1996 JLB :
 *Objects added to draw process.                                           *
 *=============================================================================================*/
void CellClass::Draw_It(PixelView& view, int x, int y) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const TemplateTypeClass* ttype = nullptr;
  int icon = 0;  // The icon number to use from the template set.
  std::span<const unsigned char> remap;

  /*
  **	Fetch a pointer to the template type associated with this cell.
  */
  if (TType != TEMPLATE_NONE && TType != TEMPLATE_CLEAR1 &&
      TType != static_cast<TemplateType>(255)) {
    ttype = &TemplateTypeClass::As_Reference(TType);
    icon = TIcon;
  } else {
    ttype = &TemplateTypeClass::As_Reference(TEMPLATE_CLEAR1);
    icon = Clear_Icon();
  }

  bool draw_debug_icon = false;
  if constexpr (config::kCheatKeysEnabled) {
    draw_debug_icon = TheDebugState().show_cell_info();
  }

  if (draw_debug_icon) {
    /*
    **	Draw debug cell visualization instead of normal terrain.
    */
    const Cell cell = Cell_Number();
    view.FillRect(TheMap().TacPixelX + x, TheMap().TacPixelY + y,
                  TheMap().TacPixelX + x + ICON_PIXEL_W - 1,
                  TheMap().TacPixelY + y + ICON_PIXEL_H - 1,
                  static_cast<unsigned char>(Sim_Random_Pick(1, 254)));
    Fancy_Text_Print(
        view, "%02X%02X\r%d%d%d\r%d %d",
        TheMap().TacPixelX + x + (ICON_PIXEL_W >> 1), TheMap().TacPixelY + y,
        &ThePalettes().grey_scheme(), kTBlack,
        TPF_EFNT | TPF_CENTER | TPF_BRIGHT_COLOR | TPF_FULLSHADOW, cell.y(),
        cell.x(), Zones.at(MZONE_NORMAL), Zones.at(MZONE_CRUSHER),
        Zones.at(MZONE_DESTROYER), Overlay, OverlayData);
  } else {
    if constexpr (config::kScenarioEditorEnabled) {
      /*
      **	Set up the remap table for this icon.
      */
      if (TheDebugState().map_editor_active() &&
          TheDebugState().show_passability()) {
        if (TheRules().ground().at(Land).Cost.at(SPEED_FOOT) == 0 ||
            (Cell_Occupier() != nullptr &&
             Cell_Occupier()->What_Am_I() != RTTI_INFANTRY)) {  // impassable
          remap = DisplayClass::FadingRed;
        } else {
          if (TheRules().ground().at(Land).Cost.at(SPEED_FOOT) >
              Fixed(1, 3)) {  // pretty passable
            remap = DisplayClass::FadingGreen;
          } else {
            remap = DisplayClass::FadingYellow;  // moderately passable
          }
        }
      }
    }

    /*
    **	This is the underlying terrain icon.
    */
    if (!ttype->Get_Image_Data().empty()) {
      view.DrawStamp(ttype->Get_Image_Data(), icon, x, y, {},
                     WindowRect(WINDOW_TACTICAL));
      if (!remap.empty()) {
        view.Remap(x + TheMap().TacPixelX, y + TheMap().TacPixelY, ICON_PIXEL_W,
                   ICON_PIXEL_H, remap);
      }
    }

    if constexpr (config::kScenarioEditorEnabled) {
      /*
      **	Draw the map editor's "current" cell. This is the cell that can
      **	be assigned attributes such as tag labels. This must be drawn
      **	before the placement cursor, but after drawing the objects in
      * the *	cell.
      */
      if (TheDebugState().map_editor_active() &&
          TheWorld().current_cell() == Cell_Number()) {
        view.DrawRect(x + TheMap().TacPixelX, y + TheMap().TacPixelY,
                      TheMap().TacPixelX + x + CELL_PIXEL_W - 1,
                      TheMap().TacPixelY + y + CELL_PIXEL_H - 1, kYellow);
      }
    }

    /*
    **	Redraw any smudge.
    */
    if (Smudge != SMUDGE_NONE) {
      SmudgeTypeClass::As_Reference(Smudge).Draw_It(view, x, y, SmudgeData);
    }

    /*
    **	Draw the overlay object.
    */
    if (Overlay != OVERLAY_NONE) {
      const OverlayTypeClass& otype = OverlayTypeClass::As_Reference(Overlay);
      CC_Draw_Shape(view, otype.Get_Image_Data(), OverlayData,
                    x + (CELL_PIXEL_W >> 1), y + (CELL_PIXEL_H >> 1),
                    WINDOW_TACTICAL, ShapeFlags::kCentered | ShapeFlags::kGhost,
                    {}, DisplayClass::UnitShadow);
    }

    if constexpr (config::kScenarioEditorEnabled) {
      if (TheDebugState().map_editor_active()) {
        /*
        **	Draw the cell's Trigger mnemonic, if it has a trigger
        */
        if (Trigger != nullptr) {
          Fancy_Text_Print(view, Trigger->Class->IniName,
                           x + TheMap().TacPixelX, y + TheMap().TacPixelY,
                           &ThePalettes().color_remaps().at(PCOLOR_RED),
                           kTBlack, TPF_EFNT | TPF_FULLSHADOW);
        }

        /*
        **	Draw the cell's Waypoint designation if there is one.
        */
        if (IsWaypoint) {
          char waypt[3];
          for (int i = 0; i < ScenarioClass::kHomeWaypoint; i++) {
            if (base::At(TheScenario().Waypoint, i) == Cell_Number()) {
              if (i < 26) {
                base::At(waypt, 0) = static_cast<char>('A' + i);
                base::At(waypt, 1) = 0;
              } else {
                base::At(waypt, 0) = static_cast<char>('A' + (i / 26) - 1);
                base::At(waypt, 1) = static_cast<char>('A' + (i % 26));
                base::At(waypt, 2) = 0;
              }
              Fancy_Text_Print(view, waypt,
                               TheMap().TacPixelX + x + (CELL_PIXEL_W / 2),
                               TheMap().TacPixelY + y + (CELL_PIXEL_H / 2) - 3,
                               &ThePalettes().color_remaps().at(PCOLOR_RED),
                               kTBlack, TPF_EFNT | TPF_CENTER | TPF_FULLSHADOW);
              break;
            }
          }
          if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) ==
              Cell_Number()) {
            Fancy_Text_Print(view, "Home", TheMap().TacPixelX + x,
                             TheMap().TacPixelY + y + (CELL_PIXEL_H)-7,
                             &ThePalettes().color_remaps().at(PCOLOR_GREY),
                             kTBlack, TPF_EFNT | TPF_FULLSHADOW);
          }
          if (base::At(TheScenario().Waypoint,
                       ScenarioClass::kReinforcementWaypoint) ==
              Cell_Number()) {
            Fancy_Text_Print(view, "Reinf", TheMap().TacPixelX + x,
                             TheMap().TacPixelY + y + (CELL_PIXEL_H)-7,
                             &ThePalettes().color_remaps().at(PCOLOR_GREY),
                             kTBlack, TPF_EFNT | TPF_FULLSHADOW);
          }
        }
      }
    }

    /*
    **	Draw the placement cursor:
    **	- First, draw the hash-mark cursor, so it will appear underneath
    **	  any cursor being drawn
    **	- If the PendingObject is a template, overlay, or smudge, draw
    *it *	- Otherwise, it's up to the Display.Refresh_Map() routine to
    *draw it
    */
    if (IsCursorHere) {
      SpeedType loco = SPEED_NONE;
      if (TheMap().PendingObjectPtr &&
          (TheMap().PendingObjectPtr->What_Am_I() == RTTI_BUILDING)) {
        const auto* obj =
            dynamic_cast<BuildingClass*>(TheMap().PendingObjectPtr);
        loco = obj->Class->Speed;
      }

      /*
      **	Draw the hash-mark cursor:
      */
      if (TheMap().ProximityCheck && Is_Clear_To_Build(loco)) {
        view.DrawStamp(DisplayClass::TransIconset, 0, x, y, {},
                       WindowRect(WINDOW_TACTICAL));
      } else {
        view.DrawStamp(DisplayClass::TransIconset, 2, x, y, {},
                       WindowRect(WINDOW_TACTICAL));
      }

      if constexpr (config::kScenarioEditorEnabled) {
        if (TheDebugState().map_editor_active() && TheMap().PendingObject) {
          switch (TheMap().PendingObject->What_Am_I()) {
            /*
            **	Draw a template:
            **	- Compute the icon offset of this cell for this
            * template, *	  using ZoneCell+ZoneOffset to get the
            * upper-left corner of *	  the placement cursor *	- Draw
            * the icon
            */
            case RTTI_TEMPLATETYPE: {
              const auto* tptr = dynamic_cast<const TemplateTypeClass*>(
                  TheMap().PendingObject);
              if (!tptr->Get_Image_Data().empty()) {
                const Cell cell = Cell_Number();
                icon = (cell.x() -
                        TheMap().ZoneCell.Offset(TheMap().ZoneOffset).x()) +
                       ((cell.y() -
                         TheMap().ZoneCell.Offset(TheMap().ZoneOffset).y()) *
                        tptr->Width);
                view.DrawStamp(tptr->Get_Image_Data(), icon, x, y, {},
                               WindowRect(WINDOW_TACTICAL));
              }
              break;
            }

            /*
            **	Draw an overlay; just use the existing 'OverlayData'
            * even *	though it means nothing.
            */
            case RTTI_OVERLAYTYPE:
              OverlayTypeClass::As_Reference(
                  dynamic_cast<const OverlayTypeClass*>(TheMap().PendingObject)
                      ->Type)
                  .Draw_It(view, x, y, OverlayData);
              break;

            /*
            **	Draw a smudge
            */
            case RTTI_SMUDGETYPE:
              SmudgeTypeClass::As_Reference(
                  dynamic_cast<const SmudgeTypeClass*>(TheMap().PendingObject)
                      ->Type)
                  .Draw_It(view, x, y, 0);
              break;

            case RTTIType::RTTI_NONE:
            case RTTIType::RTTI_AIRCRAFT:
            case RTTIType::RTTI_AIRCRAFTTYPE:
            case RTTIType::RTTI_ANIM:
            case RTTIType::RTTI_ANIMTYPE:
            case RTTIType::RTTI_BUILDING:
            case RTTIType::RTTI_BUILDINGTYPE:
            case RTTIType::RTTI_BULLET:
            case RTTIType::RTTI_BULLETTYPE:
            case RTTIType::RTTI_CELL:
            case RTTIType::RTTI_FACTORY:
            case RTTIType::RTTI_HOUSE:
            case RTTIType::RTTI_HOUSETYPE:
            case RTTIType::RTTI_INFANTRY:
            case RTTIType::RTTI_INFANTRYTYPE:
            case RTTIType::RTTI_OVERLAY:
            case RTTIType::RTTI_SMUDGE:
            case RTTIType::RTTI_SPECIAL:
            case RTTIType::RTTI_TEAM:
            case RTTIType::RTTI_TEAMTYPE:
            case RTTIType::RTTI_TEMPLATE:
            case RTTIType::RTTI_TERRAIN:
            case RTTIType::RTTI_TERRAINTYPE:
            case RTTIType::RTTI_TRIGGER:
            case RTTIType::RTTI_TRIGGERTYPE:
            case RTTIType::RTTI_UNIT:
            case RTTIType::RTTI_UNITTYPE:
            case RTTIType::RTTI_VESSEL:
            case RTTIType::RTTI_VESSELTYPE:
            default:
              break;
          }
        }
      }
    }

    /*
    **	Draw the flag if there is one located at this cell.
    */
    if (IsFlagged) {
      const auto flag_remap =
          HouseClass::As_Pointer(Owner)->Remap_Table(false, REMAP_NORMAL);
      CC_Draw_Shape(
          view, MixArchive::RetrieveData("FLAGFLY.SHP"),
          static_cast<int>(game::CurrentFrame() % 14), x + (ICON_PIXEL_W / 2),
          y + (ICON_PIXEL_H / 2), WINDOW_TACTICAL,
          ShapeFlags::kCentered | ShapeFlags::kGhost | ShapeFlags::kFading,
          flag_remap, DisplayClass::UnitShadow);
    }
  }
}

/***********************************************************************************************
 * CellClass::Concrete_Calc -- Calculates the concrete icon to use for the cell.
 **
 *                                                                                             *
 *    This routine examines the cells around the current one and from this,
 *determines what    * concrete icon shape to use (if any). The cell data is
 *adjusted and the cell is marked    * for redraw if the icon changed. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/01/1994 JLB : Created. *
 *=============================================================================================*/
void CellClass::Concrete_Calc() {}

/***********************************************************************************************
 * CellClass::Wall_Update -- Updates the imagery for wall objects in cell. *
 *                                                                                             *
 *    This routine will examine the cell and the adjacent cells to determine
 *what the wall     * should look like with the cell. It will then update the
 *wall's imagery value and flag    * the cell to be redrawn if necessary. This
 *routine should be called whenever the wall     * or an adjacent wall is
 *created or destroyed.                                             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. * 09/19/1994 BWG : Updated to handle
 *partially-damaged walls.                               *
 *=============================================================================================*/
void CellClass::Wall_Update() {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  static const game::FacingType _offsets[5] = {game::FACING_N, game::FACING_E,
                                               game::FACING_S, game::FACING_W,
                                               game::FACING_NONE};

  for (const auto& _offset : _offsets) {
    CellClass& newcell = Adjacent_Cell(_offset);

    if (newcell.Overlay != OVERLAY_NONE &&
        OverlayTypeClass::As_Reference(newcell.Overlay).IsWall) {
      uint32_t icon = 0;

      /*
      **	Build the icon number according to walls located in the adjacent
      **	cells.
      */
      for (int i = 0; i < 4; i++) {
        if (newcell.Adjacent_Cell(base::At(_offsets, i)).Overlay ==
            newcell.Overlay) {
          icon |= base::Bit<uint32_t>(i);
        }
      }
      newcell.OverlayData =
          static_cast<unsigned char>((newcell.OverlayData & 0xF0U) | icon);

      /*
      **	Handle special cases for the incomplete damaged wall sets. If a
      *damage stage *	is calculated, but there is no artwork for it, then
      *consider the wall to be *	completely destroyed.
      */
      if (newcell.Overlay == OVERLAY_BRICK_WALL && newcell.OverlayData == 48) {
        newcell.Overlay = OVERLAY_NONE;
        newcell.OverlayData = 0;
        Detach_This_From_All(Target::FromCell(newcell.Cell_Number()), true);
      }
      if (newcell.Overlay == OVERLAY_SANDBAG_WALL &&
          newcell.OverlayData == 16) {
        newcell.Overlay = OVERLAY_NONE;
        newcell.OverlayData = 0;
        Detach_This_From_All(Target::FromCell(newcell.Cell_Number()), true);
      }
      if (newcell.Overlay == OVERLAY_CYCLONE_WALL &&
          newcell.OverlayData == 32) {
        newcell.Overlay = OVERLAY_NONE;
        newcell.OverlayData = 0;
        Detach_This_From_All(Target::FromCell(newcell.Cell_Number()), true);
      }
      if (newcell.Overlay == OVERLAY_FENCE &&
          (newcell.OverlayData == 16 || newcell.OverlayData == 32)) {
        newcell.Overlay = OVERLAY_NONE;
        newcell.OverlayData = 0;
        Detach_This_From_All(Target::FromCell(newcell.Cell_Number()), true);
      }
      if (newcell.Overlay == OVERLAY_BARBWIRE_WALL &&
          newcell.OverlayData == 16) {
        newcell.Overlay = OVERLAY_NONE;
        newcell.OverlayData = 0;
        Detach_This_From_All(Target::FromCell(newcell.Cell_Number()), true);
      }

      newcell.Recalc_Attributes();
      newcell.Redraw_Objects();
    }
  }
}

/***********************************************************************************************
 * CellClass::Center_Coord -- Returns the coordinate of this cell. *
 *                                                                                             *
 *    This support function will determine the coordinate of this cell and
 *return it.          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with coordinate value of cell. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
Coordinate CellClass::Center_Coord() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  return Cell_Number().Center();
}

/***********************************************************************************************
 * CellClass::Reduce_Tiberium -- Reduces the tiberium in the cell by the amount
 *specified.     *
 *                                                                                             *
 *    This routine will lower the tiberium level in the cell. It is used by the
 *harvesting     * process as well as by combat damage to the tiberium fields. *
 *                                                                                             *
 * INPUT:   levels   -- The number of levels to reduce the tiberium. *
 *                                                                                             *
 * OUTPUT:  bool; Was the tiberium level reduced by at least one level? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
int CellClass::Reduce_Tiberium(int levels) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  int reducer = 0;

  if (levels > 0 && Land == LAND_TIBERIUM) {
    if (OverlayData + 1 > levels) {
      OverlayData -= levels;
      reducer = levels;
    } else {
      Overlay = OVERLAY_NONE;
      reducer = OverlayData;
      OverlayData = 0;
      Recalc_Attributes();
    }
  }
  return reducer;
}

/***********************************************************************************************
 * CellClass::Reduce_Wall -- Damages a wall, if damage is high enough. *
 *                                                                                             *
 *    This routine will change the wall shape used for a wall if it's damaged. *
 *                                                                                             *
 * INPUT:   damage   -- The number of damage points the wall was hit with. If
 *this value is    * -1, then the entire wall at this cell will be destroyed. *
 *                                                                                             *
 * OUTPUT:  bool; Was the wall destroyed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/15/1995 BWG : Created. * 03/19/1995 JLB : Updates cell
 *information if wall was destroyed.                          * 10/06/1996 JLB :
 *Updates zone as necessary.                                               *
 *=============================================================================================*/
bool CellClass::Reduce_Wall(int damage) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  if (Overlay != OVERLAY_NONE) {
    bool destroyed = false;
    const OverlayTypeClass& wall = OverlayTypeClass::As_Reference(Overlay);

    if (wall.IsWall) {
      /*
      **	If the damage was great enough to ensure wall destruction,
      *reduce the wall by one *	level (no more). Otherwise determine wall
      *reduction based on a percentage chance *	proportional to the damage
      *received and the wall's strength.
      */
      if (damage == -1 || damage >= wall.DamagePoints) {
        destroyed = true;
      } else {
        destroyed = Random_Pick(0, wall.DamagePoints) < damage;
      }

      /*
      **	If the wall is destroyed, destroy it and check for any
      *adjustments to *	adjacent walls.
      */
      if (destroyed) {
        OverlayData += 16;
        if (damage == -1 || OverlayData >> 4 >= wall.DamageLevels ||
            (OverlayData >> 4 == wall.DamageLevels - 1 &&
             (OverlayData & 0xF) == 0)) {
          Owner = HOUSE_NONE;
          Overlay = OVERLAY_NONE;
          OverlayData = 0;
          Recalc_Attributes();
          Redraw_Objects();
          Adjacent_Cell(game::FACING_N).Wall_Update();
          Adjacent_Cell(game::FACING_W).Wall_Update();
          Adjacent_Cell(game::FACING_S).Wall_Update();
          Adjacent_Cell(game::FACING_E).Wall_Update();
          Detach_This_From_All(As_Target());

          /*
          **	The zone calculation changes now for non-crushable zone
          *sensitive *	travellers.
          */
          if (wall.IsCrushable) {
            TheMap().Zone_Reset(kZoneFlagNormal);
          } else {
            TheMap().Zone_Reset(kZoneFlagCrusher | kZoneFlagNormal);
          }
          return true;
        }
      }
    }
  }
  return false;
}

/***********************************************************************************************
 * CellClass::Spot_Index -- returns cell sub-coord index for given Coordinate *
 *                                                                                             *
 * INPUT: * coord      Coordinate to compute index for *
 *                                                                                             *
 * OUTPUT: * index into StoppingCoord that's closest to this coord *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/21/1994 BR : Created. * 12/10/1994 JLB : Uses alternate
 *sub-position algorithm.                                   *
 *=============================================================================================*/
int CellClass::Spot_Index(Coordinate coord) {
  const Coordinate rel = coord.Fraction();  // Sub coordinate value within cell.

  /*
  **	If the coordinate is close enough to the center of the cell, then return
  **	the center position index.
  */
  if (Distance(rel, Coordinate::CellCenterOffset()) < 60) {
    return 0;
  }

  /*
  **	Since the center cell position has been eliminated, a simple comparison
  **	as related to the center of the cell can be used to determine the sub
  **	position. Take advantage of the fact that the sub positions are
  *organized *	from left to right, top to bottom.
  */
  int index = 0;
  if (rel.x() > 0x80) {
    index += 1;
  }
  if (rel.y() > 0x80) {
    index += 2;
  }
  return index + 1;
}

/***********************************************************************************************
 * CellClass::Closest_Free_Spot -- returns free spot closest to given coord *
 *                                                                                             *
 * Similar to the CellClass::Free_Spot; this routine finds the spot in * the
 *cell closest to the given coordinate, and returns the Coordinate of * that
 *spot if it's available, nullptr if it's not. *
 *                                                                                             *
 * INPUT: * coord   coordinate to check (only sub cell position examined) *
 *                                                                                             *
 *          any   -- If only the closest spot is desired regardless of whether
 *it is free or   * not, then this parameter will be true. *
 *                                                                                             *
 * OUTPUT: * Coordinate of free spot, nullptr if none. The coordinate return
 * value does not alter the cell * coordinate data portions of the coordinate
 * passed in. Only the lower sub-cell   * data is altered. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/08/1994 BR : Created. * 12/10/1994 JLB : Picks best of closest
 *stopping positions.                                * 12/21/1994 JLB : Adds a
 *mix-up factor if center location is occupied.                     *
 *=============================================================================================*/
Coordinate CellClass::Closest_Free_Spot(Coordinate coord, bool any) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const int spot_index = Spot_Index(coord);

  /*
  **	This precalculated sequence table records the closest spots to any given
  *spot. Sequential *	examination of these spots for availability ensures that
  *the closest available one is *	discovered first.
  */
  static unsigned char _sequence[5][4] = {
      {1, 2, 3, 4}, {0, 2, 3, 4}, {0, 1, 4, 3}, {0, 1, 4, 2}, {0, 2, 3, 1}};

  /*
  **	In the case of the center coordinate being requested, but is occupied,
  *then all other *	sublocations are equidistant. Instead of picking a
  *static sequence of examination, the *	order is mixed up by way of this
  *table.
  */
  static unsigned char _alternate[4][4] = {
      {1, 2, 3, 4},
      {2, 3, 4, 1},
      {3, 4, 1, 2},
      {4, 1, 2, 3},
  };
  coord = coord.Whole();

  /*
  **	Cells occupied by buildings or vehicles don't have any free spots.
  */
  if (!any && (Flag.Occupy.Vehicle || Flag.Occupy.Monolith)) {
    return Coordinate{};
  }

  /*
  **	If just the nearest position is desired regardless of whether occupied
  *or not, *	then just return with the stopping coordinate value.
  */
  if (any || Is_Spot_Free(spot_index)) {
    return coord + base::At(StoppingCoordAbs, spot_index);
  }

  /*
  **	Scan through all available sub-locations in the cell in order to
  *determine *	the closest one to the coordinate requested. Use precalculated
  *table so that *	when the first free position is found, bail.
  */
  std::span<const unsigned char> sequence;
  if (spot_index == 0) {
    sequence = std::span(base::At(_alternate, Random_Pick(0, 3)));
  } else {
    sequence = std::span(base::At(_sequence, spot_index));
  }
  for (int index = 0; index < 4; index++) {
    const int pos = base::At(sequence, base::ToSize(index));

    if (Is_Spot_Free(pos)) {
      return coord + base::At(StoppingCoordAbs, pos);
    }
  }

  /*
  **	No free spot could be found so return a nullptr coordinate.
  */
  return Coordinate{};
}

/***********************************************************************************************
 * CellClass::Clear_Icon -- Calculates what the clear icon number should be. *
 *                                                                                             *
 *    This support routine will determine what the clear icon number would be
 *for the cell.    * The icon number is determined by converting the cell number
 *into an index into a         * lookup table. This yields what appears to be a
 *randomized map without the necessity of   * generating and recording
 *randomized map numbers.                                         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the icon number for clear terrain if it were displayed
 *at the         * cell. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/26/1994 JLB : Created. * 06/09/1995 JLB : Uses 16 entry
 *scramble algorithm.                                        *
 *=============================================================================================*/
int CellClass::Clear_Icon() const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const Cell cell = Cell_Number();
  return (cell.x() % 4) + ((cell.y() % 4) * 4);
  //	return((cell & 0x03) | ((unsigned(cell)>>5) & 0x0C));
}

/***********************************************************************************************
 * CellClass::Incoming -- Causes objects in cell to "run for cover". *
 *                                                                                             *
 *    This routine is called whenever a great, but slow moving, threat is
 *presented to the     * occupants of a cell. The occupants will, in most cases,
 *stop what they are doing and     * try to get out of the way. *
 *                                                                                             *
 * INPUT:   threat      -- The coordinate source of the threat. *
 *                                                                                             *
 *          forced      -- If this threat is so major that the occupants should
 *stop what      * they are doing, then this parameter should be set to true. *
 *                                                                                             *
 *          nokidding   -- Override the scatter to also affect human controlled
 *objects.       *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/10/1995 JLB : Created. * 08/02/1996 JLB : Added the "nokidding"
 *parameter.                                         *
 *=============================================================================================*/
// Not const: scatters the units in the cell.
// NOLINTNEXTLINE(readability-make-member-function-const)
void CellClass::Incoming(Coordinate threat, bool forced, bool nokidding) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  ObjectClass* object = nullptr;

  object = Cell_Occupier();
  while (object != nullptr) {
    /*
    **	Special check to make sure that friendly units never scatter.
    */
    if (nokidding || TheRules().IsScatter ||
        (object->Is_Techno() && dynamic_cast<TechnoClass*>(object)->House->IQ >=
                                    TheRules().IQScatter)) {
      object->Scatter(threat, forced, nokidding);
    }
    object = object->Next;
  }
}

/***********************************************************************************************
 * CellClass::Adjacent_Offset -- Determines the offset to the adjacent cell.
 **
 *                                                                                             *
 *    Both Adjacent_Cell overloads share this so that neither has to cast
 *away const.       *
 *                                                                                             *
 * HISTORY: * 03/19/1995 JLB : Created. *
 *=============================================================================================*/
int CellClass::Adjacent_Offset(game::FacingType face) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  if (static_cast<unsigned>(face) >=
      magic_enum::enum_count<game::FacingType>()) {
    return 0;
  }

  // Check the index before forming the pointer: pointer arithmetic that leaves
  // the cell array is undefined even if the result is never dereferenced.
  const int adjacent = Cell_Number().index() + AdjacentCell.at(face);
  if (adjacent < 0 || adjacent >= MAP_CELL_TOTAL) {
    return 0;
  }
  return AdjacentCell.at(face);
}

/***************************************************************************
 * CellClass::Adjust_Threat -- Allows adjustment of threat at cell level   *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/24/1995 PWG : Created.                                             *
 *=========================================================================*/
// Not const: changes the other houses' threat values.
// NOLINTNEXTLINE(readability-make-member-function-const)
void CellClass::Adjust_Threat(HousesType house, int threat_value) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  const int region = MapEditClass::Cell_Region(Cell_Number());

  for (const HousesType lp : magic_enum::enum_values<HousesType>()) {
    if (lp == house) {
      continue;
    }

    HouseClass* house_ptr = HouseClass::As_Pointer(lp);
    if (house_ptr && (!house_ptr->IsHuman || !house_ptr->Is_Ally(house))) {
      house_ptr->Adjust_Threat(region, threat_value);
    }
  }
}

/***********************************************************************************************
 * CellClass::Tiberium_Adjust -- Adjust the look of the Tiberium for smoothing
 *purposes.       *
 *                                                                                             *
 *    This routine will adjust the level of the Tiberium in the cell so that it
 *will           * smoothly blend with the adjacent Tiberium. This routine
 *should only be called for        * new Tiberium cells. Existing cells that
 *contain Tiberium follow a different growth       * pattern. *
 *                                                                                             *
 * INPUT:   pregame  -- Is this a pregame call? Such a call will mixup the
 *Tiberium overlay    * used. *
 *                                                                                             *
 * OUTPUT:  Returns with the added Tiberium value that is now available for
 *harvesting.        *
 *                                                                                             *
 * WARNINGS:   The return value is only valid for the initial placement.
 *Tiberium growth will  * increase the net worth of the existing Tiberium. *
 *                                                                                             *
 * HISTORY: * 05/16/1995 JLB : Created. * 02/20/1996 JLB : Takes into account
 *the ore type.                                         *
 *=============================================================================================*/
int32_t CellClass::Tiberium_Adjust(bool pregame) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);
  if ((Overlay != OVERLAY_NONE) &&
      (OverlayTypeClass::As_Reference(Overlay).Land == LAND_TIBERIUM)) {
    static const int _adj[9] = {0, 1, 3, 4, 6, 7, 8, 10, 11};
    static const int _adjgem[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    int count = 0;

    /*
    **	Mixup the Tiberium overlays so that they don't look the same.
    **	Since the type of ore is known, also record the nominal
    **	value per step of that ore type.
    */
    bool gems = false;
    int value = 0;
    if (pregame) {
      switch (Overlay) {
        case OVERLAY_GOLD1:
        case OVERLAY_GOLD2:
        case OVERLAY_GOLD3:
        case OVERLAY_GOLD4:
          value = TheRules().GoldValue;
          Overlay = Random_Pick(OVERLAY_GOLD1, OVERLAY_GOLD4);
          break;

        case OVERLAY_GEMS1:
        case OVERLAY_GEMS2:
        case OVERLAY_GEMS3:
        case OVERLAY_GEMS4:
          gems = true;
          value = TheRules().GemValue * 4;
          Overlay = Random_Pick(OVERLAY_GEMS1, OVERLAY_GEMS4);
          break;

        case OverlayType::OVERLAY_NONE:
        case OverlayType::OVERLAY_SANDBAG_WALL:
        case OverlayType::OVERLAY_CYCLONE_WALL:
        case OverlayType::OVERLAY_BRICK_WALL:
        case OverlayType::OVERLAY_BARBWIRE_WALL:
        case OverlayType::OVERLAY_WOOD_WALL:
        case OverlayType::OVERLAY_V12:
        case OverlayType::OVERLAY_V13:
        case OverlayType::OVERLAY_V14:
        case OverlayType::OVERLAY_V15:
        case OverlayType::OVERLAY_V16:
        case OverlayType::OVERLAY_V17:
        case OverlayType::OVERLAY_V18:
        case OverlayType::OVERLAY_FLAG_SPOT:
        case OverlayType::OVERLAY_WOOD_CRATE:
        case OverlayType::OVERLAY_STEEL_CRATE:
        case OverlayType::OVERLAY_FENCE:
        case OverlayType::OVERLAY_WATER_CRATE:
        default:
          break;
      }
    }

    /*
    **	Add up all adjacent cells that contain tiberium.
    ** (Skip those cells which aren't on the map)
    */
    for (const game::FacingType face :
         magic_enum::enum_values<game::FacingType>()) {
      const CellClass& adj = Adjacent_Cell(face);

      if (adj.Overlay != OVERLAY_NONE &&
          OverlayTypeClass::As_Reference(adj.Overlay).Land == LAND_TIBERIUM) {
        count++;
      }
    }

    if (gems) {
      OverlayData = static_cast<unsigned char>(base::At(_adjgem, count));
      OverlayData = static_cast<unsigned char>(
          std::min(static_cast<int>(OverlayData), 2));
    } else {
      OverlayData = static_cast<unsigned char>(base::At(_adj, count));
    }
    return (static_cast<int32_t>(OverlayData + 1)) * value;
  }

  return 0;
}

/***********************************************************************************************
 * CellClass::Goodie_Check -- Performs crate discovery logic. *
 *                                                                                             *
 *    Call this routine whenever an object enters a cell. It will check for the
 *existence      * of a crate and generate any "goodie" it might contain. *
 *                                                                                             *
 * INPUT:   object   -- Pointer to the object that is triggering this crate. *
 *                                                                                             *
 * OUTPUT:  Can the object continue to enter this cell? A false return value
 *means that the    * cell is now occupied and must not be entered. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/22/1995 JLB : Created. * 07/08/1995 JLB : Added a bunch of
 *goodies to the crates.                                  * 06/17/1996 JLB :
 *Revamped for Red Alert                                                   *
 *=============================================================================================*/
// Not const: opens the crate and grants its reward.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool CellClass::Goodie_Check(FootClass* object) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  if (object != nullptr && Overlay != OVERLAY_NONE &&
      OverlayTypeClass::As_Reference(Overlay).IsCrate) {
    bool force_mcv = false;
    int force_money = 0;
    int damage = 0;
    Coordinate coord{};

    /*
    **	Determine the total number of shares for all the crate powerups. This is
    *used as *	the base pool to determine the odds from.
    */
    int total_shares = 0;
    for (const CrateType index : magic_enum::enum_values<CrateType>()) {
      total_shares += TheRules().crate_shares().at(index);
    }

    /*
    **	Pick a random crate powerup according to the shares allotted to each
    *powerup. *	In solo play, the bonus item is dependant upon the rules
    *control.
    */
    CrateType powerup = CRATE_MONEY;
    if (TheSession().Type == GAME_NORMAL) {
      /*
      **	Solo play has money amount determined by rules.ini file.
      */
      force_money = TheRules().SoloCrateMoney;

      if (Overlay == OVERLAY_STEEL_CRATE) {
        powerup = TheRules().SilverCrate;
      }

      if (Overlay == OVERLAY_WOOD_CRATE) {
        powerup = TheRules().WoodCrate;
      }

      if (Overlay == OVERLAY_WATER_CRATE) {
        powerup = TheRules().WaterCrate;
      }

    } else {
      const int pick = Random_Pick(1, total_shares);

      int share_count = 0;
      bool found = false;
      for (const CrateType candidate : magic_enum::enum_values<CrateType>()) {
        share_count += TheRules().crate_shares().at(candidate);
        if (pick <= share_count) {
          powerup = candidate;
          found = true;
          break;
        }
      }
      CHECK(found) << "crate share table selected nothing";

      /*
      **	Depending on what was picked, there might be an alternate goodie
      *if the selected *	goodie would have no effect.
      */
      switch (powerup) {
        case CRATE_UNIT:
          if (object->House->CurUnits > 50) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_SQUAD:
          if (object->House->CurInfantry > 100) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_DARKNESS:
          if (object->House->IsGPSActive) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_ARMOR:
          if (object->ArmorBias != 1) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_SPEED:
          if (object->SpeedBias != 1 || object->What_Am_I() == RTTI_AIRCRAFT) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_FIREPOWER:
          if (object->FirepowerBias != 1 || !object->Is_Weapon_Equipped()) {
            powerup = CRATE_MONEY;
          }
          break;

        case CRATE_REVEAL:
          if (object->House->IsVisionary) {
            if (object->House->IsGPSActive) {
              powerup = CRATE_MONEY;
            } else {
              powerup = CRATE_DARKNESS;
            }
          }
          break;

        case CRATE_CLOAK:
          if (object->IsCloakable) {
            powerup = CRATE_MONEY;
          }
          break;

          //				case CRATE_HEAL_BASE:
          //					if (object->House->BScan == 0)
          // powerup = CRATE_UNIT;

        case CRATE_MONEY:
          break;

        case CRATE_TIMEQUAKE:
          /*
          ** For the time quake crate, scan through and count up all the
          ** units (and infantry and ships and aircraft) and if either
          ** side has very few, allow the time quake.  Otherwise,
          ** change the crate to money or something.  Only do this for
          ** multiplay - for solo play, they get what they get.  First,
          ** check for time - the chance for getting a time quake crate
          ** should be very very low when they first start the mission,
          ** but as time goes on the chance goes up.
          */
          if (TheSession().Type != GAME_NORMAL) {
            int minunits = 1000;
            bool found_spot = false;
            const int64_t minutes = std::min<int64_t>(
                TheWorld().score().ElapsedTime / kTimerMinute, 100);
            if (Random_Pick(0, 100 - static_cast<int>(minutes)) == 0) {
              for (int i = 0; i < TheSession().Players.Count() +
                                      TheSession().Options.AIPlayers;
                   i++) {
                int ucount = 0;
                HouseClass* hptr =
                    HouseClass::As_Pointer(static_cast<HousesType>(
                        i + static_cast<int>(HOUSE_MULTI1)));
                if (hptr != nullptr && !hptr->IsDefeated) {
                  for (int j = 0;
                       std::cmp_less(j, magic_enum::enum_count<UnitType>());
                       j++) {
                    ucount += hptr->QuantityU(j);
                  }
                  for (int j = 0;
                       std::cmp_less(j, magic_enum::enum_count<InfantryType>());
                       j++) {
                    ucount += hptr->QuantityI(j);
                  }
                  for (int j = 0;
                       std::cmp_less(j, magic_enum::enum_count<AircraftType>());
                       j++) {
                    ucount += hptr->QuantityA(j);
                  }
                  for (int j = 0;
                       std::cmp_less(j, magic_enum::enum_count<VesselType>());
                       j++) {
                    ucount += hptr->QuantityV(j);
                  }
                  int bcount = 0;
                  // BQuantity is three entries short of the type count.
                  for (int j = 0; j < HouseClass::kBuildingQuantityCount; j++) {
                    bcount += hptr->QuantityB(j);
                  }
                  ucount += bcount / 2;  // weight buildings less
                  minunits = std::min(minunits, ucount);
                }
              }
              if (Random_Pick(0, minunits) == minunits) {
                found_spot = true;
              }
            }

            if (!found_spot) {
              powerup = CRATE_MONEY;
            }
          }
          break;
        case CrateType::CRATE_PARA_BOMB:
        case CrateType::CRATE_HEAL_BASE:
        case CrateType::CRATE_EXPLOSION:
        case CrateType::CRATE_NAPALM:
        case CrateType::CRATE_SONAR:
        case CrateType::CRATE_ICBM:
        case CrateType::CRATE_INVULN:
        case CrateType::CRATE_VORTEX:
        default:
          break;
      }
      /*
      **	Possibly force it to be an MCV if there is
      **	sufficient money and no buildings left.
      */
      if (object->House->BScan == 0 &&
          object->House->Available_Money() >
              (BuildingTypeClass::As_Reference(STRUCT_REFINERY).Cost +
               BuildingTypeClass::As_Reference(STRUCT_POWER).Cost) *
                  object->House->CostBias &&
          TheSession().Options.Bases &&
          !(object->House->UScan & kUnitFlagMcv)) {
        powerup = CRATE_UNIT;
        force_mcv = true;
      }

      /*
      **	If the powerup is money but there is insufficient money to build
      *a refinery but there is a construction *	yard available, then force the
      *money to be enough to rebuild the refinery.
      */
      if (powerup == CRATE_MONEY &&
          (object->House->BScan & (kStructFlagConst | kStructFlagRefinery)) ==
              kStructFlagConst &&
          object->House->Available_Money() <
              BuildingTypeClass::As_Reference(STRUCT_REFINERY).Cost *
                  object->House->CostBias) {
        force_money = BuildingTypeClass::As_Reference(STRUCT_REFINERY).Cost *
                      object->House->CostBias;
      }

      /*
      **	Special override for water crates so that illegal goodies items
      **	won't appear.
      */
      if (Overlay == OVERLAY_WATER_CRATE) {
        switch (powerup) {
          case CRATE_UNIT:
          case CRATE_SQUAD:
            powerup = CRATE_MONEY;
            break;

          case CrateType::CRATE_MONEY:
          case CrateType::CRATE_PARA_BOMB:
          case CrateType::CRATE_HEAL_BASE:
          case CrateType::CRATE_CLOAK:
          case CrateType::CRATE_EXPLOSION:
          case CrateType::CRATE_NAPALM:
          case CrateType::CRATE_DARKNESS:
          case CrateType::CRATE_REVEAL:
          case CrateType::CRATE_SONAR:
          case CrateType::CRATE_ARMOR:
          case CrateType::CRATE_SPEED:
          case CrateType::CRATE_FIREPOWER:
          case CrateType::CRATE_ICBM:
          case CrateType::CRATE_TIMEQUAKE:
          case CrateType::CRATE_INVULN:
          case CrateType::CRATE_VORTEX:
          default:
            break;
        }
      }
    }

    /*
    ** Keep track of the number of each type of crate found
    */
    if (TheSession().Type == GAME_INTERNET) {
      object->House->TotalCrates->Increment_Unit_Total(
          static_cast<int>(powerup));
    }

    /*
    **	Remove the crate from the map.
    */
    TheMap().Remove_Crate(Cell_Number());
    //		Map[Cell_Number()].Overlay = OVERLAY_NONE;

    if (TheSession().Type != GAME_NORMAL && TheRules().IsMPCrates) {
      TheMap().Place_Random_Crate();
    }

    /*
    **	Generate any corresponding animation associated with this crate powerup.
    */
    if (TheRules().crate_anims().at(powerup) != ANIM_NONE) {
      base::NewPooled<AnimClass>(TheRules().crate_anims().at(powerup),
                                 Center_Coord());
    }

    /*
    **	Create the effect requested.
    */
    bool tospeak = false;
    const auto give_crate_money = [&] {
      if (force_money > 0) {
        object->House->Refund_Money(force_money);
      } else {
        object->House->Refund_Money(
            Random_Pick(TheRules().crate_data().at(powerup),
                        TheRules().crate_data().at(powerup) + 900));
      }
    };
    switch (powerup) {
      case CRATE_TIMEQUAKE:
        TheWorld().time_quake() = true;
        break;

      /*
      **	Give the player money.
      */
      case CRATE_MONEY:
        give_crate_money();
        break;

      /*
      **	Shroud the world in blackness.
      */
      case CRATE_DARKNESS:
        if (object->House == ThePlayer()) {
          TheMap().Shroud_The_Map();
        }
        break;

      /*
      **	Reveal the entire map.
      */
      case CRATE_REVEAL:
        object->House->IsVisionary = true;
        if (object->House == ThePlayer()) {
          for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
            const Cell cell = Cell::FromIndex(cell_index);
            TheMap().Map_Cell(cell, ThePlayer());
          }
          TheMap().Flag_To_Redraw(true);
        }
        break;

      /*
      **	Try to create a unit where the crate was.
      */
      case CRATE_UNIT: {
        const UnitTypeClass* utp = nullptr;

        /*
        **	Give the player an MCV if he has no base left but does have more
        *than enough *	money to rebuild a new base. Of course, if he already
        *has an MCV, then don't *	give him another one.
        */
        if (force_mcv) {
          utp = &UnitTypeClass::As_Reference(UNIT_MCV);
        }

        /*
        **	If the player has a base and a refinery, but no harvester, then
        *give him *	a free one.
        */
        if (utp == nullptr && object->House->BScan & kStructFlagRefinery &&
            !(object->House->UScan & kUnitFlagHarvester)) {
          utp = &UnitTypeClass::As_Reference(UNIT_HARVESTER);
        }

        /*
        **	Check for special unit type override value.
        */
        if (TheRules().UnitCrateType != UNIT_NONE) {
          utp = &UnitTypeClass::As_Reference(TheRules().UnitCrateType);
        }

        /*
        **	If no unit type has been determined, then pick one at random.
        */
        while (utp == nullptr) {
          const UnitType utype = Random_Pick(
              magic_enum::enum_values<UnitType>().front(),
              static_cast<UnitType>(HouseClass::kOriginalUnitCount - 1 - 3));
          if (utype != UNIT_MCV || TheSession().Options.Bases) {
            utp = &UnitTypeClass::As_Reference(utype);
            if (utp->IsCrateGoodie &&
                (utp->Ownable &
                 base::Bit<uint32_t>(
                     HouseClass::As_Pointer(object->Owner())->ActLike)) != 0) {
              break;
            }
            utp = nullptr;
          }
        }

        if (utp != nullptr) {
          auto* goodie_unit =
              dynamic_cast<UnitClass*>(utp->Create_One_Of(object->House));
          if (goodie_unit != nullptr) {
            if (goodie_unit->Unlimbo(Center_Coord())) {
              return false;
            }

            /*
            **	Try to place the object into a nearby cell if something is
            *preventing *	placement at the crate location.
            */
            const Cell cell = TheMap().Nearby_Location(
                Cell_Number(), goodie_unit->Class->Speed);
            if (goodie_unit->Unlimbo(cell.Center())) {
              return false;
            }
            base::DeletePooled(goodie_unit);
            give_crate_money();
            break;
          }
        }
      } break;

      /*
      **	Create a squad of miscellaneous composition.
      */
      case CRATE_SQUAD:
        for (int index = 0; index < 5; index++) {
          static const InfantryType _inf[] = {
              INFANTRY_E1, INFANTRY_E1, INFANTRY_E1,
              INFANTRY_E1, INFANTRY_E1, INFANTRY_E1,
              INFANTRY_E2, INFANTRY_E3, INFANTRY_RENOVATOR};
          if ((!InfantryTypeClass::As_Reference(
                    base::At(_inf, Random_Pick<int>(0, std::ssize(_inf) - 1)))
                    .Create_And_Place(Cell_Number(), object->Owner())) &&
              (index == 0)) {
            give_crate_money();
            break;
          }
        }
        return false;

      /*
      **	A one para-bomb mission.
      */
      case CRATE_PARA_BOMB:
        if (object->House->SuperWeapon.at(SPC_PARA_BOMB).Enable(true) &&
            object->IsOwnedByPlayer) {
          TheMap().Add(RTTI_SPECIAL, static_cast<int>(SPC_PARA_BOMB));
          base::At(TheMap().Column, 1).Flag_To_Redraw();
        }

        break;

      /*
      **	A one time sonar pulse
      */
      case CRATE_SONAR:
        if (object->House->SuperWeapon.at(SPC_SONAR_PULSE).Enable(true) &&
            object->IsOwnedByPlayer) {
          TheMap().Add(RTTI_SPECIAL, static_cast<int>(SPC_SONAR_PULSE));
          base::At(TheMap().Column, 1).Flag_To_Redraw();
        }

        break;

      /*
      **	A group of explosions are triggered around the crate.
      */
      case CRATE_EXPLOSION:
        if (object != nullptr) {
          int d = TheRules().crate_data().at(powerup);
          object->Take_Damage(d, 0, WARHEAD_HE, nullptr, true);
        }
        for (int index = 0; index < 5; index++) {
          const Coordinate frag_coord =
              Coord_Scatter(Center_Coord(), Random_Pick(0, 0x0200));
          base::NewPooled<AnimClass>(ANIM_FBALL1, frag_coord);
          damage = TheRules().crate_data().at(powerup);
          Explosion_Damage(frag_coord, damage, nullptr, WARHEAD_HE);
        }
        break;

      /*
      **	A napalm blast is triggered.
      */
      case CRATE_NAPALM:
        coord = Coordinate::Midpoint(Center_Coord(), object->Center_Coord());
        base::NewPooled<AnimClass>(ANIM_NAPALM3, coord);
        if (object != nullptr) {
          int d = TheRules().crate_data().at(powerup);
          object->Take_Damage(d, 0, WARHEAD_FIRE, nullptr, true);
        }
        damage = TheRules().crate_data().at(powerup);
        Explosion_Damage(coord, damage, nullptr, WARHEAD_FIRE);
        break;

      /*
      **	All objects within a certain range will gain the ability to
      *cloak.
      */
      case CRATE_CLOAK:
        for (int index = 0;
             index < DisplayClass::Layer.at(LAYER_GROUND).Count(); index++) {
          ObjectClass* obj = DisplayClass::Layer.at(LAYER_GROUND).at(index);

          if (obj && obj->Is_Techno() &&
              Distance(Center_Coord(), obj->Center_Coord()) <
                  TheRules().CrateRadius) {
            dynamic_cast<TechnoClass*>(obj)->IsCloakable = true;
          }
        }
        break;

      /*
      **	All of the player's objects heal up.
      */
      case CRATE_HEAL_BASE:
        if (object->IsOwnedByPlayer) {
          PlaySoundEffectAt(VOC_HEAL, object->Center_Coord());
        }
        for (int index = 0; index < TheWorld().logic().Count(); index++) {
          ObjectClass* obj = TheWorld().logic().at(index);

          if (obj && object->Is_Techno() &&
              object->House->Class->House == obj->Owner()) {
            obj->Strength = obj->Class_Of().MaxStrength;
          }
        }
        break;

      case CRATE_ICBM:
        if (object->House->SuperWeapon.at(SPC_NUCLEAR_BOMB).Enable(true) &&
            object->IsOwnedByPlayer) {
          TheMap().Add(RTTI_SPECIAL, static_cast<int>(SPC_NUCLEAR_BOMB));
          base::At(TheMap().Column, 1).Flag_To_Redraw();
        }

        break;

      case CRATE_ARMOR:
        for (int index = 0;
             index < DisplayClass::Layer.at(LAYER_GROUND).Count(); index++) {
          ObjectClass* obj = DisplayClass::Layer.at(LAYER_GROUND).at(index);

          if (obj != nullptr && obj->Is_Techno() &&
              Distance(Center_Coord(), obj->Center_Coord()) <
                  TheRules().CrateRadius &&
              dynamic_cast<TechnoClass*>(obj)->ArmorBias == 1) {
            const Fixed val =
                dynamic_cast<TechnoClass*>(obj)->ArmorBias *
                Fixed(TheRules().crate_data().at(powerup), 256).Invert();
            dynamic_cast<TechnoClass*>(obj)->ArmorBias = val;
            if (obj->Owner() == ThePlayer()->Class->House) {
              tospeak = true;
            }
          }
        }
        if (tospeak) {
          Speak(VOX_UPGRADE_ARMOR);
        }
        break;

      case CRATE_SPEED:
        for (int index = 0;
             index < DisplayClass::Layer.at(LAYER_GROUND).Count(); index++) {
          ObjectClass* obj = DisplayClass::Layer.at(LAYER_GROUND).at(index);

          if (obj && obj->Is_Foot() &&
              Distance(Center_Coord(), obj->Center_Coord()) <
                  TheRules().CrateRadius &&
              dynamic_cast<FootClass*>(obj)->SpeedBias == 1 &&
              obj->What_Am_I() != RTTI_AIRCRAFT) {
            auto* foot = dynamic_cast<FootClass*>(obj);

            const Fixed val = foot->SpeedBias *
                              Fixed(TheRules().crate_data().at(powerup), 256);
            foot->SpeedBias = val;
            if (foot->IsOwnedByPlayer) {
              tospeak = true;
            }
          }
        }
        if (tospeak) {
          Speak(VOX_UPGRADE_SPEED);
        }
        break;

      case CRATE_FIREPOWER:
        for (int index = 0;
             index < DisplayClass::Layer.at(LAYER_GROUND).Count(); index++) {
          ObjectClass* obj = DisplayClass::Layer.at(LAYER_GROUND).at(index);

          if (obj && obj->Is_Techno() &&
              Distance(Center_Coord(), obj->Center_Coord()) <
                  TheRules().CrateRadius &&
              dynamic_cast<TechnoClass*>(obj)->FirepowerBias == 1) {
            const Fixed val = dynamic_cast<TechnoClass*>(obj)->FirepowerBias *
                              Fixed(TheRules().crate_data().at(powerup), 256);
            dynamic_cast<TechnoClass*>(obj)->FirepowerBias = val;
            if (obj->Owner() == ThePlayer()->Class->House) {
              tospeak = true;
            }
          }
        }
        if (tospeak) {
          Speak(VOX_UPGRADE_FIREPOWER);
        }
        break;

      case CRATE_INVULN:
        for (int index = 0;
             index < DisplayClass::Layer.at(LAYER_GROUND).Count(); index++) {
          ObjectClass* obj = DisplayClass::Layer.at(LAYER_GROUND).at(index);

          if (obj && obj->Is_Techno() &&
              Distance(Center_Coord(), obj->Center_Coord()) <
                  TheRules().CrateRadius) {
            dynamic_cast<TechnoClass*>(obj)->IronCurtainCountDown.Set(
                kTicksPerMinute *
                Fixed(TheRules().crate_data().at(powerup), 256));
            obj->Mark(MARK_CHANGE);
          }
        }
        break;

      /*
      ** A chronal vortex appears targetted at the triggering object.
      */
      case CRATE_VORTEX:
        if (!TheWorld().chronal_vortex().Is_Active()) {
          TheWorld().chronal_vortex().Appear(Center_Coord());
          TheWorld().chronal_vortex().Set_Target(object);
          PlaySoundEffectAt(VOC_TESLA_ZAP, object->Center_Coord());
        }
        break;

      default:
        break;
    }
  }
  return true;
}

/***********************************************************************************************
 * CellClass::Flag_Place -- Places a house flag down on the cell. *
 *                                                                                             *
 *    This routine will place the house flag at this cell location. *
 *                                                                                             *
 * INPUT:   house -- The house that is having its flag placed here. *
 *                                                                                             *
 * OUTPUT:  Was the flag successfully placed here? *
 *                                                                                             *
 * WARNINGS:   Failure to place means that the cell is impassable for some
 *reason.             *
 *                                                                                             *
 * HISTORY: * 05/23/1995 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Flag_Place(HousesType house) {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  if (!IsFlagged && Is_Clear_To_Move(SPEED_TRACK, false, false)) {
    IsFlagged = true;
    Owner = house;
    Redraw_Objects();
    return true;
  }
  return false;
}

/***********************************************************************************************
 * CellClass::Flag_Remove -- Removes the house flag from the cell. *
 *                                                                                             *
 *    This routine will free the cell of any house flag that may be located
 *there.             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Was there a flag here that was removed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/23/1995 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Flag_Remove() {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  if (IsFlagged) {
    IsFlagged = false;
    Owner = HOUSE_NONE;
    Redraw_Objects();
    return true;
  }
  return false;
}

/***********************************************************************************************
 * CellClass::Shimmer -- Causes all objects in the cell to shimmer. *
 *                                                                                             *
 *    This routine is called when some event would cause a momentary disruption
 *in the         * cloaking device. All objects that are cloaked in the cell
 *will have their cloaking       * device shimmer. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
// Not const: starts the occupants' shimmer effect.
// NOLINTNEXTLINE(readability-make-member-function-const)
void CellClass::Shimmer() {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  ObjectClass* object = Cell_Occupier();

  while (object) {
    object->Do_Shimmer();
    object = object->Next;
  }
}

/***********************************************************************************************
 * CellClass::Is_Clear_To_Move -- Determines if the cell is generally clear for
 *travel         *
 *                                                                                             *
 *    This routine is called when determining general passability for purposes
 *of zone         * calculation. Only blockages that cannot be circumvented are
 *considered to make a cell    * impassable. All other obstructions can either
 *be destroyed or are temporary.             *
 *                                                                                             *
 * INPUT:   loco     -- The locomotion type to use when determining
 *passablility.              *
 *                                                                                             *
 *          ignoreinfantry -- Should infantry in the cell be ignored for
 *movement purposes?    *
 *                                                                                             *
 *          ignorevehicles -- If vehicles should be ignored, then this flag will
 *be true.      *
 *                                                                                             *
 *          zone     -- If specified, the zone must match this value or else
 *movement is       * presumed disallowed. *
 *                                                                                             *
 *          check    -- This specifies the zone type that this check applies to.
 **
 *                                                                                             *
 * OUTPUT:  Is the cell generally passable to ground targeting? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/25/1995 JLB : Created. * 06/25/1996 JLB : Uses tracked vehicles
 *as a basis for zone check.                         * 10/05/1996 JLB : Allows
 *checking for crushable blockages.                                 *
 *=============================================================================================*/
bool CellClass::Is_Clear_To_Move(SpeedType loco, bool ignoreinfantry,
                                 bool ignorevehicles, int zone,
                                 MZoneType check) const {
  DCHECK(static_cast<unsigned>(Cell_Number().index()) <= MAP_CELL_TOTAL);

  /*
  **	Flying objects always consider every cell passable since they can fly
  *over everything.
  */
  if (loco == SPEED_WINGED) {
    return true;
  }

  /*
  **	If a zone was specified, then see if the cell is in a legal
  **	zone to allow movement.
  */
  if ((zone != -1) && std::cmp_not_equal(zone, Zones.at(check))) {
    return false;
  }

  /*
  **	Check the occupy bits for passable legality. If ignore infantry is true,
  *then *	don't consider infnatry.
  */
  uint32_t composite = Flag.Composite;
  if (ignoreinfantry) {
    composite &= 0xE0U;  // Drop the infantry occupation bits.
  }
  if (ignorevehicles) {
    composite &= 0x5FU;  // Drop the vehicle/building bit.
  }
  if (composite != 0) {
    return false;
  }

  /*
  **	Fetch the land type of the cell -- to be modified and used later.
  */
  LandType land = Land_Type();

  /*
  **	Walls are always considered to block the terrain for general passability
  **	purposes unless this is a wall crushing check or if the checking object
  **	can destroy walls.
  */
  const OverlayTypeClass* overlay = nullptr;
  if (Overlay != OVERLAY_NONE) {
    overlay = &OverlayTypeClass::As_Reference(Overlay);
  }
  if (overlay != nullptr && overlay->IsWall) {
    if (check != MZONE_DESTROYER &&
        (check != MZONE_CRUSHER || !overlay->IsCrushable)) {
      return false;
    }

    /*
    **	Crushing objects consider crushable walls as clear rather than the
    **	typical LAND_WALL setting.
    */
    land = LAND_CLEAR;
  }

  /*
  **	See if the ground type is impassable to this locomotion type and if
  **	so, return the error condition.
  */
  if (TheRules().ground().at(land).Cost.at(loco) == 0) {
    return false;
  }

  /*
  **	All checks passed, so this cell must be passable.
  */
  return true;
}

/***********************************************************************************************
 * CellClass::Is_Bridge_Here -- Checks to see if this is a bridge occupied cell.
 **
 *                                                                                             *
 *    This routine will examine this cell and if there is a bridge here, it will
 *return        * true. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is there a bridge located in this cell? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/30/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Is_Bridge_Here() const {
  const TemplateType bridge_type = TType;
  return bridge_type == TEMPLATE_BRIDGE1 || bridge_type == TEMPLATE_BRIDGE1H ||
         bridge_type == TEMPLATE_BRIDGE1D || bridge_type == TEMPLATE_BRIDGE2 ||
         bridge_type == TEMPLATE_BRIDGE2H || bridge_type == TEMPLATE_BRIDGE2D ||
         bridge_type == TEMPLATE_BRIDGE_1A ||
         bridge_type == TEMPLATE_BRIDGE_1B ||
         bridge_type == TEMPLATE_BRIDGE_2A ||
         bridge_type == TEMPLATE_BRIDGE_2B ||
         bridge_type == TEMPLATE_BRIDGE_3A ||
         bridge_type == TEMPLATE_BRIDGE_3B ||
         bridge_type == TEMPLATE_BRIDGE_3C ||
         bridge_type == TEMPLATE_BRIDGE_3D ||
         bridge_type == TEMPLATE_BRIDGE_3E || bridge_type == TEMPLATE_BRIDGE_3F;
}

/***********************************************************************************************
 * CellClass::Can_Tiberium_Grow -- Determines if Tiberium can grow in this cell.
 **
 *                                                                                             *
 *    This checks the cell to see if Tiberium can grow at least one level in it.
 *Tiberium can  * grow only if there is Tiberium already present. It can only
 *grow to a certain level      * and then all further growth is suspended. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Can Tiberium grow in this cell? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/14/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Can_Tiberium_Grow() const {
  if (!TheRules().IsTGrowth) {
    return false;
  }

  if ((TheSession().Type != GAME_NORMAL) && (!TheSession().Options.Tiberium)) {
    return false;
  }

  if (Land_Type() != LAND_TIBERIUM) {
    return false;
  }

  if (OverlayData >= 11) {
    return false;
  }

  if (Overlay != OVERLAY_GOLD1 && Overlay != OVERLAY_GOLD2 &&
      Overlay != OVERLAY_GOLD3 && Overlay != OVERLAY_GOLD4) {
    return false;
  }

  return true;
}

/***********************************************************************************************
 * CellClass::Can_Tiberium_Spread -- Determines if Tiberium can spread from this
 *cell.         *
 *                                                                                             *
 *    This routine will examine the cell and determine if there is sufficient
 *Tiberium         * present that Tiberium spores will spread to adjacent cells.
 *If the Tiberium level is     * too low, spreading will not occur. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Can Tiberium spread from this cell into adjacent cells? *
 *                                                                                             *
 * WARNINGS:   This routine does not check to see if, in fact, there are any
 *adjacent cells    * available to spread to. *
 *                                                                                             *
 * HISTORY: * 08/14/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Can_Tiberium_Spread() const {
  if (!TheRules().IsTSpread) {
    return false;
  }

  if ((TheSession().Type != GAME_NORMAL) && (!TheSession().Options.Tiberium)) {
    return false;
  }

  if (Land_Type() != LAND_TIBERIUM) {
    return false;
  }

  if (OverlayData <= 6) {
    return false;
  }

  if (Overlay != OVERLAY_GOLD1 && Overlay != OVERLAY_GOLD2 &&
      Overlay != OVERLAY_GOLD3 && Overlay != OVERLAY_GOLD4) {
    return false;
  }

  return true;
}

/***********************************************************************************************
 * CellClass::Grow_Tiberium -- Grows the tiberium in the cell. *
 *                                                                                             *
 *    This routine will cause the tiberium to grow in the cell. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Did Tiberium grow in the cell? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/14/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Grow_Tiberium() {
  if (Can_Tiberium_Grow()) {
    OverlayData++;
    Redraw_Objects();
    return true;
  }
  return false;
}

/***********************************************************************************************
 * CellClass::Spread_Tiberium -- Spread Tiberium from this cell to an adjacent
 *cell.           *
 *                                                                                             *
 *    This routine will cause the Tiberium to spread from this cell into an
 *adjacent (random)  * cell. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Did the Tiberium spread? *
 *                                                                                             *
 * WARNINGS:   If there are no adjacent cells that the tiberium can spread to,
 *then this       * routine will fail. *
 *                                                                                             *
 * HISTORY: * 08/14/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Spread_Tiberium(bool forced) {
  if ((!forced) && (!Can_Tiberium_Spread())) {
    return false;
  }

  const game::FacingType offset = Random_Pick(game::FACING_N, game::FACING_NW);
  // Placing the overlay is the point of the search, so not any_of.
  // NOLINTNEXTLINE(readability-use-anyofallof)
  for (const game::FacingType index :
       magic_enum::enum_values<game::FacingType>()) {
    CellClass* newcell = &Adjacent_Cell(index + offset);

    if (newcell != nullptr && newcell->Can_Tiberium_Germinate()) {
      base::NewPooled<OverlayClass>(Random_Pick(OVERLAY_GOLD1, OVERLAY_GOLD4),
                                    newcell->Cell_Number());
      newcell->OverlayData = 0;
      return true;
    }
  }
  return false;
}

/***********************************************************************************************
 * CellClass::Can_Tiberium_Germinate -- Determines if Tiberium can begin growth
 *in the cell.   *
 *                                                                                             *
 *    This routine will examine the cell and determine if Tiberium can start
 *growth in it.     *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Can Tiberium grow in this cell? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/14/1996 JLB : Created. *
 *=============================================================================================*/
bool CellClass::Can_Tiberium_Germinate() const {
  if (!TheMap().In_Radar(Cell_Number())) {
    return false;
  }

  if (Is_Bridge_Here()) {
    return false;
  }

  /*
  **	Don't allow Tiberium to grow on a cell with a building unless that
  *building is *	invisible. In such a case, the Tiberium must grow or
  *else the location of the *	building will be revealed.
  */
  const BuildingClass* building = Cell_Building();
  if (building != nullptr && !building->Class->IsInvisible) {
    return false;
  }

  if (!TheRules().ground().at(Land_Type()).Build) {
    return false;
  }

  if (Overlay != OVERLAY_NONE) {
    return false;
  }

  return true;
}

CellClass& CellClass::Adjacent_Cell(game::FacingType face) {
  const int offset = Adjacent_Offset(face);
  return offset == 0 ? *this : TheMap().at(Cell_Number().Offset(offset));
}

const CellClass& CellClass::Adjacent_Cell(game::FacingType face) const {
  const int offset = Adjacent_Offset(face);
  return offset == 0 ? *this : TheMap().at(Cell_Number().Offset(offset));
}

}  // namespace ra
