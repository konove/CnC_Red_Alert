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

/* $Header:   F:\projects\c&c\vcs\code\overlay.h_v   2.16   16 Oct 1995 16:44:50
 * JOE_BOSTIC  $ */
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

#ifndef OVERLAY_H
#define OVERLAY_H

#include <cstddef>

#include "td/defines.h"
#include "td/globals.h"
#include "td/object.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

/******************************************************************************
**	This class controls the overlay object. Overlay objects function
*congruously *	to carpet on a floor. They have no depth, but merely control the
*icon to be rendered *	as the cell's bottom most layer.
*/
class OverlayClass : public ObjectClass {
 public:
  /*-------------------------------------------------------------------
  **	Constructors and destructors.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  OverlayClass();
  OverlayClass(OverlayType type, CELL pos = -1, HousesType = HOUSE_NONE);
  OverlayClass(const NoInitClass& x) : ObjectClass(x), Class(Class) {}
  ~OverlayClass() override {
    if (GameActive) OverlayClass::Limbo();
  }
  operator OverlayType() const { return Class->Type; }
  RTTIType What_Am_I() const override { return RTTI_OVERLAY; }

  static void Init();

  /*
  **	File I/O.
  */
  static void Read_INI(char*);
  static void Write_INI(char*);
  static const char* INI_Name() { return "OVERLAY"; }
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	Virtual support functionality.
  */
  bool Mark(MarkType) override;
  const ObjectTypeClass& Class_Of() const override { return *Class; }
  void Draw_It(int, int, WindowNumberType) override {}

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

 private:
  /*
  **	This is used to control the marking process of the overlay. If this is
  **	set to a valid house number, then the cell that the overlay is marked
  *down *	upon will be flagged as being owned by the specified house.
  */
  static HousesType ToOwn;

  /*
  **	This is a pointer to the overlay object's class.
  */
  const OverlayTypeClass* const Class;

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void* VTable;
};

#endif
