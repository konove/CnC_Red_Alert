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

/* $Header:   F:\projects\c&c\vcs\code\super.h_v   1.5   16 Oct 1995 16:47:04
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_SUPER_H_
#define CNC_RED_ALERT_TD_SUPER_H_

#include "td/defines.h"
#include "td/ftimer.h"

class SuperClass {
 public:
  explicit SuperClass(int recharge = 0, VoxType ready = VOX_NONE,
             VoxType charging = VOX_NONE, VoxType impatient = VOX_NONE,
             VoxType suspend = VOX_NONE)
      : VoxRecharge(ready),
        VoxCharging(charging),
        VoxImpatient(impatient),
        VoxSuspend(suspend),
        RechargeTime(recharge) {}

  // Preserve charging progress and suspension independently of the current
  // frame.
  template <class Archive>
  void Serialize(Archive& ar) {
    bool present = IsPresent, one_time = IsOneTime, ready = IsReady,
         suspended = IsSuspended;
    ar(present, one_time, ready, suspended, Control, OldStage, SuspendTime,
       VoxRecharge, VoxCharging, VoxImpatient, VoxSuspend, RechargeTime);
    if constexpr (Archive::kIsReading) {
      IsPresent = present;
      IsOneTime = one_time;
      IsReady = ready;
      IsSuspended = suspended;
      if (RechargeTime < 0 || SuspendTime < 0 || VoxRecharge < VOX_NONE ||
          VoxRecharge >= VOX_COUNT || VoxCharging < VOX_NONE ||
          VoxCharging >= VOX_COUNT || VoxImpatient < VOX_NONE ||
          VoxImpatient >= VOX_COUNT || VoxSuspend < VOX_NONE ||
          VoxSuspend >= VOX_COUNT) {
        ar.Fail("invalid superweapon state");
      }
    }
  }

  bool Suspend(bool on);
  bool Enable(bool onetime = false, bool player = false, bool quiet = false);
  void Forced_Charge(bool player = false);
  bool AI(bool player = false);
  bool Remove(bool forced = false);
  void Impatient_Click() const;
  int Anim_Stage() const;
  bool Discharged(bool player);
  bool Is_Ready() const { return IsReady; }
  bool Is_Present() const { return IsPresent; }
  bool Is_One_Time() const { return IsOneTime && IsPresent; }

 private:
  bool Recharge(bool player = false);

  unsigned IsPresent : 1 = false;
  unsigned IsOneTime : 1 = false;
  unsigned IsReady : 1 = false;
  unsigned IsSuspended : 1 = false;

  TCountDownTimerClass Control;
  int OldStage = -1;
  int SuspendTime = 0;

  VoxType VoxRecharge;
  VoxType VoxCharging;
  VoxType VoxImpatient;
  VoxType VoxSuspend;
  int RechargeTime;

  enum { ANIMATION_STAGES = 102 };
};

#endif  // CNC_RED_ALERT_TD_SUPER_H_
