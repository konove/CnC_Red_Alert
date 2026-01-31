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

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <cstddef>

#include "td/bullet.h"
#include "td/defines.h"
#include "td/facing.h"
#include "td/fly.h"
#include "td/foot.h"
#include "td/ftimer.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/techno.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

class AircraftClass : public FootClass, public FlyClass {
 public:
  void* operator new(size_t) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void*);
  operator AircraftType() const { return Class->Type; }
  AircraftClass() : Class(nullptr) {}
  AircraftClass(const NoInitClass& x)
      : FootClass(x),
        FlyClass(x),
        Class(Class),
        SecondaryFacing(x),
        SightTimer(x) {}
  AircraftClass(AircraftType classid, HousesType house);
  ~AircraftClass() override;
  RTTIType What_Am_I() const override { return RTTI_AIRCRAFT; }

  static void Init();
  enum { FLIGHT_LEVEL = 24 };

  int Mission_Attack() override;
  int Mission_Unload() override;
  int Mission_Hunt() override;
  int Mission_Retreat() override;
  int Mission_Move() override;
  int Mission_Enter() override;
  int Mission_Guard() override;
  int Mission_Guard_Area() override;

  // State machine support routines.
  bool Process_Take_Off();
  bool Process_Landing();
  int Process_Fly_To(bool slowdown);

  // Query functions.
  int Threat_Range(int control) const override;
  int Rearm_Delay(bool second) const override;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType facing = FACING_NONE) const override;
  LayerType In_Which_Layer() const override;
  const ObjectTypeClass& Class_Of() const override { return *Class; }
  ActionType What_Action(ObjectClass* target) override;
  ActionType What_Action(CELL cell) const override;
  DirType Desired_Load_Dir(ObjectClass* passenger, CELL& moveto) const override;
  int Pip_Count() const override;
  TARGET Good_Fire_Location(TARGET target) const;
  bool Cell_Seems_Ok(CELL cell, bool landing = false) const;
  DirType Pose_Dir() const;
  TARGET Good_LZ() const;
  DirType Fire_Direction() const override;

  // Landing zone support functionality.
  bool Is_LZ_Clear(TARGET target) const;
  TARGET New_LZ(TARGET oldlz) const;

  // Coordinate inquiry functions. These are used for both display and
  COORDINATE Sort_Y() const override;
  COORDINATE Fire_Coord(int which) const override;
  COORDINATE Target_Coord() const override;

  // Object entry and exit from the game system.
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  int Exit_Object(TechnoClass*) override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
  const short* Overlap_List() const override;
  void Draw_It(int x, int y, WindowNumberType window) override;
  void Set_Speed(int speed) override;

  /*
  **	User I/O.
  */
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override;
  void Player_Assign_Mission(MissionType mission, TARGET target = kTargetNone,
                             TARGET destination = kTargetNone) override;
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;

  // Combat related.
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source) override;
  BulletClass* Fire_At(TARGET target, int which) override;
  TARGET As_Target() const override;

  /*
  **	AI.
  */
  void AI() override;
  void Enter_Idle_Mode(bool initial = false) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void Scatter(COORDINATE threat, bool forced = false) override;

  /*
   **	Scenario and debug support.
   */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static const char* INI_Name() { return "AIRCRAFT"; }
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers() override;
  void Decode_Pointers() override;

  // Debugging support.
  int Validate() const;

  // This is a pointer to the class control structure for the aircraft.
  const AircraftTypeClass* const Class;

  /*
  **	This is the facing used for the body of the aircraft. Typically, this is
  *the same *	as the PrimaryFacing, but in the case of helicopters, it can be
  *different.
  */
  FacingClass SecondaryFacing;

  /*
  **	This is the altitude of the aircraft. It is expressed in pixels that
  **	the shadow is offset to the south. If the altitude reaches zero, then
  **	the aircraft has landed. The altitude for normal aircraft is at
  **	Flight_Level().
  */
  int Altitude;

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
  *and *	is flying. It is most noticable when the helicopter is hovering.
  */
  unsigned char Jitter;

  /*
  **	This timer controls when the aircraft will reveal the terrain around
  *itself. *	When this timer expires and this aircraft has a sight range,
  *then the *	look around process will occur.
  */
  TCountDownTimerClass SightTimer;

  /*
  **	Most attack aircraft can make several attack runs. This value contains
  *the *	number of attack runs the aircraft has left. When this value
  *reaches *	zero then the aircraft is technically out of ammo.
  */
  char AttacksRemaining;

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void* VTable;
};

#endif
