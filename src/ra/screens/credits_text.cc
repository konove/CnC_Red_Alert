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
constexpr bool EndsString(char ch) {
  return ch == '\t' || ch == '\r' || ch == '\n';
}

}  // namespace

std::vector<CreditString> ParseCredits(std::string_view text) {
  std::vector<CreditString> strings;
  const base::ssize size = std::ssize(text);
  const auto at = [text](base::ssize index) {
    return text.at(base::ToSize(index));
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
        while (string_end < size && !EndsString(at(string_end)) &&
               (at(string_end) != ' ' ||
                (string_end + 1 < size && at(string_end + 1) != ' ' &&
                 !EndsString(at(string_end + 1))))) {
          string_end++;
        }

        const std::string_view string = text.substr(
            base::ToSize(cursor), base::ToSize(string_end - cursor));
        const int start_column = column;
        const int end_column = column + static_cast<int>(string_end - cursor);
        if (start_column < kCreditCenterColumn &&
            end_column > kCreditCenterColumn) {
          strings.push_back({.text = string,
                             .line = line,
                             .column = kCreditCenterColumn,
                             .alignment = CreditAlignment::kCenter});
        } else if (start_column < kCreditCenterColumn) {
          strings.push_back({.text = string,
                             .line = line,
                             .column = end_column,
                             .alignment = CreditAlignment::kRight});
        } else {
          strings.push_back({.text = string,
                             .line = line,
                             .column = start_column,
                             .alignment = CreditAlignment::kLeft});
        }

        // Whatever ended the string is still to be read.
        cursor = string_end;
        column = end_column;
        break;
      }
    }
  }
  return strings;
}

}  // namespace ra
