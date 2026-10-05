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

}  // namespace

std::vector<CreditString> ParseCredits(std::string_view text) {
  std::vector<CreditString> strings;
  const base::ssize size = std::ssize(text);
  int line = 0;
  int column = 0;
  base::ssize cursor = 0;
  while (cursor < size) {
    char ch = text.at(base::ToSize(cursor++));
    switch (ch) {
      case '\r':
        line++;
        column = 0;
        break;

      case '\n':
        // The carriage return already ended the line.
        break;

      case ' ':
        column++;
        break;

      case '\t':
        column += kTabColumns;
        column -= column % kTabColumns;
        break;

      default: {
        // The start of a string. Find where it ends: `string_end` stops one
        // past the character that ended it, which is left in `ch`.
        const base::ssize string_begin = cursor - 1;
        base::ssize string_end = string_begin;
        const int start_column = column;
        int end_column = column;
        char previous_char = ch;
        bool string_ended = false;
        do {
          ch = text.at(base::ToSize(string_end++));
          switch (ch) {
            case '\t':
            case '\n':
            case '\r':
              string_ended = true;
              break;

            case ' ':
              if (previous_char == ' ') {
                string_ended = true;
              }
              end_column++;
              break;

            default:
              end_column++;
          }
          if (string_end >= size) {
            string_ended = true;
          }
          previous_char = ch;
        } while (!string_ended);

        // TODO: A string that runs into the end of the text is dropped, so
        // the last line of credits is lost unless a CR and LF follow it.
        if (string_end >= size) {
          return strings;
        }

        // Ended by two spaces: step back so that neither is in the string.
        //
        // TODO: This also fires for one space before a tab or line end. The
        // end column is then one too low, and after a CR the line counter
        // advances twice, because the cursor is left on the CR and the outer
        // loop counts it again.
        if (text.at(base::ToSize(string_end - 2)) == ' ') {
          string_end--;
          end_column -= 2;
        }

        // The string is everything before the character that ended it.
        const std::string_view string =
            text.substr(base::ToSize(string_begin),
                        base::ToSize(string_end - 1 - string_begin));
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

        // Carry on after the string. The terminator was consumed with it, so
        // do here what the outer loop would have done for it.
        cursor = string_end;
        column += static_cast<int>(string_end - string_begin);
        if (ch == '\r') {
          line++;
          column = 0;
        } else if (ch == '\t') {
          column += kTabColumns - 1;
          column -= column % kTabColumns;
        }
        break;
      }
    }
  }
  return strings;
}

}  // namespace ra
