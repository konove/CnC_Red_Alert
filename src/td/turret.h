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

/* $Header:   F:\projects\c&c\vcs\code\turret.h_v   2.17   16 Oct 1995 16:48:00
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TURRET.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 25, 1994 *
 *                                                                                             *
 *                  Last Update : April 25, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_TURRET_H_
#define CNC_RED_ALERT_TD_TURRET_H_

#include "td/bullet.h"
#include "td/defines.h"
#include "td/drive.h"
#include "td/facing.h"
#include "td/ftimer.h"
#include "td/monoc.h"
#include "tech/noinit.h"

class TurretClass : public DriveClass {
 public:
  /*
  **	This is the timer that controls the reload rate. The MSAM rocket
  **	launcher is the primary user of this.
  */
  TCountDownTimerClass Reload;

  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  FacingClass SecondaryFacing;

  void Debug_Dump(MonoClass* mono) const override;
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;

 protected:
  TurretClass(UnitType classid, HousesType house);
  TurretClass() = default;
  TurretClass(const NoInitClass& x)
      : DriveClass(x), Reload(x), SecondaryFacing(x) {}
  ~TurretClass() override = default;

  BulletClass* Fire_At(TARGET target, int which) override;

  DirType Fire_Direction() const override;
  FireErrorType Can_Fire(TARGET target, int which) const override;
  virtual bool Ok_To_Move(DirType facing);
  void AI() override;
  COORDINATE Fire_Coord(int which) const override;
};

#endif  // CNC_RED_ALERT_TD_TURRET_H_
