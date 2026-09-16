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

/* $Header:   F:\projects\c&c\vcs\code\fuse.cpv   2.18   16 Oct 1995 16:50:46
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FUSE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 24, 1994 *
 *                                                                                             *
 *                  Last Update : October 17, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * FuseClass::Arm_Fuse -- Sets up fuse for detonation check. *
 *   FuseClass::Fuse_Checkup -- Determines if the fuse triggers. *
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

/***********************************************************************************************
 * FuseClass::FuseClass -- Constructor. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/27/1995 BRR : Created.  Gosh, what a lotta work. *
 *=============================================================================================*/
#include "td/fuse.h"

#include <algorithm>
#include <cstdint>

#include "td/defines.h"
#include "td/display_constants.h"
#include "td/inline.h"

FuseClass::FuseClass() = default;

/***********************************************************************************************
 * FuseClass::Arm_Fuse -- Sets up fuse for detonation check. *
 *                                                                                             *
 *    This starts a fuse. Fuses are proximity detonation variety but * can be
 *modified to have a minimum time to elapse before detonation * and a maximum
 *time to exist before detonation. Typically, the                            *
 *    timing values are used for missiles that have a minimum arming * distance
 *and a limited amount of fuel. *
 *                                                                                             *
 * INPUT:   location -- The coordinate where the projectile start. This * is
 *needed for proper proximity tracking.                               *
 *                                                                                             *
 *          target   -- The actual impact point. Fuses are based on real * word
 *coordinates.                                                      *
 *                                                                                             *
 *          time     -- The maximum time that the fuse may work before *
 *                      explosion is forced. *
 *                                                                                             *
 *          arming   -- The minimum time that must elapse before the * fuse may
 *explode.                                                      *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/24/1994 JLB : Created. *
 *=============================================================================================*/
void FuseClass::Arm_Fuse(COORDINATE location, COORDINATE target, int timeto,
                         int arming) {
  timeto = std::max(timeto, arming);
  Timer = static_cast<unsigned char>(std::min(timeto, 0xFF));
  Arming = static_cast<unsigned char>(std::min(arming, 0xFF));
  HeadTo = target;
  Proximity = static_cast<int16_t>(Distance(location, target));
}

/***********************************************************************************************
 * FuseClass::Fuse_Checkup -- Determines if the fuse triggers. *
 *                                                                                             *
 *    This will process the fuse and update the internal clocks as well * as
 *check to see if the fuse should trigger (explode) or not. *
 *                                                                                             *
 * INPUT:   newlocation -- The new location of the fuse. This is needed * to
 *determine proximity explosions.                                  *
 *                                                                                             *
 * OUTPUT:  bool; Was the fuse triggered to explode now? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/24/1994 JLB : Created. *
 *=============================================================================================*/
bool FuseClass::Fuse_Checkup(COORDINATE newlocation) {

  /*
  **	Always decrement the fuse timer.
  */
  if (Timer) {
    Timer--;
  }

  /*
  **	If the arming countdown has not expired, then do nothing.
  */
  if (Arming) {
    Arming--;
  } else {
    /*
    **	If the timer has run out, then the warhead explodes.
    */
    if (!Timer) {
      return true;
    }

    const int proximity = Distance(newlocation, HeadTo);
    if (proximity < 0x0010) {
      return true;
    }
    if (proximity < ICON_LEPTON_W && proximity > Proximity) {
      return true;
    }
    Proximity = static_cast<int16_t>(proximity);
  }
  return false;
}
