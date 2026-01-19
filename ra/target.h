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

#ifndef TARGET_H
#define TARGET_H

#include "ra/abstract.h"
#include "ra/ccini.h"
#include "ra/defines.h"
#include "ra/object.h"
#include "tech/noinit.h"

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
  return (RTTIType(((TARGET_COMPOSITE&)a).Sub.Exponent));
}

inline unsigned Target_Value(TARGET a) {
  return (((TARGET_COMPOSITE&)a).Sub.Mantissa);
}

inline bool Is_Target_Team(TARGET a) { return (Target_Kind(a) == RTTI_TEAM); }
inline bool Is_Target_TeamType(TARGET a) {
  return (Target_Kind(a) == RTTI_TEAMTYPE);
}
inline bool Is_Target_Trigger(TARGET a) {
  return (Target_Kind(a) == RTTI_TRIGGER);
}
inline bool Is_Target_TriggerType(TARGET a) {
  return (Target_Kind(a) == RTTI_TRIGGERTYPE);
}
inline bool Is_Target_Infantry(TARGET a) {
  return (Target_Kind(a) == RTTI_INFANTRY);
}
inline bool Is_Target_Bullet(TARGET a) {
  return (Target_Kind(a) == RTTI_BULLET);
}
inline bool Is_Target_Terrain(TARGET a) {
  return (Target_Kind(a) == RTTI_TERRAIN);
}
inline bool Is_Target_Cell(TARGET a) { return (Target_Kind(a) == RTTI_CELL); }
inline bool Is_Target_Unit(TARGET a) { return (Target_Kind(a) == RTTI_UNIT); }
inline bool Is_Target_Vessel(TARGET a) {
  return (Target_Kind(a) == RTTI_VESSEL);
}
inline bool Is_Target_Building(TARGET a) {
  return (Target_Kind(a) == RTTI_BUILDING);
}
inline bool Is_Target_Template(TARGET a) {
  return (Target_Kind(a) == RTTI_TEMPLATE);
}
inline bool Is_Target_Aircraft(TARGET a) {
  return (Target_Kind(a) == RTTI_AIRCRAFT);
}
inline bool Is_Target_Animation(TARGET a) {
  return (Target_Kind(a) == RTTI_ANIM);
}
inline bool Is_Target_Object(TARGET a) {
  return (Target_Kind(a) == RTTI_TERRAIN || Target_Kind(a) == RTTI_UNIT ||
          Target_Kind(a) == RTTI_VESSEL || Target_Kind(a) == RTTI_INFANTRY ||
          Target_Kind(a) == RTTI_BUILDING || Target_Kind(a) == RTTI_AIRCRAFT);
}

TARGET As_Target(CELL cell);
TARGET As_Target(COORDINATE coord);
// inline TARGET As_Target(CELL cell) {return (TARGET)(((unsigned)RTTI_CELL <<
// TARGET_MANTISSA) | cell);}

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
  operator RTTIType() const { return (RTTIType(Target.Sub.Exponent)); }

  // comparison operator
  int operator==(xTargetClass& tgt) {
    return (tgt.Target.Target == Target.Target ? 1 : 0);
  }

  // conversion operator to regular TARGET type
  TARGET As_TARGET() const { return (Target.Target); }

  unsigned Value() const { return (Target.Sub.Mantissa); }

  void Invalidate() {
    Target.Sub.Exponent = RTTI_NONE;
    Target.Sub.Mantissa = (1 << TARGET_MANTISSA) - 1;
  }
  bool Is_Valid() const { return (Target.Sub.Exponent != RTTI_NONE); }

  TARGET As_Target() const { return (Target.Target); }
  AbstractTypeClass* As_TypeClass() const;
  AbstractClass* As_Abstract() const;
  TechnoClass* As_Techno() const;
  ObjectClass* As_Object() const;
  CellClass* As_Cell() const;

  /*
  **	Helper routines to combine testing for, and fetching a pointer to, the
  **	type of object indicated.
  */
  TriggerTypeClass* As_TriggerType() const {
    if (*this == RTTI_TRIGGERTYPE) return ((TriggerTypeClass*)As_TypeClass());
    return (nullptr);
  }
  TeamTypeClass* As_TeamType() const {
    if (*this == RTTI_TEAMTYPE) return ((TeamTypeClass*)As_TypeClass());
    return (nullptr);
  }
  TerrainClass* As_Terrain() const {
    if (*this == RTTI_TERRAIN) return ((TerrainClass*)As_Abstract());
    return (nullptr);
  }
  BulletClass* As_Bullet() const {
    if (*this == RTTI_BULLET) return ((BulletClass*)As_Abstract());
    return (nullptr);
  }
  AnimClass* As_Anim() const {
    if (*this == RTTI_ANIM) return ((AnimClass*)As_Abstract());
    return (nullptr);
  }
  TeamClass* As_Team() const {
    if (*this == RTTI_TEAM) return ((TeamClass*)As_Abstract());
    return (nullptr);
  }
  InfantryClass* As_Infantry() const {
    if (*this == RTTI_INFANTRY) return ((InfantryClass*)As_Techno());
    return (nullptr);
  }
  UnitClass* As_Unit() const {
    if (*this == RTTI_UNIT) return ((UnitClass*)As_Techno());
    return (nullptr);
  }
  BuildingClass* As_Building() const {
    if (*this == RTTI_BUILDING) return ((BuildingClass*)As_Techno());
    return (nullptr);
  }
  AircraftClass* As_Aircraft() const {
    if (*this == RTTI_AIRCRAFT) return ((AircraftClass*)As_Techno());
    return (nullptr);
  }
  VesselClass* As_Vessel() const {
    if (*this == RTTI_VESSEL) return ((VesselClass*)As_Techno());
    return (nullptr);
  }
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
  TargetClass() { Invalidate(); }
  TargetClass(NoInitClass const&) {}
  TargetClass(RTTIType rtti, int id) {
    Target.Sub.Exponent = rtti;
    Target.Sub.Mantissa = id;
  }
  TargetClass(CELL cell) {
    Target.Sub.Exponent = RTTI_CELL;
    Target.Sub.Mantissa = cell;
  }
  TargetClass(TARGET target);
  TargetClass(AbstractClass const* ptr);
  TargetClass(AbstractTypeClass const* ptr);
  TargetClass(CellClass const* ptr);
};

TechnoTypeClass const* As_TechnoType(TARGET target);
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
inline bool Target_Legal(TARGET target) { return (target != TARGET_NONE); };
ObjectClass* As_Object(TARGET target);

#endif
