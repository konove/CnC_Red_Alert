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

#ifndef BUILDING_H
#define BUILDING_H

#include <cstddef>

#include "td/bullet.h"
#include "td/defines.h"
#include "td/factory.h"
#include "td/ftimer.h"
#include "td/house.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/techno.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

#define MAX_DOOR_STAGE 18  // # of frames of door opening on weapons factory
#define DOOR_OPEN_STAGE 9  // frame on which the door is entirely open
#define MAX_REPAIR_ANIM_STAGE \
  5  // # of stages of anim for repair center cycling

/****************************************************************************
**	For each instance of a building in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular building.
*/
class BuildingClass : public TechnoClass {
 public:
  BuildingTypeClass const *const Class;
  operator StructType() const { return Class->Type; }

  /*
  **	If this building is in the process of producing something, then this
  **	will point to the factory manager.
  */
  FactoryClass *Factory;

  /*
  **	This is the house that originally owned this factory. Objects buildable
  **	by this house type will be produced from this factory regardless of who
  **	the current owner is.
  */
  HousesType ActLike;

  /*
  **	If the building is at a good point to change orders, then this
  **	flag will be set to true.
  */
  unsigned IsReadyToCommence : 1;

  /*
  **	If this building is currently spending money to repair itself, then
  **	this flag is true. It will automatically be set to false when the
  *building *	has reached full strength, when money is exhausted, or if the
  *player *	specifically stops the repair process.
  */
  unsigned IsRepairing : 1;

  /*
  **	If repair is currently in progress and this flag is true, then a wrench
  *graphic *	will be overlaid on the building to give visual feedback for the
  *repair process.
  */
  unsigned IsWrenchVisible : 1;

  /*
  ** This flag is set when a commando has raided the building and planted
  ** plastic explosives.  When the CommandoCountDown timer expires, the
  ** building takes massive damage.
  */
  unsigned IsGoingToBlow : 1;

  /*
  **	If this building was destroyed by some method that would prevent
  **	survivors, then this flag will be true.
  */
  unsigned IsSurvivorless : 1;

  /*
  **	These state control variables are used by the oblisk for the charging
  **	animation.
  */
  unsigned IsCharging : 1;
  unsigned IsCharged : 1;

  /*
  **	A building that has been captured will not contain the full compliment
  **	of crew. This is true even if it subsiquently gets captured back.
  */
  unsigned IsCaptured : 1;

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
  BStateType BState;
  BStateType QueueBState;

  /*
  ** For multiplayer games, this keeps track of the last house to damage
  ** this building, so if it burns to death or otherwise gradually dies,
  ** proper credit can be given for the kill.
  */
  HousesType WhoLastHurtMe;

  /*
  **	This is the saboteur responsible for this building's destruction.
  */
  TARGET WhomToRepay;

  /*
  **	This is a record of the last strength of the building. Every so often,
  **	it will compare this strength to the current strength. If there is a
  **	discrepency, then the owner power is adjusted accordingly.
  */
  int LastStrength;

  /*
  **	This is the countdown timer that regulates placement retry logic
  **	for factory type buildings.
  */
  TCountDownTimerClass PlacementDelay;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void *operator new(size_t size) throw();
  void *operator new(size_t, void *ptr) throw() { return ptr; }
  void operator delete(void *ptr);
  BuildingClass() : Class(nullptr) {}
  BuildingClass(StructType type, HousesType house);
  BuildingClass(NoInitClass const &x)
      : TechnoClass(x), Class(Class), CountDown(x), PlacementDelay(x) {}
  ~BuildingClass() override;
  RTTIType What_Am_I() const override { return RTTI_BUILDING; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  TARGET Target_Scan();
  BuildingTypeClass::AnimControlType const *Fetch_Anim_Control() {
    return &Class->Anims[BState];
  }

  /*
  **	Query functions.
  */
  CELL Find_Exit_Cell(TechnoClass const *techno) const override;
  InfantryType Crew_Type() const override;
  int Pip_Count() const override;
  bool Can_Player_Move() const override { return false; }
  ActionType What_Action(ObjectClass *target) override;
  ActionType What_Action(CELL cell) const override;
  bool Can_Demolish() const override;
  ObjectTypeClass const &Class_Of() const override { return *Class; }
  int Refund_Amount() const override;
  DirType Fire_Direction() const override;
  int Power_Output() const;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Docking_Coord() const override;
  COORDINATE Fire_Coord(int which) const override;
  COORDINATE Center_Coord() const override;
  COORDINATE Sort_Y() const override;
  COORDINATE Target_Coord() const override { return Center_Coord(); }

  /*
  **	Object entry and exit from the game system.
  */
  void Detach(TARGET target, bool all) override;
  void Detach_All(bool all = true) override;
  virtual void Grand_Opening(bool captured = false);
  virtual void Update_Buildables();
  MoveType Can_Enter_Cell(CELL cell, FacingType = FACING_NONE) const override;
  bool Unlimbo(COORDINATE, DirType dir = DIR_N) override;
  bool Limbo() override;
  bool Passes_Proximity_Check(CELL homecell);

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void const *Remap_Table() override;
  int Exit_Object(TechnoClass *base) override;
  void Draw_It(int x, int y, WindowNumberType window) override;
  bool Mark(MarkType mark) override;
  void Look(bool incremental = false) override;
  void Fire_Out() override;
  void Begin_Mode(BStateType bstate);

  /*
  **	User I/O.
  */
  void Active_Click_With(ActionType action, ObjectClass *object) override;
  void Active_Click_With(ActionType action, CELL cell) override;

  /*
  **	Combat related.
  */
  void Death_Announcement(TechnoClass const *source = nullptr) const override;
  FireErrorType Can_Fire(TARGET, int which) const override;
  TARGET Greatest_Threat(ThreatType threat) const override;
  ResultType Take_Damage(int &damage, int distance, WarheadType warhead,
                         TechnoClass *source = nullptr) override;
  TARGET As_Target() const override;
  bool Captured(HouseClass *newowner) override;

  /*
  **	AI.
  */
  void Hidden() override;
  bool Revealed(HouseClass *house) override;
  void Repair(int control) override;
  void Sell_Back(int control) override;
  RadioMessageType Receive_Message(RadioClass *from, RadioMessageType message,
                                   long &param) override;
  void AI() override;
  void Assign_Target(TARGET target) override;
  virtual bool Toggle_Primary();
  bool Flush_For_Placement(TechnoClass *techno, CELL cell);

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
#ifdef CHEAT_KEYS
  virtual void Debug_Dump(MonoClass *mono) const;
#endif

  /*
  **	File I/O.
  */
  static void Read_INI(char *buffer);
  static void Write_INI(char *buffer);
  static char const *INI_Name() { return "STRUCTURES"; }
  bool Load(FileClass &file);
  bool Save(FileClass &file);
  void Code_Pointers() override;
  void Decode_Pointers() override;
  void Update_Specials();

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

 private:
  void Drop_Debris(TARGET source = kTargetNone);
  BulletClass *Fire_At(TARGET target, int which) override;

  static COORDINATE const CenterOffset[BSIZE_COUNT];

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void *VTable;
};

#endif
