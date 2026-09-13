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

/* $Header: /CounterStrike/TRIGTYPE.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRIGTYPE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/05/96 *
 *                                                                                             *
 *                  Last Update : June 5, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_TRIGTYPE_H_
#define CNC_RED_ALERT_RA_TRIGTYPE_H_

#include <cstddef>
#include <string>

#include "absl/base/attributes.h"
#include "ra/ccini.h"
#include "ra/defines.h"
#include "ra/object.h"
#include "ra/taction.h"
#include "ra/tevent.h"
#include "ra/type.h"

/*
**	There can be multiple trigger events and trigger actions. This
*enumeration is used to *	indicate if there are multiple events/actions
*and what their relationship is.
*/
typedef enum MultiStyleType {
  MULTI_ONLY,   // "Only" main trigger action/event?
  MULTI_AND,    // "And" secondary trigger action/event?
  MULTI_OR,     // "Or" secondary event?
  MULTI_LINKED  // Cause and effect pairs are linked?
} MultiStyleType;

class TriggerTypeClass : public AbstractTypeClass {
 public:
  // operator new sets this before the constructor runs; the initializer
  // must agree with it.
  bool IsActive : 1 = true;

  typedef enum PersistantType {
    VOLATILE = 0,
    SEMIPERSISTANT = 1,
    PERSISTANT = 2
  } PersistantType;

  /*
  **	This flag controls whether the trigger destroys itself after it goes
  **	off.
  **	0 = trigger destroys itself immediately after going off, and removes
  **	    itself from all objects it's attached to
  **	1 = trigger is "Semi-Persistent"; it maintains a count of all objects
  **	    it's attached to, and only actually "springs" after its been
  **		 triggered from all the objects; then, it removes itself.
  **	2 = trigger is Fully Persistent; it just won't go away.
  */
  PersistantType IsPersistant = VOLATILE;

  /*
  **	For house-specific events, this is the house for that event.
  */
  HousesType House = HOUSE_SPAIN;

  /*
  **	Each trigger must have an event which activates it. This is the event
  *that is *	used to activate this trigger.
  */
  TEventClass Event1;
  TEventClass Event2;
  MultiStyleType EventControl = MULTI_ONLY;

  /*
  **	This is the action to perform when the trigger event occurs.
  */
  TActionClass Action1;
  TActionClass Action2;
  MultiStyleType ActionControl = MULTI_ONLY;

  TriggerTypeClass();
  ~TriggerTypeClass() override = default;
  TriggerTypeClass(const TriggerTypeClass&) = delete;
  TriggerTypeClass& operator=(const TriggerTypeClass&) = delete;
  TriggerTypeClass(TriggerTypeClass&&) = delete;
  TriggerTypeClass& operator=(TriggerTypeClass&&) = delete;

  void* operator new(size_t /*unused*/);
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);

  /*
  **	Initialization: clears all trigger types in preparation for new scenario
  */
  static void Init();

  /*
  **	File I/O routines
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  void Fill_In(char* name, char* entry);
  void Build_INI_Entry(std::string& buffer) const;

  static const char* INI_Name() { return "Trigs"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Processing routines
  */
  // the heap owns the new object; many callers create without keeping it.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  TriggerClass* Create_One_Of() const;
  void Destroy_All_Of() const;

  /*
  **	Utility routines
  */
  void Detach(TARGET target, bool all = true);
  [[nodiscard]] AttachType Attaches_To() const;
  static TriggerTypeClass* From_Name(const char* name);
  bool Edit();
  [[nodiscard]] const char* Description() const;
  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator const char*() const { return Description(); }
  void Draw_It(int index, int x, int y, int width, int height, bool selected,
               TextPrintType flags) const;
};

class ArchiveReader;
class ArchiveWriter;
extern template void TriggerTypeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerTypeClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_TRIGTYPE_H_
