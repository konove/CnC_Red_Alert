#include "ra/mission_id.h"

#include <charconv>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"

bool IsMissionCounterstrike(const std::string_view file_name) {
  // Case-sensitive, unlike IsMissionAftermath(): the original sscanf("SCM%03d")
  // parse only matched an uppercase prefix, and scenario lists are built from
  // the uppercase names stored in the mission packet files.
  if (!absl::StartsWith(file_name, "SCM")) {
    return false;
  }
  // The scenario number is at most three digits; from_chars() stops at the
  // first non-digit (e.g. the house letters in "SCM25EA.INI").
  const std::string_view number = file_name.substr(3, 3);
  int scenario_number = 0;
  std::from_chars(number.data(), number.data() + number.size(),
                  scenario_number);
  return scenario_number > 24;
}

bool IsMissionAftermath(const std::string_view file_name) {
  if (!absl::StartsWithIgnoreCase(file_name, "scm")) {
    return false;
  }
  const std::string_view rest = file_name.substr(3);
  if (rest.empty()) {
    return false;
  }
  if (!absl::ascii_isdigit(rest[0])) {
    return true;
  }
  return rest.size() >= 3 && absl::ascii_isdigit(rest[1]) &&
         !absl::ascii_isdigit(rest[2]);
}
