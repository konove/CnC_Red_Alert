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

/* $Header: /CounterStrike/VESSEL.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VESSEL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/13/96 *
 *                                                                                             *
 *                  Last Update : March 13, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_VESSEL_H_
#define CNC_RED_ALERT_RA_VESSEL_H_

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
#include "tech/ftimer.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class VesselClass : public DriveClass {
 public:
  /*
  **	This points to the controling static characteristic data associated with
  **	this vessel.
  */
  CCPtr<VesselTypeClass> Class;

  /*
  ** Has this sea vessel been told to move to a shipyard?  If so, then
  ** when we get there, start the repair process.
  */
  unsigned IsToSelfRepair : 1;

  /*
  ** Is this sea vessel parked next to a shipyard/subpen, and therefore
  ** in the special self-repair mode?
  */
  unsigned IsSelfRepairing : 1;

  /*
  ** If this is an LST, is it time to shut the door?
  */
  Timer<FrameTickSource> DoorShutCountDown;

  /*
  ** If this is a sub, has the sonar pulse worn off, such that we can
  ** re-submerge?
  */
  Timer<FrameTickSource> PulseCountDown;

  VesselClass(VesselType classid, HousesType house);
  VesselClass(const NoInitClass& x)
      : DriveClass(x), Class(x), SecondaryFacing(x) {}
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  operator VesselType() const { return Class->Type; }

  static void Init();

  ~VesselClass() override;
  const ObjectTypeClass& Class_Of() const override;

  virtual MZoneType Zone_Check_Type() const { return MZONE_WATER; }
  int Shape_Number() const;
  void Rotation_AI();
  void Combat_AI();
  bool Edge_Of_World_AI();
  void Repair_AI();
  DirType Turret_Facing() const override {
    if (Class->IsTurretEquipped) {
      return SecondaryFacing.Current();
    }
    return PrimaryFacing.Current();
  }
  bool Start_Driver(COORDINATE& headto) override;
  int Mission_Retreat() override;
  int Mission_Unload() override;
  void LST_Open_Door();
  void LST_Close_Door();
  COORDINATE Fire_Coord(int which) const override;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType from = FACING_NONE) const override;
  void Draw_It(int x, int y, WindowNumberType window) const override;
  const short* Overlap_List(bool redraw = false) const override;
  DirType Desired_Load_Dir(ObjectClass* passenger, CELL& moveto) const override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  void AI() override;
  void Per_Cell_Process(PCPType why) override;
  void Assign_Destination(TARGET target) override;

  virtual ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                                 TechnoClass* source = nullptr,
                                 int forced = false);
  FireErrorType Can_Fire(TARGET target, int which) const override;

  void Enter_Idle_Mode(bool initial = false) override;
  ActionType What_Action(const ObjectClass* object) const override;
  ActionType What_Action(CELL cell) const override;
  void Active_Click_With(ActionType action, CELL cell) override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  TARGET Greatest_Threat(ThreatType threat) override;  // const;
  bool Is_Allowed_To_Recloak() const override;
  BulletClass* Fire_At(TARGET target, int which = 0) override;
  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "SHIPS"; }
  bool Load(Straw& file);
  bool Save(Pipe& file) const;

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

 protected:
  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  FacingClass SecondaryFacing;
};

#endif  // CNC_RED_ALERT_RA_VESSEL_H_
