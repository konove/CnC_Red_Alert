// The layout of CREDITS.TXT, the text of the scrolling credits.
#ifndef CNC_RED_ALERT_RA_SCREENS_CREDITS_TEXT_H_
#define CNC_RED_ALERT_RA_SCREENS_CREDITS_TEXT_H_

#include <iosfwd>
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

  // Writes the string and its place on the page, for logs and failed test
  // expectations.
  friend std::ostream& operator<<(std::ostream& os, const CreditString& string);
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

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_SCREENS_CREDITS_TEXT_H_
