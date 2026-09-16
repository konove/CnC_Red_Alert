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

/* $Header: /CounterStrike/SOUNDDLG.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : OPTIONS.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : June 8, 1994 *
 *                                                                                             *
 *                  Last Update : June 8, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_SOUNDDLG_H_
#define CNC_RED_ALERT_RA_SOUNDDLG_H_

#include "ra/config.h"

class SoundControlsClass {
  static constexpr int kOptionWidth = config::kIsFrench ? 308 : 292;
  static constexpr int kOptionHeight = 146;

  static constexpr int kOptionX = (320 - kOptionWidth) / 2;
  static constexpr int kOptionY = (200 - kOptionHeight) / 2;

  static constexpr int kListboxX = 17;
  static constexpr int kListboxY = 54;
  static constexpr int kListboxW = kOptionWidth - (kListboxX * 2);
  static constexpr int kListboxH = 72;

  static constexpr int kButtonWidth = 70;
  static constexpr int kButtonX =
      kOptionWidth - (kButtonWidth + 17);  // Options button x pos
  static constexpr int kButtonY = 128;     // Options button y pos

  static constexpr int kStopX = 17;   // Stop button X.
  static constexpr int kStopY = 128;  //	Stop button Y.

  static constexpr int kPlayX = 35;
  static constexpr int kPlayY = 128;

  static constexpr int kOnoffWidth = 25;
  static constexpr int kShuffleX = [] {
    if (config::kIsGerman) {
      return 79;
    }
    if (config::kIsFrench) {
      return 99;
    }
    return 97;
  }();
  static constexpr int kShuffleY = 128;

  static constexpr int kRepeatX = config::kIsFrench ? 169 : 164;
  static constexpr int kRepeatY = 128;

  static constexpr int kMsliderX = 147;
  static constexpr int kMsliderY = 28;
  static constexpr int kMsliderW = 108;
  static constexpr int kMsliderHeight = 5;

  static constexpr int kFxsliderX = 147;
  static constexpr int kFxsliderY = 40;
  static constexpr int kFxsliderW = 108;
  static constexpr int kFxsliderHeight = 5;

  static constexpr int kButtonStop = 605;
  static constexpr int kButtonPlay = 606;
  static constexpr int kButtonShuffle = 607;
  static constexpr int kButtonRepeat = 608;
  static constexpr int kButtonOptions = 609;
  static constexpr int kSliderMusic = 610;
  static constexpr int kSliderSound = 611;
  static constexpr int kButtonListbox = 612;

 public:
  SoundControlsClass() = default;
  static void Process();
};

#endif  // CNC_RED_ALERT_RA_SOUNDDLG_H_
