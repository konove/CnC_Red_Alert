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

/* $Header:   F:\projects\c&c\vcs\code\smudge.h_v   2.16   16 Oct 1995 16:47:32
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SMUDGE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 9, 1994 *
 *                                                                                             *
 *                  Last Update : August 9, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_SMUDGE_H_
#define CNC_RED_ALERT_TD_SMUDGE_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>

#include "td/defines.h"
#include "td/globals.h"
#include "td/object.h"
#include "td/type.h"

/******************************************************************************
**	This is the transitory form for smudges. They exist as independent
*objects *	only in the transition stage from creation to placement upon the
*map. Once *	they are placed on the map, they merely become 'smudges' in the
*cell data. This *	object is then destroyed.
*/
class SmudgeClass : public ObjectClass {
 public:
  /*-------------------------------------------------------------------
  **	Constructors and destructors.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  explicit SmudgeClass(SmudgeType type, COORDINATE pos = -1,
              HousesType house = HOUSE_NONE);
  SmudgeClass() { IsActive = true; }
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator SmudgeType() const { return Class->Type; }
  ~SmudgeClass() override {
    if (GameActive) {
      SmudgeClass::Limbo();
    }
  }
  SmudgeClass(const SmudgeClass&) = delete;
  SmudgeClass& operator=(const SmudgeClass&) = delete;
  SmudgeClass(SmudgeClass&&) = delete;
  SmudgeClass& operator=(SmudgeClass&&) = delete;
  [[nodiscard]] RTTIType What_Am_I() const override { return RTTI_SMUDGE; }

  static void Init();

  /*
  **	File I/O.
  */
  static void Read_INI(char* /*buffer*/);
  static void Write_INI(char* /*buffer*/);
  static const char* INI_Name() { return "SMUDGE"; }
  // Field-wise saved-game support, defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  bool Mark(MarkType /*mark*/ /*unused*/) override;
  void Draw_It(int /*x*/, int /*y*/, WindowNumberType /*unused*/) override {}

  void Disown(CELL cell);

  /*
  **	Dee-buggin' support.
  */
  // debug self-check; callers run it for its assertions and ignore the count.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

 private:
  static HousesType ToOwn;

  /*
  **	This is a pointer to the template object's class.
  */
  const SmudgeTypeClass* Class = nullptr;
};

extern template void SmudgeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void SmudgeClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_SMUDGE_H_
