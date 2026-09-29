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

// HsvColor's conversion to PaletteColor.

#include "engine/gfx/hsv_color.h"

#include "base/hsv.h"
#include "engine/gfx/palette_color.h"

namespace engine::gfx {

PaletteColor HsvColor::ToPaletteColor() const {
  const auto [red, green, blue] = base::HsvToRgb8(hue(), saturation(), value());

  // Round each gun to the nearest 6-bit step rather than dropping the low two
  // bits, which is what keeps PaletteColor::ToHsv() followed by this within
  // one step of the original color.
  const auto scale_down = [](const int component) {
    return base::DivideWithRound(component * PaletteColor::kMaxGun,
                                 PaletteColor::kMaxComponent);
  };
  return PaletteColor::FromGuns(scale_down(red), scale_down(green),
                                scale_down(blue));
}

}  // namespace engine::gfx
