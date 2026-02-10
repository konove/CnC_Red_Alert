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

/* $Header: /CounterStrike/AIRCRAFT.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : AIRCRAFT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : July 22, 1994 *
 *                                                                                             *
 *                  Last Update : November 28, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_AIRCRAFT_H_
#define CNC_RED_ALERT_RA_AIRCRAFT_H_

#include <cstddef>

#include "ra/bullet.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/facing.h"
#include "ra/fly.h"
#include "ra/foot.h"
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

/*
**	This aircraft class is used for all flying sentient objects. This
*includes fixed wing *	aircraft as well as helicopters. It excludes bullets
*even though some bullets might *	be considered to be "flying" in a loose
*interpretatin of the word.
*/
class AircraftClass : public FootClass, public FlyClass {
 public:
  /*
  **	This is a pointer to the class control structure for the aircraft.
  */
  CCPtr<AircraftTypeClass> Class;

  //-----------------------------------------------------------------------------
  void* operator new(size_t) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void*);
  operator AircraftType() const { return Class->Type; }
  AircraftClass(AircraftType classid, HousesType house);
  AircraftClass(const NoInitClass& x)
      : FootClass(x),
        FlyClass(x),
        Class(x),
        SecondaryFacing(x),
        SightTimer(x) {}
  ~AircraftClass() override;

  static void Init();

  int Mission_Attack() override;
  int Mission_Unload() override;
  int Mission_Hunt() override;
  int Mission_Retreat() override;
  int Mission_Move() override;
  int Mission_Enter() override;
  int Mission_Guard() override;
  int Mission_Guard_Area() override;

  void Assign_Destination(TARGET target) override;
  /*
  **	State machine support routines.
  */
  bool Process_Take_Off();
  bool Process_Landing();
  int Process_Fly_To(bool slowdown, TARGET dest);

  /*
  **	Query functions.
  */
  LayerType In_Which_Layer() const override;
  DirType Turret_Facing() const override { return SecondaryFacing.Current(); }
  int Shape_Number() const;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType facing = FACING_NONE) const override;
  const ObjectTypeClass& Class_Of() const override { return *Class; }
  ActionType What_Action(const ObjectClass* target) const override;
  ActionType What_Action(CELL cell) const override;
  DirType Desired_Load_Dir(ObjectClass* passenger, CELL& moveto) const override;
  int Pip_Count() const override;
  TARGET Good_Fire_Location(TARGET target) const;
  bool Cell_Seems_Ok(CELL cell, bool landing = false) const;
  DirType Pose_Dir() const;
  TARGET Good_LZ() const;
  DirType Fire_Direction() const override;
  FireErrorType Can_Fire(TARGET target, int which) const override;

  /*
  **	Landing zone support functionality.
  */
  void Per_Cell_Process(PCPType why) override;
  bool Is_LZ_Clear(TARGET target) const;
  TARGET New_LZ(TARGET oldlz) const;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Sort_Y() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void Look(bool incremental = false) override;
  void Draw_Rotors(int x, int y, WindowNumberType window) const;
  int Exit_Object(TechnoClass*) override;
  const short* Overlap_List(bool redraw = false) const override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  void Set_Speed(int speed) override;

  /*
  **	User I/O.
  */
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override;
  void Player_Assign_Mission(MissionType mission, TARGET target = TARGET_NONE,
                             TARGET destination = TARGET_NONE) override;
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;

  /*
  **	Combat related.
  */
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source, bool forced = false) override;
  BulletClass* Fire_At(TARGET target, int which) override;

  /*
  **	AI.
  */
  bool Landing_Takeoff_AI();
  bool Edge_Of_World_AI();
  void Movement_AI();
  void Rotation_AI();
  int Paradrop_Cargo();
  void AI() override;
  void Enter_Idle_Mode(bool initial = false) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void Scatter(COORDINATE threat, bool forced = false,
               bool nokidding = false) override;

  // Scenario and debug support.
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static const char* INI_Name() { return "AIRCRAFT"; }
  bool Load(Straw& file);
  bool Save(Pipe& file) const;

  /*
  **	This is the facing used for the body of the aircraft. Typically, this is
  *the same *	as the PrimaryFacing, but in the case of helicopters, it can be
  *different.
  */
  FacingClass SecondaryFacing;

  /*
  **	If this is a passenger carrying aircraft then this flag will be set.
  *This is *	necessary because once the passengers are unloaded, the fact
  *that it was a *	passenger carrier must still be known.
  */
  bool Passenger;

 private:
  /*
  **	Aircraft can be in either state of landing, taking off, or in steady
  *altitude. *	These flags are used to control transition between flying and
  *landing. It is *	necessary to handle the transition in this manner so
  *that it occurs smoothly *	during the graphic processing section.
  */
  unsigned IsLanding : 1;
  unsigned IsTakingOff : 1;

  /*
  **	It is very common for aircraft to be homing in on a target. When this
  *flag is *	true, the aircraft will constantly adjust its facing toward the
  *TarCom. When the *	target is very close (one cell away or less), then this
  *flag is automatically cleared. *	This is because the homing algorithm is
  *designed to get the aircraft to the destination *	but no more. Checking
  *when this flag is cleared is a way of flagging transition into *	a new
  *mode. Example: Transport helicopters go into a hovering into correct position
  **	mode when the target is reached.
  */
  unsigned IsHoming : 1;

  /*
  **	Helicopters that are about to land must hover into a position exactly
  *above the landing *	zone. When this flag is true, the aircraft will be
  *adjusted so that it is exactly over *	the TarCom. The facing of the
  *aircraft is not altered by this movement. The affect *	like the
  *helicopter is hovering and shifting sideways to position over the landing
  **	zone. When the position is over the landing zone, then this flag is set
  *to false.
  */
  unsigned IsHovering : 1;

  /*
  **	This is the jitter tracker to be used when the aircraft is a helicopter
  *and *	is flying. It is most noticeable when the helicopter is
  *hovering.
  */
  unsigned char Jitter;

  /*
  **	This timer controls when the aircraft will reveal the terrain around
  *itself. *	When this timer expires and this aircraft has a sight range,
  *then the *	look around process will occur.
  */
  CDTimerClass<FrameTickSource> SightTimer;

  /*
  **	Most attack aircraft can make several attack runs. This value contains
  *the *	number of attack runs the aircraft has left. When this value
  *reaches *	zero then the aircraft is technically out of ammo.
  */
  char AttacksRemaining;
};

bool Building_Check();

#endif  // CNC_RED_ALERT_RA_AIRCRAFT_H_
