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

// The scrolling credits shown from the main menu and after the last mission.
#ifndef CNC_RED_ALERT_RA_SCREENS_EGOS_H_
#define CNC_RED_ALERT_RA_SCREENS_EGOS_H_

#include <string>
#include <string_view>

#include "engine/gfx/pixel_buffer.h"
#include "ra/core/defines.h"

namespace ra {

// One string of credits text on its way up the screen.
class EgoClass {
 public:
  // `x` and `y` are where the text printer is given the string, in pixels on
  // the hidden page: `x` is the left edge, the centre or the right edge of
  // the text according to `flags`, `y` its top row.
  EgoClass(int x, int y, std::string_view text, FontType font,
           TextPrintType flags);
  ~EgoClass() = default;
  EgoClass(const EgoClass&) = delete;
  EgoClass& operator=(const EgoClass&) = delete;
  EgoClass(EgoClass&&) = delete;
  EgoClass& operator=(EgoClass&&) = delete;

  // Moves the text up by `distance` pixels. Returns true once it has left
  // the top of the screen for good.
  bool Scroll(int distance);

  // Draws the text on `view`, unless it is partly or wholly off the page.
  void Render(PixelView& view) const;

  // Erases the text from `view` by copying the rectangle it covers from
  // `background`, which must not be null.
  void Wipe(PixelView& view, PixelBuffer* background) const;

  std::string Text;
  int XPos;
  int YPos;  // Negative once the text has started to leave the page.
  FontType TextFont;
  TextPrintType Flags;
};

// Rolls the credits from CREDITS.TXT over the slideshow, with the credits
// score playing, until the last line has scrolled off or the player presses
// Esc. Returns at once if the file is missing.
//
// Leaves the screen black, the title palette overwritten and the music
// stopped, so the caller redraws and restarts whatever it had.
void Show_Who_Was_Responsible();

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_EGOS_H_
