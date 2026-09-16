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

/* $Header:   F:\projects\c&c\vcs\code\options.h_v   2.18   16 Oct 1995 16:46:20
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_OPTIONS_H_
#define CNC_RED_ALERT_TD_OPTIONS_H_

#include <span>

class OptionsClass {
 public:
  static constexpr int kMaxScrollSetting = 7;
  static constexpr int kMaxSpeedSetting = 7;

  OptionsClass();

  void One_Time();

  void Fixup_Palette() const;
  void Set_Shuffle(int on);
  void Set_Repeat(int on);
  void Set_Score_Volume(int volume);
  void Set_Sound_Volume(int volume, bool feedback);
  void Set_Brightness(int brightness);
  [[nodiscard]] int Get_Brightness() const;
  void Set_Color(int color);
  [[nodiscard]] int Get_Color() const;
  void Set_Contrast(int contrast);
  [[nodiscard]] int Get_Contrast() const;
  void Set_Tint(int tint);
  [[nodiscard]] int Get_Tint() const;
  [[nodiscard]] int Normalize_Delay(int delay) const;
  [[nodiscard]] int Normalize_Sound(int volume) const;

  /*
  ** File I/O routines
  */
  void Load_Settings();
  void Save_Settings() const;

  void Set();

  /*
  **	This is actually the delay between game frames expressed as 1/60 of
  **	a second. The default value is 4 (1/15 second).
  */
  unsigned int GameSpeed;

  int ScrollRate;  // Distance to scroll.
  unsigned char Brightness{0x80};
  unsigned char Volume{0xE0};            // Volume for sound effects.
  unsigned char ScoreVolume{0x90};       // Volume for scores.
  unsigned char Contrast{0x80};          // Value
  unsigned char Color{0x80};             // Saturation
  unsigned char Tint{0x80};              // Hue
  bool AutoScroll : 1 {true};            // Does map autoscroll?
  bool IsScoreRepeat : 1 {false};        // Score should repeat?
  bool IsScoreShuffle : 1 {false};       // Score list should shuffle?
  bool IsDeathAnnounce : 1 {false};      // Announce enemy deaths?
  bool IsFreeScroll : 1 {false};         // Allow free direction scrolling?

 protected:
  static void Adjust_Palette(std::span<const unsigned char> oldpal,
                             std::span<unsigned char> newpal,
                             unsigned char brightness, unsigned char color,
                             unsigned char tint, unsigned char contrast);

 private:
};

#endif  // CNC_RED_ALERT_TD_OPTIONS_H_
