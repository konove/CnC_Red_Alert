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

/* $Header:   F:\projects\c&c\vcs\code\ftimer.h_v   2.14   16 Oct 1995 16:47:28
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FTIMER.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/16/95 *
 *                                                                                             *
 *                  Last Update : March 16, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_FTIMER_H_
#define CNC_RED_ALERT_TD_FTIMER_H_

#include <algorithm>
#include <cstdint>

#include "td/globals.h"

/*
**	This timer class is based around an external tick system. As such, it is
*inherently *	in sync with any connected system (through network or modem)
*that also keeps the external *	tick system in sync. The game frame number is a
*good sync value.
*/
class TCountDownTimerClass {
 public:
  // Constructor.  Timers set before low level init has been done will not
  // be able to be 'Started' or 'on' until timer system is in place.
  // a countdown assigns and reads as its tick count.
  // NOLINTNEXTLINE(*-explicit-constructor)
  TCountDownTimerClass(int64_t set = 0) noexcept { Set(set); }

  // Saves remaining ticks and whether the timer is active. Restore Frame
  // before reading; the timer re-anchors to it instead of an old frame origin.
  template <class Archive>
  void Serialize(Archive& ar) {
    int64_t remaining = Active() ? Time() : 0;
    bool active = Active();
    ar(remaining, active);
    if constexpr (Archive::kIsReading) {
      if (remaining < 0) {
        ar.Fail("negative countdown duration");
        return;
      }
      if (active) {
        Set(remaining);
      } else {
        Clear();
      }
    }
  }

  // No destructor.

  // a countdown assigns and reads as its tick count.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator int64_t() const { return Time(); }

  // Public functions
  void Set(int64_t set) {
    Started = Frame;
    DelayTime = set;
  }  // Set count down value.

  void Clear() {
    Started = -1;
    DelayTime = 0;
  }
  [[nodiscard]] int64_t Get_Start() const { return Started; }
  [[nodiscard]] int64_t Get_Delay() const { return DelayTime; }
  [[nodiscard]] bool Active() const { return Started != -1; }
  [[nodiscard]] int Expired() const { return Time() == 0; }
  [[nodiscard]] int64_t Time() const {
    return std::max<int64_t>(DelayTime - (Frame - Started), 0);
  }  // Fetch current count down value.

 protected:
  int64_t Started = -1;  // Initial frame time start.
  int64_t DelayTime = 0;  // Ticks remaining before countdown timer expires.
};

#endif  // CNC_RED_ALERT_TD_FTIMER_H_
