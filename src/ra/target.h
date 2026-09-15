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

/* $Header: /CounterStrike/TARGET.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TARGET.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 25, 1994 *
 *                                                                                             *
 *                  Last Update : April 25, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_TARGET_H_
#define CNC_RED_ALERT_RA_TARGET_H_

#include <bit>

#include "ra/abstract.h"
#include "ra/ccini.h"
#include "ra/defines.h"
#include "ra/object.h"

class AbstractTypeClass;
class AircraftClass;
class AnimClass;
class BulletClass;
class CellClass;
class InfantryClass;
class TeamClass;
class TeamTypeClass;
class TechnoTypeClass;
class TerrainClass;
class UnitClass;
class VesselClass;

inline RTTIType Target_Kind(TARGET a) {
  return static_cast<RTTIType>(std::bit_cast<TARGET_COMPOSITE>(a).Sub.Exponent);
}

inline int Target_Value(TARGET a) {
  const TARGET_COMPOSITE composite{.Target = a};
  return static_cast<int>(composite.Sub.Mantissa);
}

inline bool Is_Target_Team(TARGET a) { return Target_Kind(a) == RTTI_TEAM; }
inline bool Is_Target_TeamType(TARGET a) {
  return Target_Kind(a) == RTTI_TEAMTYPE;
}
inline bool Is_Target_Trigger(TARGET a) {
  return Target_Kind(a) == RTTI_TRIGGER;
}
inline bool Is_Target_TriggerType(TARGET a) {
  return Target_Kind(a) == RTTI_TRIGGERTYPE;
}
inline bool Is_Target_Infantry(TARGET a) {
  return Target_Kind(a) == RTTI_INFANTRY;
}
inline bool Is_Target_Bullet(TARGET a) { return Target_Kind(a) == RTTI_BULLET; }
inline bool Is_Target_Terrain(TARGET a) {
  return Target_Kind(a) == RTTI_TERRAIN;
}
inline bool Is_Target_Cell(TARGET a) { return Target_Kind(a) == RTTI_CELL; }
inline bool Is_Target_Unit(TARGET a) { return Target_Kind(a) == RTTI_UNIT; }
inline bool Is_Target_Vessel(TARGET a) { return Target_Kind(a) == RTTI_VESSEL; }
inline bool Is_Target_Building(TARGET a) {
  return Target_Kind(a) == RTTI_BUILDING;
}
inline bool Is_Target_Template(TARGET a) {
  return Target_Kind(a) == RTTI_TEMPLATE;
}
inline bool Is_Target_Aircraft(TARGET a) {
  return Target_Kind(a) == RTTI_AIRCRAFT;
}
inline bool Is_Target_Animation(TARGET a) {
  return Target_Kind(a) == RTTI_ANIM;
}
inline bool Is_Target_Object(TARGET a) {
  return Target_Kind(a) == RTTI_TERRAIN || Target_Kind(a) == RTTI_UNIT ||
         Target_Kind(a) == RTTI_VESSEL || Target_Kind(a) == RTTI_INFANTRY ||
         Target_Kind(a) == RTTI_BUILDING || Target_Kind(a) == RTTI_AIRCRAFT;
}

TARGET As_Target(CELL cell);
TARGET As_Target(COORDINATE coord);
// inline TARGET As_Target(CELL cell) {return (TARGET)(((unsigned)RTTI_CELL <<
// kTargetMantissaBits) | cell);}

/*
** Must not have a constructor since Watcom cannot handle a class that has a
*constructor if
** that class object is in a union. Don't use this class for normal purposes.
*Use the TargetClass *	instead. The xTargetClass is only used in one module for
*a special reason -- keep it that way.
*/
class xTargetClass {
 protected:
  TARGET_COMPOSITE Target;

 public:
  // conversion operator to RTTIType
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator RTTIType() const {
    return static_cast<RTTIType>(Target.Sub.Exponent);
  }

  // comparison operator
  int operator==(const xTargetClass& tgt) const {
    return tgt.Target.Target == Target.Target ? 1 : 0;
  }

  // conversion operator to regular TARGET type
  [[nodiscard]] TARGET As_TARGET() const { return Target.Target; }

  [[nodiscard]] unsigned Value() const { return Target.Sub.Mantissa; }

  void Invalidate() {
    Target.Sub.Exponent = RTTI_NONE;
    Target.Sub.Mantissa = (1 << kTargetMantissaBits) - 1;
  }
  [[nodiscard]] bool Is_Valid() const {
    return Target.Sub.Exponent != RTTI_NONE;
  }

  [[nodiscard]] TARGET As_Target() const { return Target.Target; }
  [[nodiscard]] AbstractTypeClass* As_TypeClass() const;
  [[nodiscard]] AbstractClass* As_Abstract() const;
  [[nodiscard]] TechnoClass* As_Techno() const;
  [[nodiscard]] ObjectClass* As_Object() const;
  [[nodiscard]] CellClass* As_Cell() const;

  // Helpers that test for, and fetch a pointer to, the kind of object named.
  // Each returns the object this target refers to when the target is of that
  // kind, or nullptr otherwise.
  [[nodiscard]] TriggerTypeClass* As_TriggerType() const;
  [[nodiscard]] TeamTypeClass* As_TeamType() const;
  [[nodiscard]] TerrainClass* As_Terrain() const;
  [[nodiscard]] BulletClass* As_Bullet() const;
  [[nodiscard]] AnimClass* As_Anim() const;
  [[nodiscard]] TeamClass* As_Team() const;
  [[nodiscard]] InfantryClass* As_Infantry() const;
  [[nodiscard]] UnitClass* As_Unit() const;
  [[nodiscard]] BuildingClass* As_Building() const;
  [[nodiscard]] AircraftClass* As_Aircraft() const;
  [[nodiscard]] VesselClass* As_Vessel() const;
};

/*
**	This class only serves as a wrapper to the xTargetClass. This class must
*not define any members except *	for the constructors. This is because
*the xTargetClass is used in a union and this target object is *	used as
*its initializer. If this class had any extra members they would not be properly
*copied and *	communicated to the other machines in a network/modem game.
*Combining this class with xTargetClass would *	be more efficient, but Watcom
*doesn't allow class objects that have a constructor to be part of a union [even
**	if the class object has a default constructor!].
*/
class TargetClass : public xTargetClass {
 public:
  TargetClass() : xTargetClass() { Invalidate(); }
  TargetClass(RTTIType rtti, int id) : xTargetClass() {
    Target.Sub.Exponent = rtti;
    Target.Sub.Mantissa = static_cast<unsigned>(id);
  }
  // targets convert from every addressable thing by design.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TargetClass(CELL cell) : xTargetClass() {
    Target.Sub.Exponent = RTTI_CELL;
    Target.Sub.Mantissa = static_cast<unsigned>(cell);
  }
  // targets convert from every addressable thing by design.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TargetClass(TARGET target);
  // targets convert from every addressable thing by design.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TargetClass(const AbstractClass* ptr);
  // targets convert from every addressable thing by design.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TargetClass(const AbstractTypeClass* ptr);
  // targets convert from every addressable thing by design.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TargetClass(const CellClass* ptr);
};

const TechnoTypeClass* As_TechnoType(TARGET target);
COORDINATE As_Movement_Coord(TARGET target);
AircraftClass* As_Aircraft(TARGET target);
AnimClass* As_Animation(TARGET target);
BuildingClass* As_Building(TARGET target);
BulletClass* As_Bullet(TARGET target);
CELL As_Cell(TARGET target);
COORDINATE As_Coord(TARGET target);
InfantryClass* As_Infantry(TARGET target);
TeamClass* As_Team(TARGET target);
TeamTypeClass* As_TeamType(TARGET target);
TechnoClass* As_Techno(TARGET target);
TriggerClass* As_Trigger(TARGET target);
TriggerTypeClass* As_TriggerType(TARGET target);
UnitClass* As_Unit(TARGET target);
VesselClass* As_Vessel(TARGET target);
inline bool Target_Legal(TARGET target) { return target != kTargetNone; };
ObjectClass* As_Object(TARGET target);

#endif  // CNC_RED_ALERT_RA_TARGET_H_
