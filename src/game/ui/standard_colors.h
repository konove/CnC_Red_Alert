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

// Names for the first sixteen palette indices. Originally part of the Westwood
// standard header (WWSTD.H, Jeff Wilson, March 1994).
//
// The games' palettes reserve slots 0-15 for a fixed set of colors, so text,
// dialog frames and debug overlays can name a color without looking it up in
// the palette that happens to be current. The names are the Westwood library's
// and only approximate: Tiberian Dawn's palette has orange at kPurple and tan at
// kBrown, and aliases them in td/core/defines.h (kCcOrange, kCcTan).

#ifndef CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_
#define CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_

namespace game {

// Palette indices, `int` because that is what the drawing functions take.

// Index 0 shows as black, but a transparent blit or a text background skips it:
// "transparent black". kBlack is the black that is always drawn.
inline constexpr int kTransparentBlack = 0;
inline constexpr int kPurple = 1;
inline constexpr int kCyan = 2;
inline constexpr int kGreen = 3;
inline constexpr int kLightGreen = 4;
inline constexpr int kYellow = 5;
inline constexpr int kPink = 6;
inline constexpr int kBrown = 7;
inline constexpr int kRed = 8;
inline constexpr int kLightCyan = 9;
inline constexpr int kLightBlue = 10;
inline constexpr int kBlue = 11;
inline constexpr int kBlack = 12;
inline constexpr int kGrey = 13;
inline constexpr int kLightGrey = 14;
inline constexpr int kWhite = 15;

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_
