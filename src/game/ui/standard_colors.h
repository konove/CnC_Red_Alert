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
// Every theater palette of both games (Red Alert's TEMPERAT, SNOW and INTERIOR,
// Tiberian Dawn's TEMPERAT, DESERT and WINTER) and Red Alert's menu palette
// (PALETTE.CPS) hold the same sixteen colors in slots 0-15, the EGA set in
// Westwood's order, so text, dialog frames and debug overlays can name a color
// without looking it up in the palette that happens to be current. (One value
// differs: the red of kLightGreen is 22 of 63 in Red Alert and 21 in Tiberian
// Dawn.) The names fit what the files hold, in both games: kPurple is magenta
// (aa00aa), kPink a light red (ff5555), kLightBlue a bright blue (5151ff).
//
// Tiberian Dawn's gradient fonts do not draw these slots: with
// TPF_USE_GRAD_PAL, TPF_MEDIUM_COLOR or TPF_BRIGHT_COLOR its TextStyleFor()
// (td/ui/dialog.cc) takes the color as a row of its font palette tables, and
// the row draws other slots. kPurple gives oranges (24-27), kBrown tans
// (176-180), kCyan blue-greens (135, 136, 119) and kLightBlue blue-greys
// (201-204), which is why its player text colors (td/net/session.h) are
// commented "Orange" and "BlueGreen".
//
// A screen that brings its own palette need not keep the set: Tiberian Dawn's
// map selection (DARK_*.PAL) and satellite view (SATSEL.PAL) put their own
// colors in slots 1-11 and keep little more than black and white.

#ifndef CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_
#define CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_

#include "engine/gfx/palette_index.h"

namespace game {

// Index 0 shows as black, but a transparent blit or a text background skips it:
// "transparent black". kBlack is the black that is always drawn.
inline constexpr engine::gfx::PaletteIndex kTransparentBlack{0};
inline constexpr engine::gfx::PaletteIndex kPurple{1};
inline constexpr engine::gfx::PaletteIndex kCyan{2};
inline constexpr engine::gfx::PaletteIndex kGreen{3};
inline constexpr engine::gfx::PaletteIndex kLightGreen{4};
inline constexpr engine::gfx::PaletteIndex kYellow{5};
inline constexpr engine::gfx::PaletteIndex kPink{6};
inline constexpr engine::gfx::PaletteIndex kBrown{7};
inline constexpr engine::gfx::PaletteIndex kRed{8};
inline constexpr engine::gfx::PaletteIndex kLightCyan{9};
inline constexpr engine::gfx::PaletteIndex kLightBlue{10};
inline constexpr engine::gfx::PaletteIndex kBlue{11};
inline constexpr engine::gfx::PaletteIndex kBlack{12};
inline constexpr engine::gfx::PaletteIndex kGrey{13};
inline constexpr engine::gfx::PaletteIndex kLightGrey{14};
inline constexpr engine::gfx::PaletteIndex kWhite{15};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_STANDARD_COLORS_H_
