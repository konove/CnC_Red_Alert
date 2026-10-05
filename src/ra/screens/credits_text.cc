#include "ra/screens/credits_text.h"

#include <iterator>
#include <string_view>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"

namespace ra {

namespace {

// Tab stops in CREDITS.TXT are this many columns apart.
constexpr int kTabColumns = 8;

// The column that divides the page: a string wholly to its left is
// right-justified, one wholly to its right left-justified.
constexpr int kCreditCenterColumn = kCreditColumns / 2;

// True for the characters no string of the credits contains.
constexpr bool EndsString(const char ch) {
  return ch == '\t' || ch == '\r' || ch == '\n';
}

}  // namespace

std::vector<CreditString> ParseCredits(std::string_view text) {
  std::vector<CreditString> strings;
  const base::ssize size = std::ssize(text);
  const auto at = [text](const base::ssize index) {
    return text.at(base::ToSize(index));
  };
  // True where no string can continue: past the end and at a tab or line end.
  const auto ends_string = [size, at](const base::ssize index) {
    return index >= size || EndsString(at(index));
  };
  int line = 0;
  int column = 0;
  base::ssize cursor = 0;
  while (cursor < size) {
    switch (at(cursor)) {
      case '\r':
        line++;
        column = 0;
        cursor++;
        break;

      case '\n':
        // The carriage return already ended the line.
        cursor++;
        break;

      case ' ':
        column++;
        cursor++;
        break;

      case '\t':
        column += kTabColumns;
        column -= column % kTabColumns;
        cursor++;
        break;

      default: {
        // The start of a string. It runs up to a tab, the end of the line or
        // the end of the text, and a space belongs to it only when another
        // character of the string follows.
        base::ssize string_end = cursor + 1;
        while (!ends_string(string_end) &&
               (at(string_end) != ' ' ||
                (!ends_string(string_end + 1) && at(string_end + 1) != ' '))) {
          string_end++;
        }

        const int end_column = column + static_cast<int>(string_end - cursor);
        CreditString string = {
            .text = text.substr(base::ToSize(cursor),
                                base::ToSize(string_end - cursor)),
            .line = line,
            .column = column,
            .alignment = CreditAlignment::kLeft};
        if (column < kCreditCenterColumn) {
          if (end_column > kCreditCenterColumn) {
            string.column = kCreditCenterColumn;
            string.alignment = CreditAlignment::kCenter;
          } else {
            string.column = end_column;
            string.alignment = CreditAlignment::kRight;
          }
        }
        strings.push_back(string);

        // Whatever ended the string is still to be read.
        cursor = string_end;
        column = end_column;
      }
    }
  }
  return strings;
}

}  // namespace ra
