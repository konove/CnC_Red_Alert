// Reinforcements: bringing a new team, or an air strike, onto the map while a
// scenario runs.

#ifndef CNC_RED_ALERT_RA_AI_REINF_H_
#define CNC_RED_ALERT_RA_AI_REINF_H_

#include "ra/ai/teamtype.h"
#include "ra/core/defines.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"

namespace ra {

// Creates a team of type `team_type` and brings it into play, announcing it
// when it belongs to the player's house. The members follow the team's
// missions; a team type with none is given one here, to attack its origin
// waypoint, and keeps it.
//
// A group of only infantry whose origin waypoint is on or next to a building or
// a transport vehicle comes out of that. Any other group enters from the map
// edge nearest the origin waypoint, or from the house's edge when the type has
// no origin, with the cargo already loaded when the type includes a transport.
//
// Returns false when the type has no members, when none could be created (the
// object limit), or when none of the infantry could leave the building.
// Members that find no room at the map edge are dropped and the result is still
// true.
bool DeliverReinforcements(TeamTypeClass* team_type);

// Launches an air strike: creates up to `count` aircraft of `aircraft_type`
// for `house` at its map edge (north when it has none), each a loaner that
// leaves the map when done, and gives each `mission`, `target` and
// `destination`; a default `Target{}` leaves that one unassigned.
//
// Stops at the first aircraft that cannot be created or placed. Returns the
// number placed.
int LaunchAirStrike(HouseClass* house, AircraftType aircraft_type, int count,
                    MissionType mission, Target target, Target destination);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_REINF_H_
