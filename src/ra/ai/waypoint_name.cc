#include "ra/ai/waypoint_name.h"

#include <string>
#include <string_view>

#include "absl/strings/ascii.h"

namespace ra {

namespace {

constexpr int kLetterCount = 26;

// Returns the position in the alphabet of the letter `letter`, 0 for 'A' or
// 'a'.
int LetterIndex(const char letter) {
  return absl::ascii_toupper(static_cast<unsigned char>(letter)) - 'A';
}

}  // namespace

std::string WaypointLetters(const int waypoint) {
  if (waypoint < kLetterCount) {
    return {static_cast<char>('A' + waypoint)};
  }
  // The first letter counts whole alphabets, starting at 'A' for one.
  return {static_cast<char>('A' + (waypoint / kLetterCount) - 1),
          static_cast<char>('A' + (waypoint % kLetterCount))};
}

int WaypointFromLetters(const std::string_view letters) {
  if (letters.empty()) {
    return -1;
  }
  int waypoint = LetterIndex(letters.front());
  if (letters.size() > 1) {
    waypoint = ((waypoint + 1) * kLetterCount) + LetterIndex(letters.at(1));
  }
  return waypoint;
}

}  // namespace ra
