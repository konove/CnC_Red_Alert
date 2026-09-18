// Reads and edits the fields packed into scenario file names.
//
// A campaign scenario is named "SC<side><NN><dir><variant>.INI", e.g.
// "SCG05EA.INI": NN is the two-digit scenario number and the variant letter
// (A, B, ...) picks one of the alternative maps for that scenario.
//
// Multiplayer scenarios are named "SCM<id><house>.INI". Scenarios numbered
// above 24 belong to Counterstrike; scenarios with an alphabetical name
// (e.g. "SCMJGEA.INI") belong to Aftermath.

#ifndef CNC_RED_ALERT_RA_MISSION_ID_H_
#define CNC_RED_ALERT_RA_MISSION_ID_H_

#include <string>
#include <string_view>

// Returns true if `file_name` names a Counterstrike multiplayer scenario:
// an uppercase "SCM" followed by a two- or three-digit scenario number greater
// than 24, e.g. "SCM25EA.INI" or "SCM100.INI". The prefix match is
// case-sensitive here but not in IsMissionAftermath(); both preserve the
// original game's behavior.
bool IsMissionCounterstrike(std::string_view file_name);

// Returns true if `file_name` names an Aftermath multiplayer scenario: "scm"
// (case-insensitive) followed by either a non-digit ("SCMJGEA.INI") or two
// digits and a non-digit. Note that a name like "SCM25EA.INI" matches both
// this and IsMissionCounterstrike(); callers that need an exclusive
// classification check IsMissionCounterstrike() first.
bool IsMissionAftermath(std::string_view file_name);

// Returns the campaign scenario name `file_name` with its number replaced by
// `scenario` (0-99): ("SCG05EA.INI", 6) gives "SCG06EA.INI". Returns
// `file_name` unchanged if it is too short to hold a number.
std::string MissionWithNumber(std::string_view file_name, int scenario);

// Returns the campaign scenario name `file_name` with its variant letter
// replaced by `variant`: ("SCG05EA.INI", 'B') gives "SCG05EB.INI". Returns
// `file_name` unchanged if it is too short to hold a variant.
std::string MissionWithVariant(std::string_view file_name, char variant);

#endif  // CNC_RED_ALERT_RA_MISSION_ID_H_
