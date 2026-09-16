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

/* $Header:   F:\projects\c&c\vcs\code\factory.h_v   2.17   16 Oct 1995 16:45:44
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FACTORY.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/26/94 *
 *                                                                                             *
 *                  Last Update : December 26, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_FACTORY_H_
#define CNC_RED_ALERT_TD_FACTORY_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>

#include "absl/base/attributes.h"
#include "td/house.h"
#include "td/stage.h"
#include "td/techno.h"
#include "td/type.h"
#include "tech/file.h"

class FactoryClass : StageClass {
 public:
  FactoryClass() = default;
  ~FactoryClass() override;
  FactoryClass(const FactoryClass&) = delete;
  FactoryClass& operator=(const FactoryClass&) = delete;
  FactoryClass(FactoryClass&&) = delete;
  FactoryClass& operator=(FactoryClass&&) = delete;
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);

  static void Init();

  /*
  **	File I/O.
  */
  // Field-wise saved-game support, defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  bool Abandon();
  bool Completed();
  bool Has_Changed();
  bool Has_Completed();
  [[nodiscard]] bool Is_Building() const { return Fetch_Rate() != 0; }
  bool Set(const TechnoTypeClass& object, HouseClass& house);
  bool Set(const int& type, HouseClass& house);
  bool Start();
  bool Suspend();
  int Completion();
  [[nodiscard]] TechnoClass* Get_Object() const;
  [[nodiscard]] int Get_Special_Item() const;
  void AI();
  void Set(TechnoClass& object);
  HouseClass* Get_House() { return HouseClass::As_Pointer(House); }

  /*
  **	Dee-buggin' support.
  */
  // debug self-check; callers run it for its assertions and ignore the count.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

  /*
  **	This flag is used to maintain the pool of factory class objects. If the
  *object has *	been allocated, then this flag is true. Otherwise, the object is
  *free to be *	allocated.
  */
  bool IsActive : 1 = true;

 protected:
  // Number of steps to break production down into.
  static constexpr int kStepCount = 108;

  int Cost_Per_Tick();

 private:
  /*
  **	If production is temporarily suspended, then this flag will be true. A
  *factory *	is suspended when it is first created, when production has
  *completed, and when *	explicitly instructed to Suspend() production.
  *Suspended production is not *	abandoned. It may be resumed with a call
  *to Start().
  */
  bool IsSuspended : 1 = false;

  /*
  **	If the AI process detected that the production process has advanced far
  *enough *	that a change in the building animation would occur, this flag
  *will be true. *	Examination of this flag (through the Has_Chaged
  *function) allows intelligent *	updating of any production graphic.
  */
  bool IsDifferent : 1 = false;

  /*
  **	This records the balance due on the current production item. This value
  *will *	be reduced as production proceeds. It will reach zero the moment
  *production has *	finished. Using this method ensures that the total
  *production cost will be EXACT *	regardless of the number of installment
  *payments that are made.
  */
  int Balance = 0;
  int OriginalBalance = 0;

  /*
  **	This is the object that is being produced. It is held in a state of
  *limbo while *	undergoing production. Since the object is created at
  *the time production is *	started, it is always available when production
  *completes.
  */
  TechnoClass* Object = nullptr;

  /*
  **	If the factory is not producing an object and is instead producing
  ** a special item, then special item will be set.
  */
  SpecialWeaponType SpecialItem = SPC_NONE;

  /*
  ** The factory has to be doing production for one house or another.
  ** The house ID records whichever house it is being done
  ** for.
  */
  HousesType House = HOUSE_NONE;
};

extern template void FactoryClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void FactoryClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_FACTORY_H_
