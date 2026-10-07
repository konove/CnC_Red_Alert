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

/* $Header: /CounterStrike/ABSTRACT.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : ABSTRACT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/26/95 *
 *                                                                                             *
 *                  Last Update : January 26, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_OBJECTS_ABSTRACT_H_
#define CNC_RED_ALERT_RA_OBJECTS_ABSTRACT_H_

#include "game/sim/direction.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/packed.h"

namespace ra {

// Base class for all game objects that exist on the battlefield.
class AbstractClass {
 public:
  RTTIType RTTI;
  int ID;  // Also serves as the index into the type-specific object heap.

  // Object position. Center point for vehicles, upper-left corner for
  // buildings.
  Coordinate Coord;

  int Height;  // Above-ground height in leptons.

  // Whether this slot in the fixed-size object heap is in use.
  bool IsActive : 1 = true;

  AbstractClass(const RTTIType rtti, const int id)
      : RTTI(rtti), ID(id), Coord(Coordinate::NoPosition()), Height(0) {}

  // Saved-game support for the base part; derived classes call it first.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_active = IsActive;
    ar(RTTI, ID, Coord, Height, is_active);
    IsActive = is_active;
  }

 protected:
  // Shell for ObjectHeap::Load; Serialize() supplies every value.
  // IsActive starts true because each heap type's operator new sets it
  // before a normal constructor runs, and the shell must not disagree.
  AbstractClass()
      : RTTI(RTTI_NONE), ID(-1), Coord(Coordinate::NoPosition()), Height(0) {}

 public:
  virtual ~AbstractClass();
  AbstractClass(const AbstractClass&) = delete;
  AbstractClass& operator=(const AbstractClass&) = delete;
  AbstractClass(AbstractClass&&) = delete;
  AbstractClass& operator=(AbstractClass&&) = delete;

  [[nodiscard]] virtual const char* Name() const { return ""; }
  [[nodiscard]] virtual HousesType Owner() const { return HOUSE_NONE; }
  [[nodiscard]] Target As_Target() const { return Target::Make(RTTI, ID); }
  [[nodiscard]] RTTIType What_Am_I() const { return RTTI; }

  [[nodiscard]] virtual Coordinate Center_Coord() const { return Coord; }
  [[nodiscard]] virtual Coordinate Target_Coord() const { return Coord; }

  game::Direction DirectionTo(const AbstractClass* object) const {
    return ra::DirectionBetween(Center_Coord(), object->Target_Coord());
  }
  [[nodiscard]] game::Direction DirectionTo(const Coordinate coord) const {
    return ra::DirectionBetween(Center_Coord(), coord);
  }
  [[nodiscard]] game::Direction DirectionTo(Target target) const;
  [[nodiscard]] game::Direction DirectionTo(const Cell cell) const {
    return ra::DirectionBetween(Cell::Containing(Center_Coord()), cell);
  }

  // Returns distance in leptons to the target. For buildings, subtracts
  // the building's average radius so range checks measure to the edge.
  [[nodiscard]] int Distance(Target target) const;
  [[nodiscard]] int Distance(const Coordinate coord) const {
    return ra::Distance(Center_Coord(), coord);
  }
  int Distance(const AbstractClass* object) const {
    return ra::Distance(Center_Coord(), object->Target_Coord());
  }

  [[nodiscard]] virtual game::MoveType Can_Enter_Cell(
      Cell /*unused*/,
      game::FacingType /*unused*/ = game::FacingType::kNone) const {
    return game::MOVE_OK;
  }

  virtual void AI() {}
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_ABSTRACT_H_
