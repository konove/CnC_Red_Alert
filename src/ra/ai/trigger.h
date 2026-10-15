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

/* $Header: /CounterStrike/TRIGGER.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRIGGER.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/12/94 *
 *                                                                                             *
 *                  Last Update : November 12, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_AI_TRIGGER_H_
#define CNC_RED_ALERT_RA_AI_TRIGGER_H_

#include <cstddef>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/core/object_heap.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_type.h"
#include "ra/core/defines.h"
#include "ra/objects/object.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class TriggerClass {
 public:
  RTTIType RTTI;
  int ID;

  TriggerType* Class = nullptr;

  /*
  **	Record of the "already sprung" flag for the events.
  */
  TriggerEventState Event1;
  TriggerEventState Event2;

  /*
  **	Constructor/Destructor
  */
  explicit TriggerClass(TriggerType* trigtype ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~TriggerClass();
  TriggerClass(const TriggerClass&) = delete;
  TriggerClass& operator=(const TriggerClass&) = delete;
  TriggerClass(TriggerClass&&) = delete;
  TriggerClass& operator=(TriggerClass&&) = delete;

  /*
  **	Initialization: clears all triggers in preparation for new scenario
  */
  static void Init();

  /*
  **	Processing routines
  */
  bool Spring(TriggerEventType event = TriggerEventType::kAny,
              ObjectClass* object = nullptr, ra::Cell cell = ra::Cell{},
              bool forced = false);
  void Detach(Target target, bool all = true) const;

  /*
  **	File I/O routines
  */
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Utility routines
  */
  [[nodiscard]] Target As_Target() const;
  [[nodiscard]] const char* Name() const { return Class->Name(); }

  /*
  **	Overloaded operators
  */
  void* operator new(std::size_t size) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* pointer);

  /*
  **	If this trigger object is active, then this flag will be true. Trigger
  **	objects that are not active are either not yet created or have been
  **	deleted after fulfilling their action.
  */
  // operator new sets this before the constructor runs; the initializer
  // must agree with it.
  bool IsActive : 1 = true;

  /*
  **	This value tells how many objects or cells this trigger is attached
  **	to.  The Read_INI routine for all classes that point to a trigger must
  **	increment this value!
  */
  int AttachCount = 0;

  /*
  **	This value is used for triggers that can only exist in one cell. It is
  **	needed for such triggers that the exact location of the trigger is
  *needed *	during processing but its location cannot be inferred from other
  *data. *	For all other triggers, this value is ignored.
  */
  ra::Cell Cell{};

 private:
  // Shell for ObjectHeap::Load: a fully initialized object with no
  // trigger type, filled in by Serialize().
  TriggerClass();
  friend class game::ObjectHeap<TriggerClass>;
};

TriggerClass* Find_Or_Make(TriggerType* trigtype);

// Removes `trigger` from `triggers`, a list the AI walks with `cursor` as its
// index, and steps `cursor` back when the removal moves the trigger it points
// at down a slot, so the walk goes on with the trigger after it. Does nothing
// if `trigger` is not in the list.
void EraseTrigger(std::vector<TriggerClass*>& triggers, int& cursor,
                  const TriggerClass* trigger);

extern template void TriggerClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TRIGGER_H_
