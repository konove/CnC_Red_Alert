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

/* $Header:   F:\projects\c&c\vcs\code\unit.h_v   2.19   16 Oct 1995 16:45:56
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : UNIT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 14, 1994 *
 *                                                                                             *
 *                  Last Update : April 14, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_UNIT_H_
#define CNC_RED_ALERT_TD_UNIT_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>
#include <cstdint>

#include "absl/base/attributes.h"
#include "td/defines.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/tarcom.h"
#include "td/techno.h"
#include "td/type.h"

/****************************************************************************
**	For each instance of a unit (vehicle) in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular unit.
*/
class UnitClass final : public TarComClass {
 public:
  // Field-wise state and checked references; load shells have no scenario effects.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	This records the house flag that this object is currently carrying.
  */
  HousesType Flagged = HOUSE_NONE;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  UnitClass() { IsActive = true; }
  UnitClass(UnitType classid, HousesType house);
  ~UnitClass() override;
  UnitClass(const UnitClass&) = delete;
  UnitClass& operator=(const UnitClass&) = delete;
  UnitClass(UnitClass&&) = delete;
  UnitClass& operator=(UnitClass&&) = delete;
  RTTIType What_Am_I() const override;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  bool Goto_Clear_Spot();
  bool Try_To_Deploy();

  bool Tiberium_Check(CELL& center, int x, int y);
  bool Flag_Attach(HousesType house);
  bool Flag_Remove();
  void Find_LZ();
  bool Unload_Hovercraft_Process();
  bool Goto_Tiberium();
  bool Harvesting();
  void APC_Close_Door();
  void APC_Open_Door();

  /*
  **	Query functions.
  */
  bool Can_Player_Move() const override;
  int Pip_Count() const override;
  InfantryType Crew_Type() const override;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Sort_Y() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE /*coord*/ /*unused*/, DirType dir = DIR_N) override;
  bool Limbo() override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  const void* Remap_Table() override;
  void Look(bool incremental = false) override;
  const int16_t* Overlap_List() const override;
  void Draw_It(int x, int y, WindowNumberType window) override;

  /*
  **	User I/O.
  */
  ActionType What_Action(CELL cell) const override;
  ActionType What_Action(ObjectClass* object) override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override;
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;

  /*
  **	Combat related.
  */
  COORDINATE Target_Coord() const override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr) override;
  TARGET As_Target() const override;
  void Stun() override;

  /*
  **	Driver control support functions. These are used to control cell
  **	occupation flags and driver instructions.
  */
  bool Stop_Driver() override;
  bool Start_Driver(COORDINATE& headto) override;

  /*
  **	AI.
  */
  DirType Desired_Load_Dir(ObjectClass* passenger, CELL& moveto) const override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   int32_t& param) override;
  void AI() override;
  int Mission_Attack() override;
  int Mission_Unload() override;
  int Mission_Guard() override;
  int Mission_Harvest() override;
  int Mission_Hunt() override;
  int Mission_Move() override;
  FireErrorType Can_Fire(TARGET /*target*/, int which) const override;

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	Movement and animation.
  */
  void Enter_Idle_Mode(bool initial = false) override;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType facing = FACING_NONE) const override;
  void Per_Cell_Process(bool center) override;
  void Scatter(COORDINATE threat, bool forced = false) override;
  void Exit_Repair();
  //		MoveType Blocking_Object(TechnoClass const *techno, CELL cell)
  // const;

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static const char* INI_Name() { return "UNITS"; }

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

 private:
};

extern template void UnitClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void UnitClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_UNIT_H_
