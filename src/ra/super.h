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

/* $Header: /CounterStrike/SUPER.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SUPER.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/28/95 *
 *                                                                                             *
 *                  Last Update : July 28, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_SUPER_H_
#define CNC_RED_ALERT_RA_SUPER_H_

#include "ra/defines.h"
#include "ra/jshell.h"
#include "tech/ftimer.h"

class SuperClass {
 public:
  SuperClass() = default;
  SuperClass(int recharge, bool powered, VoxType charging = VOX_NONE,
             VoxType ready = VOX_NONE, VoxType impatient = VOX_NONE,
             VoxType suspend = VOX_NONE);

  bool Suspend(bool on);
  bool Enable(bool onetime = false, bool player = false, bool quiet = false);
  void Forced_Charge(bool player = false);
  bool AI(bool player = false);
  bool Remove();
  void Impatient_Click() const;
  int Anim_Stage() const;
  bool Discharged(bool player);

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool is_powered = IsPowered;
    bool is_present = IsPresent;
    bool is_one_time = IsOneTime;
    bool is_ready = IsReady;
    ar(is_powered, is_present, is_one_time, is_ready, Control, OldStage,
       VoxRecharge, VoxCharging, VoxImpatient, VoxSuspend, RechargeTime);
    IsPowered = is_powered;
    IsPresent = is_present;
    IsOneTime = is_one_time;
    IsReady = is_ready;
  }
  bool Is_Ready() const { return IsReady; }
  bool Is_Present() const { return IsPresent; }
  bool Is_One_Time() const { return IsOneTime && IsPresent; }
  bool Is_Powered() const { return IsPowered; }

 private:
  bool Recharge(bool player = false);

  unsigned IsPowered : 1 = false;
  unsigned IsPresent : 1 = false;
  unsigned IsOneTime : 1 = false;
  unsigned IsReady : 1 = false;

  Timer<FrameTickSource> Control;
  int OldStage = -1;

  VoxType VoxRecharge = VOX_NONE;
  VoxType VoxCharging = VOX_NONE;
  VoxType VoxImpatient = VOX_NONE;
  VoxType VoxSuspend = VOX_NONE;
  int RechargeTime = 0;

  enum { ANIMATION_STAGES = 54 };
};

#endif  // CNC_RED_ALERT_RA_SUPER_H_
