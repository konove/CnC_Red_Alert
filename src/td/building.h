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

/* $Header:   F:\projects\c&c\vcs\code\building.h_v   2.20   16 Oct 1995
 * 16:47:54   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BUILDING.H *
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

#ifndef CNC_RED_ALERT_TD_BUILDING_H_
#define CNC_RED_ALERT_TD_BUILDING_H_

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>

#include "absl/base/attributes.h"
#include "td/bullet.h"
#include "td/defines.h"
#include "td/factory.h"
#include "td/ftimer.h"
#include "td/house.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/techno.h"
#include "td/type.h"

#define MAX_DOOR_STAGE 18  // # of frames of door opening on weapons factory
#define DOOR_OPEN_STAGE 9  // frame on which the door is entirely open
#define MAX_REPAIR_ANIM_STAGE \
  5  // # of stages of anim for repair center cycling

/****************************************************************************
**	For each instance of a building in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular building.
*/
class BuildingClass final : public TechnoClass {
 public:
  const BuildingTypeClass* Class = nullptr;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator StructType() const { return Class->Type; }

  /*
  **	If this building is in the process of producing something, then this
  **	will point to the factory manager.
  */
  FactoryClass* Factory = nullptr;

  /*
  **	This is the house that originally owned this factory. Objects buildable
  **	by this house type will be produced from this factory regardless of who
  **	the current owner is.
  */
  HousesType ActLike = HOUSE_NONE;

  /*
  **	If the building is at a good point to change orders, then this
  **	flag will be set to true.
  */
  unsigned IsReadyToCommence : 1 = false;

  /*
  **	If this building is currently spending money to repair itself, then
  **	this flag is true. It will automatically be set to false when the
  *building *	has reached full strength, when money is exhausted, or if the
  *player *	specifically stops the repair process.
  */
  unsigned IsRepairing : 1 = false;

  /*
  **	If repair is currently in progress and this flag is true, then a wrench
  *graphic *	will be overlaid on the building to give visual feedback for the
  *repair process.
  */
  unsigned IsWrenchVisible : 1 = false;

  /*
  ** This flag is set when a commando has raided the building and planted
  ** plastic explosives.  When the CommandoCountDown timer expires, the
  ** building takes massive damage.
  */
  unsigned IsGoingToBlow : 1 = false;

  /*
  **	If this building was destroyed by some method that would prevent
  **	survivors, then this flag will be true.
  */
  unsigned IsSurvivorless : 1 = false;

  /*
  **	These state control variables are used by the oblisk for the charging
  **	animation.
  */
  unsigned IsCharging : 1 = false;
  unsigned IsCharged : 1 = false;

  /*
  **	A building that has been captured will not contain the full compliment
  **	of crew. This is true even if it subsiquently gets captured back.
  */
  unsigned IsCaptured : 1 = false;

  /*
  **	Special countdown to destruction value. If the building is destroyed,
  **	it won't actually be removed from the map until this value reaches
  **	zero. This delay is for cosmetic reasons.
  */
  TCountDownTimerClass CountDown;

  /*
  **	This is the current animation processing state that the building is
  **	in.
  */
  BStateType BState = BSTATE_NONE;
  BStateType QueueBState = BSTATE_NONE;

  /*
  ** For multiplayer games, this keeps track of the last house to damage
  ** this building, so if it burns to death or otherwise gradually dies,
  ** proper credit can be given for the kill.
  */
  HousesType WhoLastHurtMe = HOUSE_NONE;

  /*
  **	This is the saboteur responsible for this building's destruction.
  */
  TARGET WhomToRepay = kTargetNone;

  /*
  **	This is a record of the last strength of the building. Every so often,
  **	it will compare this strength to the current strength. If there is a
  **	discrepency, then the owner power is adjusted accordingly.
  */
  int LastStrength = 0;

  /*
  **	This is the countdown timer that regulates placement retry logic
  **	for factory type buildings.
  */
  TCountDownTimerClass PlacementDelay;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  BuildingClass() { IsActive = true; }
  BuildingClass(StructType type, HousesType house);
  ~BuildingClass() override;
  BuildingClass(const BuildingClass&) = delete;
  BuildingClass& operator=(const BuildingClass&) = delete;
  BuildingClass(BuildingClass&&) = delete;
  BuildingClass& operator=(BuildingClass&&) = delete;
  [[nodiscard]] RTTIType What_Am_I() const override { return RTTI_BUILDING; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  TARGET Target_Scan();
  const BuildingTypeClass::AnimControlType* Fetch_Anim_Control() {
    return &Class->Anims[BState];
  }

  /*
  **	Query functions.
  */
  CELL Find_Exit_Cell(const TechnoClass* techno) const override;
  [[nodiscard]] InfantryType Crew_Type() const override;
  [[nodiscard]] int Pip_Count() const override;
  [[nodiscard]] bool Can_Player_Move() const override { return false; }
  ActionType What_Action(ObjectClass* object) override;
  [[nodiscard]] ActionType What_Action(CELL cell) const override;
  [[nodiscard]] bool Can_Demolish() const override;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  [[nodiscard]] int Refund_Amount() const override;
  [[nodiscard]] DirType Fire_Direction() const override;
  [[nodiscard]] int Power_Output() const;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  [[nodiscard]] COORDINATE Docking_Coord() const override;
  [[nodiscard]] COORDINATE Fire_Coord(int which) const override;
  [[nodiscard]] COORDINATE Center_Coord() const override;
  [[nodiscard]] COORDINATE Sort_Y() const override;
  [[nodiscard]] COORDINATE Target_Coord() const override {
    return Center_Coord();
  }

  /*
  **	Object entry and exit from the game system.
  */
  void Detach(TARGET target, bool all) override;
  void Detach_All(bool all = true) override;
  void Grand_Opening(bool captured = false);
  void Update_Buildables();
  [[nodiscard]] MoveType Can_Enter_Cell(
      CELL cell, FacingType /*unused*/ = FACING_NONE) const override;
  bool Unlimbo(COORDINATE /*coord*/ /*unused*/, DirType dir = DIR_N) override;
  bool Limbo() override;
  bool Passes_Proximity_Check(CELL homecell);

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  const void* Remap_Table() override;
  int Exit_Object(TechnoClass* base) override;
  void Draw_It(int x, int y, WindowNumberType window) override;
  bool Mark(MarkType mark) override;
  void Look(bool incremental = false) override;
  void Fire_Out() override;
  void Begin_Mode(BStateType bstate);

  /*
  **	User I/O.
  */
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override;

  /*
  **	Combat related.
  */
  void Death_Announcement(const TechnoClass* source = nullptr) const override;
  [[nodiscard]] FireErrorType Can_Fire(TARGET /*target*/,
                                       int which) const override;
  [[nodiscard]] TARGET Greatest_Threat(ThreatType threat) const override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr) override;
  [[nodiscard]] TARGET As_Target() const override;
  bool Captured(HouseClass* newowner) override;

  /*
  **	AI.
  */
  void Hidden() override;
  bool Revealed(HouseClass* house) override;
  void Repair(int control) override;
  void Sell_Back(int control) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void AI() override;
  void Assign_Target(TARGET target) override;
  bool Toggle_Primary();
  bool Flush_For_Placement(TechnoClass* techno, CELL cell);

  int Mission_Unload() override;
  int Mission_Repair() override;
  int Mission_Attack() override;
  int Mission_Harvest() override;
  int Mission_Guard() override;
  int Mission_Construction() override;
  int Mission_Deconstruction() override;
  int Mission_Missile() override;
  void Enter_Idle_Mode(bool initial = false) override;

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static const char* INI_Name() { return "STRUCTURES"; }
  // Saves building state and checked references without scenario side effects.
  template <class Archive>
  void Serialize(Archive& ar);
  void Update_Specials();

  BulletClass* Fire_At(TARGET target, int which) override;

  /*
  **	Dee-buggin' support.
  */
  // debug self-check; callers run it for its assertions and ignore the count.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

 private:
  void Drop_Debris(TARGET source = kTargetNone);

  static const COORDINATE CenterOffset[BSIZE_COUNT];

};

extern template void BuildingClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void BuildingClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_BUILDING_H_
