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

/* $Header:   F:\projects\c&c\vcs\code\sounddlg.h_v   2.18   16 Oct 1995
 * 16:46:48   JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_SOUNDDLG_H_
#define CNC_RED_ALERT_TD_SOUNDDLG_H_

class SoundControlsClass {
#ifdef FRENCH
  static constexpr int kOptionWidth = 308;
#else
  static constexpr int kOptionWidth = 292;
#endif
  static constexpr int kOptionHeight = 146;

  static constexpr int kOptionX = (320 - kOptionWidth) / 2;
  static constexpr int kOptionY = (200 - kOptionHeight) / 2;

  static constexpr int kListboxX = 1;
  static constexpr int kListboxY = 54;
  static constexpr int kListboxW = 290;
  static constexpr int kListboxH = 72;

  static constexpr int kButtonWidth = 85;
  static constexpr int kButtonX =
      kOptionWidth - (kButtonWidth + 7);  // Options button x pos
  static constexpr int kButtonY = 130;    // Options button y pos

  static constexpr int kStopX = 5;    // Stop button X.
  static constexpr int kStopY = 129;  //	Stop button Y.

  static constexpr int kPlayX = 23;
  static constexpr int kPlayY = 129;

  static constexpr int kOnoffWidth = 25;
#ifdef GERMAN
  static constexpr int kShuffleX = 79;  // BGA:91,
#else
#ifdef FRENCH
  static constexpr int kShuffleX = 99;
#else
  static constexpr int kShuffleX = 91;
#endif
#endif
  static constexpr int kShuffleY = 130;

#ifdef FRENCH
  static constexpr int kRepeatX = 174;
#else
  static constexpr int kRepeatX = 166;
#endif
  static constexpr int kRepeatY = 130;

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
  //		SLIDER_SPEECH,
  static constexpr int kSliderSound = 611;
  static constexpr int kButtonListbox = 612;

 public:
  SoundControlsClass() = default;
  void Process();
  int Init();

 private:
  int Option_Width = 0;
  int Option_Height = 0;

  int Option_X = 0;
  int Option_Y = 0;

  int Listbox_X = 0;
  int Listbox_Y = 0;
  int Listbox_W = 0;
  int Listbox_H = 0;

  int Button_Width = 0;
  int Button_X = 0;
  int Button_Y = 0;

  int Stop_X = 0;
  int Stop_Y = 0;

  int Play_X = 0;
  int Play_Y = 0;

  int OnOff_Width = 0;

  int Shuffle_X = 0;
  int Shuffle_Y = 0;

  int Repeat_X = 0;
  int Repeat_Y = 0;

  int MSlider_X = 0;
  int MSlider_Y = 0;
  int MSlider_W = 0;
  int MSlider_Height = 0;

  int FXSlider_X = 0;
  int FXSlider_Y = 0;
  int FXSlider_W = 0;
  int FXSlider_Height = 0;
};

#endif  // CNC_RED_ALERT_TD_SOUNDDLG_H_
