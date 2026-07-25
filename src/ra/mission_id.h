// Classifies multiplayer scenario file names by expansion pack.
//
// Multiplayer scenarios are named "SCM<id><house>.INI". Scenarios numbered
// above 24 belong to Counterstrike; scenarios with an alphabetical name
// (e.g. "SCMJGEA.INI") belong to Aftermath.

#ifndef CNC_RED_ALERT_RA_MISSION_ID_H_
#define CNC_RED_ALERT_RA_MISSION_ID_H_

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

#endif  // CNC_RED_ALERT_RA_MISSION_ID_H_
