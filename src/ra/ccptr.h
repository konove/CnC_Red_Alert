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

/* $Header: /CounterStrike/CCPTR.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCPTR.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/07/96 *
 *                                                                                             *
 *                  Last Update : June 7, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/
#ifndef CNC_RED_ALERT_RA_CCPTR_H_
#define CNC_RED_ALERT_RA_CCPTR_H_

#include <cassert>

#include "ra/heap.h"

// The CCPtr class is designed for a specific purpose. It functions like a
// pointer except that it requires no fixups for saving and loading. If pointer
// fixups are not an issue, than using regular pointers would be more efficient.
template <class T>
class CCPtr {
 public:
  CCPtr() : ID(-1) {}
  // a CCPtr stands in for the raw object pointer.
  // NOLINTNEXTLINE(*-explicit-constructor)
  CCPtr(T* ptr);

  // a CCPtr stands in for the raw object pointer.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator T*() const {
    if (ID == -1) {
      return nullptr;
    }
    assert(Heap != nullptr && ID < Heap->Length());
    return static_cast<T*>((*Heap)[ID]);
  }
  T& operator*() const {
    assert(Heap != nullptr && ID < Heap->Length());
    return *static_cast<T*>((*Heap)[ID]);
  }
  T* operator->() const {
    if (ID == -1) {
      return nullptr;
    }
    assert(Heap != nullptr && ID < Heap->Length());
    return static_cast<T*>((*Heap)[ID]);
  }

  bool Is_Valid() const { return ID != -1; }

  bool operator==(const CCPtr& rvalue) const { return ID == rvalue.ID; }
  bool operator!=(const CCPtr& rvalue) const { return ID != rvalue.ID; }
  bool operator>(const CCPtr& rvalue) const;
  bool operator<=(const CCPtr& rvalue) const { return rvalue > *this; }
  bool operator<(const CCPtr& rvalue) const {
    return *this != rvalue && rvalue > *this;
  }
  bool operator>=(const CCPtr& rvalue) const {
    return *this == rvalue || rvalue > *this;
  }

  long Raw() const { return ID; }
  void Set_Raw(const long value) { ID = static_cast<int>(value); }

  // Saved-game support. The ID is the whole state; a loaded ID outside the
  // heap is a corrupt save, not a programmer error, so it is reported through
  // the reader rather than checked.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(ID);
    if constexpr (Archive::kIsReading) {
      if (ID != -1 && (Heap == nullptr || ID < 0 || ID >= Heap->Length())) {
        ar.Fail("CCPtr ID outside its heap");
        ID = -1;
      }
    }
  }

 private:
  static FixedIHeapClass* Heap;

  /*
  **	This is the ID number of the object it refers to. By using an ID number,
  *this class can *	be saved and loaded without pointer fixups.
  */
  int ID;
};

/*
**	These template helper functions tell the compiler what to do in the
**	ambiguous case of a CCPtr on one side and a regular type pointer on the
**	other side. In such a case the compiler could create a temp CCPtr object
**	OR call the conversion operator on the existing CCPtr object. Either way
**	is technically valid, but the compiler doesn't know which is better so
*it *	generates an error. These routines force the conversion operator rather
*than *	creating a temporary object. This presumes that the conversion operator
*is *	cheaper than constructing a temporary and that cheaper solutions are
*desirable.
*/
template <class T>
bool operator==(const CCPtr<T>& lvalue, T* rvalue) {
  return static_cast<T*>(lvalue) == rvalue;
}

template <class T>
bool operator==(T* lvalue, const CCPtr<T>& rvalue) {
  return lvalue == static_cast<T*>(rvalue);
}

/*
**	The heap pointers are defined in globals.cc, one per game object type
* that a *	CCPtr may refer to. Declaring the specializations here lets
* every translation *	unit that dereferences a CCPtr see that a definition
* exists.
*/
class AircraftClass;
class AnimClass;
class BuildingClass;
class BulletClass;
class FactoryClass;
class HouseClass;
class InfantryClass;
class OverlayClass;
class SmudgeClass;
class TeamClass;
class TeamTypeClass;
class TemplateClass;
class TerrainClass;
class TriggerClass;
class TriggerTypeClass;
class HouseTypeClass;
class BuildingTypeClass;
class AircraftTypeClass;
class InfantryTypeClass;
class BulletTypeClass;
class AnimTypeClass;
class UnitTypeClass;
class VesselTypeClass;
class TemplateTypeClass;
class TerrainTypeClass;
class OverlayTypeClass;
class SmudgeTypeClass;

template <>
FixedIHeapClass* CCPtr<AircraftClass>::Heap;
template <>
FixedIHeapClass* CCPtr<AnimClass>::Heap;
template <>
FixedIHeapClass* CCPtr<BuildingClass>::Heap;
template <>
FixedIHeapClass* CCPtr<BulletClass>::Heap;
template <>
FixedIHeapClass* CCPtr<FactoryClass>::Heap;
template <>
FixedIHeapClass* CCPtr<HouseClass>::Heap;
template <>
FixedIHeapClass* CCPtr<InfantryClass>::Heap;
template <>
FixedIHeapClass* CCPtr<OverlayClass>::Heap;
template <>
FixedIHeapClass* CCPtr<SmudgeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TeamClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TeamTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TemplateClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TerrainClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TriggerClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TriggerTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<HouseTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<BuildingTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<AircraftTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<InfantryTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<BulletTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<AnimTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<UnitTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<VesselTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TemplateTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<TerrainTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<OverlayTypeClass>::Heap;
template <>
FixedIHeapClass* CCPtr<SmudgeTypeClass>::Heap;

// ccptr.cc instantiates CCPtr for these types; the declarations stop every
// other translation unit from instantiating members it has no definition for.
extern template class CCPtr<AircraftClass>;
extern template class CCPtr<AnimClass>;
extern template class CCPtr<BuildingClass>;
extern template class CCPtr<BulletClass>;
extern template class CCPtr<FactoryClass>;
extern template class CCPtr<HouseClass>;
extern template class CCPtr<InfantryClass>;
extern template class CCPtr<OverlayClass>;
extern template class CCPtr<SmudgeClass>;
extern template class CCPtr<TeamClass>;
extern template class CCPtr<TeamTypeClass>;
extern template class CCPtr<TemplateClass>;
extern template class CCPtr<TerrainClass>;
extern template class CCPtr<TriggerClass>;
extern template class CCPtr<TriggerTypeClass>;
extern template class CCPtr<HouseTypeClass>;
extern template class CCPtr<BuildingTypeClass>;
extern template class CCPtr<AircraftTypeClass>;
extern template class CCPtr<InfantryTypeClass>;
extern template class CCPtr<BulletTypeClass>;
extern template class CCPtr<AnimTypeClass>;
extern template class CCPtr<UnitTypeClass>;
extern template class CCPtr<VesselTypeClass>;
extern template class CCPtr<TemplateTypeClass>;
extern template class CCPtr<TerrainTypeClass>;
extern template class CCPtr<OverlayTypeClass>;
extern template class CCPtr<SmudgeTypeClass>;

#endif  // CNC_RED_ALERT_RA_CCPTR_H_
