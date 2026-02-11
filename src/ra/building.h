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

/* $Header: /CounterStrike/BUILDING.H 1     3/03/97 10:24a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_BUILDING_H_
#define CNC_RED_ALERT_RA_BUILDING_H_

#include <cstddef>

#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/factory.h"
#include "ra/house.h"
#include "ra/jshell.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/radio.h"
#include "ra/techno.h"
#include "ra/type.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

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
  /*
  **	This points to the control data that gives this building its
  *characteristics.
  */
  CCPtr<BuildingTypeClass> Class;

  /*
  **	If this building is in the process of producing something, then this
  **	will point to the factory manager.
  */
  CCPtr<FactoryClass> Factory;

  /*
  **	This is the house that originally owned this factory. Objects buildable
  **	by this house type will be produced from this factory regardless of who
  **	the current owner is.
  */
  HousesType ActLike;

  /*
  **	This building should be rebuilt if it is destroyed. This is in spite
  **	of the condition of the prebuilt base list.
  */
  unsigned IsToRebuild : 1;

  /*
  **	Is the building allowed to repair itself?
  */
  unsigned IsToRepair : 1;

  /*
  **	If the computer owns this building, then it is allowed to sell it if
  **	the situation warrants it. In the other case, it cannot sell the
  **	building regardless of conditions.
  */
  unsigned IsAllowedToSell : 1;

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
  **	These state control variables are used by the obelisk for the charging
  **	animation.
  */
  unsigned IsCharging : 1;
  unsigned IsCharged : 1;

  /*
  **	A building that has been captured will not contain the full compliment
  **	of crew. This is true even if it subsequently gets captured back.
  */
  unsigned IsCaptured : 1;

  /*
  ** Used by the gap generator to decide if it should jam or unjam
  */
  unsigned IsJamming : 1;

  /*
  ** Used by radar facilities to know if they're being jammed by a mobile
  ** radar jammer
  */
  unsigned IsJammed : 1;

  /*
  ** Used only by advanced tech center, this keeps track of whether the
  ** GPS satellite has been fired or not.
  */
  unsigned HasFired : 1;

  /*
  **	If Grand_Opening was already called for this building, then this
  **	flag will be true. By utilizing this flag, multiple inadvertant
  **	calls to Grand_Opening won't cause problems.
  */
  unsigned HasOpened : 1;

  /*
  **	Special countdown to destruction value. If the building is destroyed,
  **	it won't actually be removed from the map until this value reaches
  **	zero. This delay is for cosmetic reasons.
  */
  Timer<FrameTickSource> CountDown;

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
  **	discrepancy, then the owner power is adjusted accordingly.
  */
  int LastStrength;

  /*
  ** This is a target id of an animation we're keeping track of.  Examples
  ** of this usage are the advanced tech center, which needs to know
  ** when the sputdoor animation has reached a certain stage.
  */
  TARGET AnimToTrack;

  /*
  **	This is the countdown timer that regulates placement retry logic
  **	for factory type buildings.
  */
  Timer<FrameTickSource> PlacementDelay;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  BuildingClass(StructType type, HousesType house);
  BuildingClass(const NoInitClass& x)
      : TechnoClass(x), Class(x), Factory(x), CountDown(x), PlacementDelay(x) {}
  ~BuildingClass() override;
  operator StructType() const { return Class->Type; }

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
  int Value() const override;
  const void* Get_Image_Data() const override;
  int How_Many_Survivors() const override;
  DirType Turret_Facing() const override;
  CELL Find_Exit_Cell(const TechnoClass* techno) const override;
  InfantryType Crew_Type() const override;
  int Pip_Count() const override;
  bool Can_Player_Move() const override;
  ActionType What_Action(const ObjectClass* target) const override;
  ActionType What_Action(CELL cell) const override;
  bool Can_Demolish() const override;
  const ObjectTypeClass& Class_Of() const override { return *Class; }
  DirType Fire_Direction() const override;
  const short* Overlap_List(bool redraw = false) const override;
  int Shape_Number() const;
  int Power_Output() const;
  CELL Check_Point(CheckPointType cp) const;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Target_Coord() const override;
  COORDINATE Docking_Coord() const override;
  COORDINATE Center_Coord() const override;
  COORDINATE Sort_Y() const override;
  COORDINATE Exit_Coord() const override;

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

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  virtual const void* Remap_Table();
  int Exit_Object(TechnoClass* base) override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
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
  FireErrorType Can_Fire(TARGET, int which) const override;
  TARGET Greatest_Threat(ThreatType threat) override;  // const;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr,
                         bool forced = false) override;
  bool Captured(HouseClass* newowner) override;
  void Update_Radar_Spied();

  /*
  **	AI.
  */
  void Charging_AI();
  void Rotation_AI();
  void Factory_AI();
  void Repair_AI();
  void Animation_AI();
  bool Revealed(HouseClass* house) override;
  void Repair(int control) override;
  void Sell_Back(int control) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void AI() override;
  void Assign_Target(TARGET target) override;
  virtual bool Toggle_Primary();
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
  void Remove_Gap_Effect();

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "STRUCTURES"; }
  bool Load(Straw& file);
  bool Save(Pipe& file) const;

 private:
  void Drop_Debris(TARGET source = TARGET_NONE);

  static const COORDINATE CenterOffset[BSIZE_COUNT];
};

#endif  // CNC_RED_ALERT_RA_BUILDING_H_
