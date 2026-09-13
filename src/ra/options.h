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

/* $Header: /CounterStrike/OPTIONS.H 1     3/03/97 10:25a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_OPTIONS_H_
#define CNC_RED_ALERT_RA_OPTIONS_H_

#include "ra/palette.h"
#include "sdllib/keyboard.h"
#include "tech/fixed.h"

class OptionsClass {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  enum { MAX_SCROLL_SETTING = 7, MAX_SPEED_SETTING = 7 };

  OptionsClass();

  void One_Time();

  void Fixup_Palette() const;
  void Set_Shuffle(bool on);
  void Set_Repeat(bool on);
  void Set_Score_Volume(fixed volume, bool feedback);
  void Set_Sound_Volume(fixed volume, bool feedback);
  void Set_Brightness(fixed brightness);
  [[nodiscard]] fixed Get_Brightness() const;
  void Set_Saturation(fixed color);
  [[nodiscard]] fixed Get_Saturation() const;
  void Set_Contrast(fixed contrast);
  [[nodiscard]] fixed Get_Contrast() const;
  void Set_Tint(fixed tint);
  [[nodiscard]] fixed Get_Tint() const;
  [[nodiscard]] int Normalize_Delay(int delay) const;
  [[nodiscard]] int Normalize_Volume(int volume) const;

  /*
  ** File I/O routines
  */
  void Load_Settings();
  void Save_Settings();

  void Set();

  /*
  **	This is actually the delay between game frames expressed as 1/60 of
  **	a second. The default value is 4 (1/15 second).
  */
  unsigned int GameSpeed{3};

  int ScrollRate{3};             // Distance to scroll.
  fixed Volume;                  // Volume for sound effects.
  fixed ScoreVolume;             // Volume for scores.
  fixed MultiScoreVolume;        // Volume for scores during multiplayer games.
  fixed Brightness;              // Brightness.
  fixed Tint;                    // Hue
  fixed Saturation;              // Saturation
  fixed Contrast;                // Value
  bool AutoScroll : 1 {true};    // Does map autoscroll?
  bool IsScoreRepeat : 1 {false};   // Score should repeat?
  bool IsScoreShuffle : 1 {false};  // Score list should shuffle?
  bool IsPaletteScroll : 1 {true};  // Allow palette scrolling?

  /*
  **	These are the hotkeys used for keyboard control.
  */
  KeyNumType KeyForceMove1{KN_LALT};
  KeyNumType KeyForceMove2{KN_RALT};
  KeyNumType KeyForceAttack1{KN_LCTRL};
  KeyNumType KeyForceAttack2{KN_RCTRL};
  KeyNumType KeySelect1{KN_LSHIFT};
  KeyNumType KeySelect2{KN_RSHIFT};
  KeyNumType KeyScatter{KN_X};
  KeyNumType KeyStop{KN_S};
  KeyNumType KeyGuard{KN_G};
  KeyNumType KeyNext{KN_N};
  KeyNumType KeyPrevious{KN_B};
  KeyNumType KeyFormation{KN_F};
  KeyNumType KeyHome1{KN_HOME};
  KeyNumType KeyHome2{KN_E_HOME};
  KeyNumType KeyBase{KN_H};
  KeyNumType KeyResign{KN_R};
  KeyNumType KeyAlliance{KN_A};
  KeyNumType KeyBookmark1{KN_F9};
  KeyNumType KeyBookmark2{KN_F10};
  KeyNumType KeyBookmark3{KN_F11};
  KeyNumType KeyBookmark4{KN_F12};
  KeyNumType KeySelectView{KN_E};
  KeyNumType KeyRepair{KN_T};
  KeyNumType KeyRepairOn{KN_NONE};
  KeyNumType KeyRepairOff{KN_NONE};
  KeyNumType KeySell{KN_Y};
  KeyNumType KeySellOn{KN_NONE};
  KeyNumType KeySellOff{KN_NONE};
  KeyNumType KeyMap{KN_U};
  KeyNumType KeySidebarUp{KN_UP};
  KeyNumType KeySidebarDown{KN_DOWN};
  KeyNumType KeyOption1{KN_ESC};
  KeyNumType KeyOption2{KN_SPACE};
  KeyNumType KeyScrollLeft{KN_NONE};
  KeyNumType KeyScrollRight{KN_NONE};
  KeyNumType KeyScrollUp{KN_NONE};
  KeyNumType KeyScrollDown{KN_NONE};
  KeyNumType KeyQueueMove1{KN_Q};
  KeyNumType KeyQueueMove2{KN_Q};
  KeyNumType KeyTeam1{KN_1};
  KeyNumType KeyTeam2{KN_2};
  KeyNumType KeyTeam3{KN_3};
  KeyNumType KeyTeam4{KN_4};
  KeyNumType KeyTeam5{KN_5};
  KeyNumType KeyTeam6{KN_6};
  KeyNumType KeyTeam7{KN_7};
  KeyNumType KeyTeam8{KN_8};
  KeyNumType KeyTeam9{KN_9};
  KeyNumType KeyTeam10{KN_0};

  static void Adjust_Palette(const PaletteClass& oldpal, PaletteClass& newpal,
                             fixed brightness, fixed color, fixed tint,
                             fixed contrast);

 protected:
 private:
  static const char* const HotkeyName;
};

class ArchiveReader;
class ArchiveWriter;
extern template void OptionsClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void OptionsClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_OPTIONS_H_
