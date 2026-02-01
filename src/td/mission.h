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

/* $Header:   F:\projects\c&c\vcs\code\mission.h_v   2.16   16 Oct 1995 16:45:46
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MISSION.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_MISSION_H_
#define CNC_RED_ALERT_TD_MISSION_H_

#include "td/defines.h"
#include "td/ftimer.h"
#include "td/object.h"
#include "tech/noinit.h"

/****************************************************************************
**	This handles order assignment and tracking. The order is used to guide
**	overall AI processing.
*/
class MissionClass : public ObjectClass {
 public:
  /*
  **	This the tactical strategy to use. It is used by the unit script. This
  **	is a general guide for unit AI processing.
  */
  MissionType Mission;
  MissionType SuspendedMission;

  /*
  **	The order queue is used for orders that should take effect when the
  *vehicle *	has reached the center point of a cell. The queued order number
  *is +1 when stored here *	so that 0 will indicated there is no queued
  *order.
  */
  MissionType MissionQueue;

  char Status;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  MissionClass();
  MissionClass(const NoInitClass& x) : ObjectClass(x), Timer(x) {}
  ~MissionClass() override {}

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  void Debug_Dump(MonoClass* mono) const override;

  MissionType Get_Mission() const override;
  virtual void Assign_Mission(MissionType mission);
  virtual bool Commence();
  void AI() override;

  /*
  **	Support functions.
  */
  virtual int Mission_Sleep();
  virtual int Mission_Ambush();
  virtual int Mission_Attack();
  virtual int Mission_Capture();
  virtual int Mission_Guard();
  virtual int Mission_Guard_Area();
  virtual int Mission_Harvest();
  virtual int Mission_Hunt();
  virtual int Mission_Timed_Hunt();
  virtual int Mission_Move();
  virtual int Mission_Retreat();
  virtual int Mission_Return();
  virtual int Mission_Stop();
  virtual int Mission_Unload();
  virtual int Mission_Enter();
  virtual int Mission_Construction();
  virtual int Mission_Deconstruction();
  virtual int Mission_Repair();
  virtual int Mission_Missile();
  virtual void Set_Mission(MissionType mission);

  static const char* Mission_Name(MissionType order);
  static MissionType Mission_From_Name(const char* name);
  virtual void Override_Mission(MissionType mission, TARGET, TARGET);
  virtual bool Restore_Mission();

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;

 private:
  /*
  **	This the thread processing timer. When this value counts down to zero,
  *then *	more script processing may occur.
  */
  TCountDownTimerClass Timer;

  /*
  **	These are the order names as ASCII strings.
  */
  static const char* Missions[MISSION_COUNT];
};

#endif  // CNC_RED_ALERT_TD_MISSION_H_
