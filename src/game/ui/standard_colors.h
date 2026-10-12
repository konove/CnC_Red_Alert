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
