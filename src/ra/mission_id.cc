#include "ra/mission_id.h"

#include <charconv>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"

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
  std::from_chars(number.begin(), number.end(), scenario_number);
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
  if (!absl::ascii_isdigit(static_cast<unsigned char>(rest.at(0)))) {
    return true;
  }
  return rest.size() >= 3 &&
         absl::ascii_isdigit(static_cast<unsigned char>(rest.at(1))) &&
         !absl::ascii_isdigit(static_cast<unsigned char>(rest.at(2)));
}

namespace {

// Positions of the fields in "SC<side><NN><dir><variant>.INI".
constexpr std::string_view::size_type kNumberPos = 3;
constexpr std::string_view::size_type kNumberLength = 2;
constexpr std::string_view::size_type kVariantPos = 6;

}  // namespace

std::string MissionWithNumber(const std::string_view file_name,
                              const int scenario) {
  std::string result(file_name);
  if (result.size() >= kNumberPos + kNumberLength) {
    result.replace(kNumberPos, kNumberLength,
                   absl::StrFormat("%02d", scenario));
  }
  return result;
}

std::string MissionWithVariant(const std::string_view file_name,
                               const char variant) {
  std::string result(file_name);
  if (result.size() > kVariantPos) {
    result.at(kVariantPos) = variant;
  }
  return result;
}
