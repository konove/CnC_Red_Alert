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

#include "base/fixed.h"
#include "engine/window/keyboard.h"
#include "ra/palette.h"

class INIClass;

class OptionsClass {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  static constexpr int kMaxScrollSetting = 7;
  static constexpr int kMaxSpeedSetting = 7;

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
  void Save_Settings() const;

  // Reads the [WinHotkeys] bindings from `ini`, translating the Windows
  // virtual-key codes stored there. A binding the file lacks keeps its value.
  void LoadHotkeys(const INIClass& ini);

  // Writes the hotkey bindings to `ini`'s [WinHotkeys] as Windows virtual-key
  // codes, the form the original Windows game reads.
  void SaveHotkeys(INIClass& ini) const;

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
  engine::window::KeyNumber KeyForceMove1{engine::window::KN_LALT};
  engine::window::KeyNumber KeyForceMove2{engine::window::KN_RALT};
  engine::window::KeyNumber KeyForceAttack1{engine::window::KN_LCTRL};
  engine::window::KeyNumber KeyForceAttack2{engine::window::KN_RCTRL};
  engine::window::KeyNumber KeySelect1{engine::window::KN_LSHIFT};
  engine::window::KeyNumber KeySelect2{engine::window::KN_RSHIFT};
  engine::window::KeyNumber KeyScatter{engine::window::KN_X};
  engine::window::KeyNumber KeyStop{engine::window::KN_S};
  engine::window::KeyNumber KeyGuard{engine::window::KN_G};
  engine::window::KeyNumber KeyNext{engine::window::KN_N};
  engine::window::KeyNumber KeyPrevious{engine::window::KN_B};
  engine::window::KeyNumber KeyFormation{engine::window::KN_F};
  engine::window::KeyNumber KeyHome1{engine::window::KN_HOME};
  engine::window::KeyNumber KeyHome2{engine::window::KN_E_HOME};
  engine::window::KeyNumber KeyBase{engine::window::KN_H};
  engine::window::KeyNumber KeyResign{engine::window::KN_R};
  engine::window::KeyNumber KeyAlliance{engine::window::KN_A};
  engine::window::KeyNumber KeyBookmark1{engine::window::KN_F9};
  engine::window::KeyNumber KeyBookmark2{engine::window::KN_F10};
  engine::window::KeyNumber KeyBookmark3{engine::window::KN_F11};
  engine::window::KeyNumber KeyBookmark4{engine::window::KN_F12};
  engine::window::KeyNumber KeySelectView{engine::window::KN_E};
  engine::window::KeyNumber KeyRepair{engine::window::KN_T};
  engine::window::KeyNumber KeyRepairOn{engine::window::KN_NONE};
  engine::window::KeyNumber KeyRepairOff{engine::window::KN_NONE};
  engine::window::KeyNumber KeySell{engine::window::KN_Y};
  engine::window::KeyNumber KeySellOn{engine::window::KN_NONE};
  engine::window::KeyNumber KeySellOff{engine::window::KN_NONE};
  engine::window::KeyNumber KeyMap{engine::window::KN_U};
  engine::window::KeyNumber KeySidebarUp{engine::window::KN_UP};
  engine::window::KeyNumber KeySidebarDown{engine::window::KN_DOWN};
  engine::window::KeyNumber KeyOption1{engine::window::KN_ESC};
  engine::window::KeyNumber KeyOption2{engine::window::KN_SPACE};
  engine::window::KeyNumber KeyScrollLeft{engine::window::KN_NONE};
  engine::window::KeyNumber KeyScrollRight{engine::window::KN_NONE};
  engine::window::KeyNumber KeyScrollUp{engine::window::KN_NONE};
  engine::window::KeyNumber KeyScrollDown{engine::window::KN_NONE};
  engine::window::KeyNumber KeyQueueMove1{engine::window::KN_Q};
  engine::window::KeyNumber KeyQueueMove2{engine::window::KN_Q};
  engine::window::KeyNumber KeyTeam1{engine::window::KN_1};
  engine::window::KeyNumber KeyTeam2{engine::window::KN_2};
  engine::window::KeyNumber KeyTeam3{engine::window::KN_3};
  engine::window::KeyNumber KeyTeam4{engine::window::KN_4};
  engine::window::KeyNumber KeyTeam5{engine::window::KN_5};
  engine::window::KeyNumber KeyTeam6{engine::window::KN_6};
  engine::window::KeyNumber KeyTeam7{engine::window::KN_7};
  engine::window::KeyNumber KeyTeam8{engine::window::KN_8};
  engine::window::KeyNumber KeyTeam9{engine::window::KN_9};
  engine::window::KeyNumber KeyTeam10{engine::window::KN_0};

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
