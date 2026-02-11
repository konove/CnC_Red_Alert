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

/* $Header: /CounterStrike/UNIT.H 1     3/03/97 10:26a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_UNIT_H_
#define CNC_RED_ALERT_RA_UNIT_H_

#include <cstddef>

#include "ra/bullet.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/drive.h"
#include "ra/face.h"
#include "ra/facing.h"
#include "ra/jshell.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/radio.h"
#include "ra/techno.h"
#include "ra/type.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

/****************************************************************************
**	For each instance of a unit (vehicle) in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular unit.
*/
class UnitClass : public DriveClass {
 public:
  /*
  **	This points to the static control data that gives 'this' unit its
  *characteristics.
  */
  CCPtr<UnitTypeClass> Class;

  /*
  **	This records the house flag that this object is currently carrying.
  */
  HousesType Flagged;

  /*
  ** This flag is used for when the harvester dumps ore, to track its
  ** special animation.
  */
  unsigned IsDumping : 1;

  /*
  ** This is a count of the # of loads of the various minerals that the
  ** unit has harvested.
  */
  unsigned Gold : 5;
  unsigned Gems : 5;

  /*
  ** This flag tells a unit that, if after reaching its destination, it
  ** should scatter away.  It's meant to help a LST unload its units by
  ** having its previous passengers get out of the way.
  */
  unsigned IsToScatter : 1;

  /*
  **	This records the number of "loads" of Tiberium the unit is carrying.
  *Only *	harvesters use this field.
  */
  int Tiberium;

  /*
  ** This is the area where a mobile gap generator stores the previously-held
  ** shroud values for the cells surrounding itself.
  */
  unsigned long ShroudBits;

  /*
  ** This is the center coordinate for the mobile gap generator, as to
  ** what cells should be revealed (according to ShroudBits)
  */
  CELL ShroudCenter;

  /*
  **	This is the timer that controls the reload rate. The MSAM rocket
  **	launcher is the primary user of this.
  */
  Timer<FrameTickSource> Reload;

  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  FacingClass SecondaryFacing;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  UnitClass(UnitType classid, HousesType house);
  UnitClass(const NoInitClass& x)
      : DriveClass(x), Class(x), Reload(x), SecondaryFacing(x) {}
  operator UnitType() const { return Class->Type; }
  ~UnitClass() override;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  const ObjectTypeClass& Class_Of() const override;
  static void Init();

  bool Goto_Clear_Spot();
  bool Try_To_Deploy();
  void Scatter(COORDINATE threat, bool forced = false,
               bool nokidding = false) override;

  bool Tiberium_Check(CELL& center, int x, int y);
  bool Flag_Attach(HousesType house);
  bool Flag_Remove();
  bool Goto_Tiberium(int radius);
  bool Harvesting();
  void APC_Close_Door();
  void APC_Open_Door();

  /*
  **	Query functions.
  */
  bool Should_Crush_It(const TechnoClass* it) const;
  int Credit_Load() const;
  DirType Turret_Facing() const override {
    if (Class->IsTurretEquipped) {
      return SecondaryFacing.Current();
    }
    return PrimaryFacing.Current();
  }
  int Shape_Number() const;
  int Pip_Count() const override;
  InfantryType Crew_Type() const override;
  DirType Fire_Direction() const override;
  bool Ok_To_Move(DirType facing) const override;
  FireErrorType Can_Fire(TARGET target, int which) const override;
  fixed Tiberium_Load() const override;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Sort_Y() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Limbo() override;
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  const short* Overlap_List(bool redraw = false) const override;
  void Draw_It(int x, int y, WindowNumberType window) const override;

  /*
  **	User I/O.
  */
  ActionType What_Action(CELL cell) const override;
  ActionType What_Action(const ObjectClass* object) const override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override;

  /*
  **	Combat related.
  */
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr,
                         bool forced = false) override;
  BulletClass* Fire_At(TARGET target, int which = 0) override;

  /*
  **	Driver control support functions. These are used to control cell
  **	occupation flags and driver instructions.
  */
  bool Start_Driver(COORDINATE& coord) override;

  /*
  **	AI.
  */
  TARGET Greatest_Threat(ThreatType threat) override;  // const;
  DirType Desired_Load_Dir(ObjectClass* passenger, CELL& moveto) const override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void AI() override;
  int Mission_Guard_Area() override;
  int Mission_Unload() override;
  int Mission_Guard() override;
  int Mission_Harvest() override;
  int Mission_Hunt() override;
  int Mission_Repair() override;
  int Mission_Move() override;
  void Rotation_AI();
  void Firing_AI();
  void Reload_AI();
  bool Edge_Of_World_AI();

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	Movement and animation.
  */
  void Assign_Destination(TARGET target) override;
  void Overrun_Square(CELL cell, bool threaten = true) override;
  void Approach_Target() override;
  int Offload_Tiberium_Bail() override;
  void Enter_Idle_Mode(bool initial = false) override;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType facing = FACING_NONE) const override;
  void Per_Cell_Process(PCPType why) override;
  void Exit_Repair();
  void Shroud_Regen();

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "UNITS"; }
  bool Load(Straw& file);
  bool Save(Pipe& file) const;
};

#endif  // CNC_RED_ALERT_RA_UNIT_H_
