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

#include <string_view>
#include <vector>

namespace ra {

// CREDITS.TXT is laid out as a page this many columns wide.
inline constexpr int kCreditColumns = 80;

// Which point of a credits string its column places.
enum class CreditAlignment {
  kLeft,    // the string starts at the column
  kCenter,  // the string is centred on it
  kRight,   // the string ends at it
};

// One string of CREDITS.TXT and where on the page it goes.
struct CreditString {
  std::string_view text;  // Points into the text that was parsed.
  int line = 0;           // Row on the page, from 0.
  int column = 0;         // Out of kCreditColumns; see `alignment`.
  CreditAlignment alignment = CreditAlignment::kLeft;

  bool operator==(const CreditString&) const = default;
};

// Splits the text of CREDITS.TXT into the strings to draw, in file order. A
// string ends at a tab, at the end of the line or at two spaces in a row, and
// its place on the 80-column page decides how it is drawn, which gives the
// two-column "job  name" look:
//
//   it starts left of column 40 and ends right of it   centred on the screen
//   it starts left of column 40 otherwise              right-justified at its
//                                                      last column
//   it starts at column 40 or beyond                   left-justified at its
//                                                      first column
//
// Lines end with a carriage return; line feeds are ignored. The strings point
// into `text`, which must outlive them.
std::vector<CreditString> ParseCredits(std::string_view text);

// Rolls the credits from CREDITS.TXT over the slideshow, with the credits
// score playing, until the last line has scrolled off or the player presses
// Esc. Returns at once if the file is missing.
//
// Leaves the screen black, the title palette overwritten and the music
// stopped, so the caller redraws and restarts whatever it had.
void ShowCredits();

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_EGOS_H_
