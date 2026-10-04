// The letters the scenario editor shows for a waypoint number.

#ifndef CNC_RED_ALERT_RA_AI_WAYPOINT_NAME_H_
#define CNC_RED_ALERT_RA_AI_WAYPOINT_NAME_H_

#include <string>
#include <string_view>

namespace ra {

// Returns the editor's name for waypoint `waypoint`: one letter, "A" to "Z",
// for 0 to 25, and two letters from there on, "AA" for 26 to "ZZ" for 701.
// Numbers outside that range give characters that are not letters.
std::string WaypointLetters(int waypoint);

// Returns the waypoint that `letters` names, ignoring case: the inverse of
// WaypointLetters(). Only the first two characters are read. Returns -1, no
// waypoint, when `letters` is empty.
int WaypointFromLetters(std::string_view letters);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_WAYPOINT_NAME_H_
