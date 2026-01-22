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

/* $Header:   F:\projects\c&c\vcs\code\template.h_v   2.18   16 Oct 1995
 * 16:45:20   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TEMPLATE.H *
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

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <cstddef>

#include "td/defines.h"
#include "td/globals.h"
#include "td/object.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

/******************************************************************************
**	This class controls the template object. Template objects function
*congruously *	to carpet on a floor. They have no depth, but merely control the
*icon to be rendered *	as the cell's bottom most layer.
*/
class TemplateClass : public ObjectClass {
 public:
  /*-------------------------------------------------------------------
  **	Constructors and destructors.
  */
  static void* operator new(size_t size) throw();
  static void* operator new(size_t, void* ptr) throw() { return ptr; }
  static void operator delete(void* ptr);
  TemplateClass();
  TemplateClass(TemplateType type, CELL pos = -1);
  TemplateClass(NoInitClass const& x) : ObjectClass(x), Class(Class) {}
  ~TemplateClass() override {
    if (GameActive) TemplateClass::Limbo();
  }
  operator TemplateType() const { return Class->Type; }
  RTTIType What_Am_I() const override { return RTTI_TEMPLATE; }

  static void Init();

  /*
  **	Query functions.
  */
  ObjectTypeClass const& Class_Of() const override { return *Class; }
  int Icon_Number(CELL cell);

  /*
  **	Object entry and exit from the game system.
  */

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void Draw_It(int, int, WindowNumberType) override {}
  bool Mark(MarkType mark) override;

  /*
  **	User I/O.
  */

  /*
  **	Combat related.
  */
  TARGET As_Target() const override;

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static char const* INI_Name() { return "TEMPLATE"; }
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

 private:
  /*
  **	This is a pointer to the template object's class.
  */
  TemplateTypeClass const* const Class;

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void* VTable;
};

#endif
