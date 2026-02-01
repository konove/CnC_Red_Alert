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

/* $Header:   F:\projects\c&c\vcs\code\visudlg.h_v   2.15   16 Oct 1995 16:47:46
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VISUDLG.H *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg
 ** Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995 *
 *                                                                                             *
 *                  Last Update : Jan 18, 1995   [MML] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 *---------------------------------------------------------------------------------------------*/

#ifndef CNC_RED_ALERT_TD_VISUDLG_H_
#define CNC_RED_ALERT_TD_VISUDLG_H_

class VisualControlsClass {
 private:
  enum VisualControlEnums {
    BUTTON_BRIGHTNESS = 1,
    BUTTON_BASE = BUTTON_BRIGHTNESS,  // Base for zero-indexed button offsets.
    BUTTON_COLOR,
    BUTTON_CONTRAST,
    BUTTON_TINT,
    BUTTON_RESET,
    BUTTON_OPTIONS,
  };

  // Layout constants.
  static constexpr int kOptionWidth = 216;
  static constexpr int kOptionHeight = 122;
  static constexpr int kOptionX = (320 - kOptionWidth) / 2;
  static constexpr int kOptionY = (200 - kOptionHeight) / 2;
  static constexpr int kTextX = kOptionX + 28;
  static constexpr int kTextY = kOptionY + 30;
  static constexpr int kSliderX = kOptionX + 105;
  static constexpr int kSliderY = kOptionY + 30;
  static constexpr int kSliderWidth = 70;
  static constexpr int kSliderHeight = 5;
  static constexpr int kSliderYSpacing = 11;
  static constexpr int kButtonX = kOptionX + 63;
  static constexpr int kButtonY = kOptionY + 102;

 public:
  VisualControlsClass() = default;
  void Process();
};

#endif  // CNC_RED_ALERT_TD_VISUDLG_H_
