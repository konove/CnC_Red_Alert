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

/* $Header: /CounterStrike/OVERLAY.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : OVERLAY.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 17, 1994 *
 *                                                                                             *
 *                  Last Update : May 17, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_OVERLAY_H_
#define CNC_RED_ALERT_RA_OVERLAY_H_

#include <cstddef>

#include "absl/base/attributes.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/object.h"
#include "ra/type.h"

/******************************************************************************
**	This class controls the overlay object. Overlay objects function
*congruously *	to carpet on a floor. They have no depth, but merely control the
*icon to be rendered *	as the cell's bottom most layer.
*/
class OverlayClass : public ObjectClass {
 public:
  /*
  **	This is a pointer to the overlay object's class.
  */
  CCPtr<OverlayTypeClass> Class;

  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  OverlayClass() = default;
  friend class TFixedIHeapClass<OverlayClass>;

  /*-------------------------------------------------------------------
  **	Constructors and destructors.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  explicit OverlayClass(OverlayType type, CELL pos = -1,
                        HousesType /*house*/ = HOUSE_NONE);
  ~OverlayClass() override {
    if (GameActive) {
      OverlayClass::Limbo();
    }
    Class = nullptr;
  }
  OverlayClass(const OverlayClass&) = delete;
  OverlayClass& operator=(const OverlayClass&) = delete;
  OverlayClass(OverlayClass&&) = delete;
  OverlayClass& operator=(OverlayClass&&) = delete;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator OverlayType() const { return Class->Type; }

  static void Init();

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "OVERLAY"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Virtual support functionality.
  */
  bool Mark(MarkType /*mark*/ /*unused*/) override;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  void Draw_It(int /*x*/, int /*y*/,
               WindowNumberType /*unused*/) const override {}

 private:
  /*
  **	This is used to control the marking process of the overlay. If this is
  **	set to a valid house number, then the cell that the overlay is marked
  *down *	upon will be flagged as being owned by the specified house.
  */
  static HousesType ToOwn;
};

class ArchiveReader;
class ArchiveWriter;
extern template void OverlayClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void OverlayClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_OVERLAY_H_
