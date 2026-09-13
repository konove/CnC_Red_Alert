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
  enum SoundControlsClassEnums {
#ifdef FRENCH
    OPTION_WIDTH = 308,
#else
    OPTION_WIDTH = 292,
#endif
    OPTION_HEIGHT = 146,

    OPTION_X = (320 - OPTION_WIDTH) / 2,
    OPTION_Y = (200 - OPTION_HEIGHT) / 2,

    LISTBOX_X = 1,
    LISTBOX_Y = 54,
    LISTBOX_W = 290,
    LISTBOX_H = 72,

    BUTTON_WIDTH = 85,
    BUTTON_X = OPTION_WIDTH - (BUTTON_WIDTH + 7),  // Options button x pos
    BUTTON_Y = 130,                                // Options button y pos

    STOP_X = 5,    // Stop button X.
    STOP_Y = 129,  //	Stop button Y.

    PLAY_X = 23,
    PLAY_Y = 129,

    ONOFF_WIDTH = 25,
#ifdef GERMAN
    SHUFFLE_X = 79,  // BGA:91,
#else
#ifdef FRENCH
    SHUFFLE_X = 99,
#else
    SHUFFLE_X = 91,
#endif
#endif
    SHUFFLE_Y = 130,

#ifdef FRENCH
    REPEAT_X = 174,
#else
    REPEAT_X = 166,
#endif
    REPEAT_Y = 130,

    MSLIDER_X = 147,
    MSLIDER_Y = 28,
    MSLIDER_W = 108,
    MSLIDER_HEIGHT = 5,

    FXSLIDER_X = 147,
    FXSLIDER_Y = 40,
    FXSLIDER_W = 108,
    FXSLIDER_HEIGHT = 5,

    BUTTON_STOP = 605,
    BUTTON_PLAY = 606,
    BUTTON_SHUFFLE = 607,
    BUTTON_REPEAT = 608,
    BUTTON_OPTIONS = 609,
    SLIDER_MUSIC = 610,
    //		SLIDER_SPEECH,
    SLIDER_SOUND = 611,
    BUTTON_LISTBOX = 612,
  };

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
