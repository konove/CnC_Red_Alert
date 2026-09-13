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

#ifndef CNC_RED_ALERT_RA_ABSTRACT_H_
#define CNC_RED_ALERT_RA_ABSTRACT_H_

#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/monoc.h"

// Base class for all game objects that exist on the battlefield.
class AbstractClass {
 public:
  RTTIType RTTI;
  int ID;  // Also serves as the index into the type-specific object heap.

  // Object position. Center point for vehicles, upper-left corner for
  // buildings.
  COORDINATE Coord;

  int Height;  // Above-ground height in leptons.

  // Whether this slot in the fixed-size object heap is in use.
  unsigned IsActive : 1 = true;

  AbstractClass(const RTTIType rtti, const int id)
      : RTTI(rtti),
        ID(id),
        Coord(0xFFFFFFFFL),  // Sentinel: no position assigned.
        Height(0) {}

  // Saved-game support for the base part; derived classes call it first.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_active = IsActive;
    ar(RTTI, ID, Coord, Height, is_active);
    IsActive = is_active;
  }

 protected:
  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  // IsActive starts true because each heap type's operator new sets it
  // before a normal constructor runs, and the shell must not disagree.
  AbstractClass() : RTTI(RTTI_NONE), ID(-1), Coord(0xFFFFFFFFL), Height(0) {}

 public:
  virtual ~AbstractClass() = default;
  AbstractClass(const AbstractClass&) = delete;
  AbstractClass& operator=(const AbstractClass&) = delete;
  AbstractClass(AbstractClass&&) = delete;
  AbstractClass& operator=(AbstractClass&&) = delete;

  [[nodiscard]] virtual const char* Name() const { return ""; }
  [[nodiscard]] virtual HousesType Owner() const { return HOUSE_NONE; }
  [[nodiscard]] TARGET As_Target() const { return Build_Target(RTTI, ID); }
  [[nodiscard]] RTTIType What_Am_I() const { return RTTI; }

  virtual void Debug_Dump(MonoClass* mono) const;

  [[nodiscard]] virtual COORDINATE Center_Coord() const { return Coord; }
  [[nodiscard]] virtual COORDINATE Target_Coord() const { return Coord; }

  DirType Direction(const AbstractClass* object) const {
    return ::Direction(Center_Coord(), object->Target_Coord());
  }
  [[nodiscard]] DirType Direction(const COORDINATE coord) const {
    return ::Direction(Center_Coord(), coord);
  }
  [[nodiscard]] DirType Direction(TARGET target) const;
  [[nodiscard]] DirType Direction(const CELL cell) const {
    return ::Direction(Coord_Cell(Center_Coord()), cell);
  }

  // Returns distance in leptons to the target. For buildings, subtracts
  // the building's average radius so range checks measure to the edge.
  [[nodiscard]] int Distance(TARGET target) const;
  [[nodiscard]] int Distance(const COORDINATE coord) const {
    return ::Distance(Center_Coord(), coord);
  }
  int Distance(const AbstractClass* object) const {
    return ::Distance(Center_Coord(), object->Target_Coord());
  }

  [[nodiscard]] virtual MoveType Can_Enter_Cell(
      CELL, FacingType = FACING_NONE) const {
    return MOVE_OK;
  }

  virtual void AI() {}
};

#endif  // CNC_RED_ALERT_RA_ABSTRACT_H_
