// Reinforcements: bringing a new team, a single unit or an air strike onto the
// map while a scenario runs.

#ifndef CNC_RED_ALERT_TD_AI_REINF_H_
#define CNC_RED_ALERT_TD_AI_REINF_H_

#include "td/ai/teamtype.h"
#include "td/core/defines.h"
#include "td/objects/house.h"
#include "td/objects/type.h"

namespace td {

// Creates the members of `team_type` and brings them into play,
// announcing them when they belong to the player's house. They follow the
// type's missions as a team; a type with no missions gets no team, and its
// members are sent onto the map on their own.
//
// How the group arrives depends on what it holds: with an aircraft, by air
// from the house's map edge; with a hovercraft, over the beach; a gunboat,
// along the shipping lane; anything else, over the house's map edge. Any other
// member is loaded into the first transport.
//
// Returns false when `team_type` is null or has no members, when neither the
// team nor any member could be created (the object limit), and when the gunboat
// or the hovercraft could not be placed; in the campaign also when the last
// aircraft could not be. Members that find no room at the map edge are dropped
// and the result is still true.
bool DeliverReinforcements(const TeamTypeClass* team_type);

// Brings on one object of `type` for `house` that no trigger planned, such as
// a replacement harvester or a unit bought at the airstrip, by building a
// one-off team type for it and delivering that. Returns whether it was
// delivered; false when `house` or `type` is null or no team type is free.
//
// `transport_type` is the transport to bring it in, or nullptr for none.
// A vehicle or soldier without one drives on from the house's map edge to the
// reinforcement waypoint, except that at a south edge with no way in it gets a
// hovercraft.
//
// `mission` and `mission_argument` are the team mission to carry out on arrival
// and its argument, usually a waypoint. TMISSION_NONE picks one: moving to the
// reinforcement waypoint, or unloading there when there is a transport.
bool DeliverSpecialReinforcement(const HouseClass* house,
                                 const TechnoTypeClass* type,
                                 const TechnoTypeClass* transport_type,
                                 TeamMissionType mission = TMISSION_NONE,
                                 int mission_argument = 0);

// Launches an air strike: creates up to `count` aircraft of `aircraft_type`
// for `house` at its map edge (north when it has none of the four), each a
// loaner that leaves the map when done, and gives each `mission`, `target` and
// `destination`; a default `Target{}` leaves that one unassigned.
//
// Stops at the first aircraft that cannot be created or placed. Returns the
// number placed.
int LaunchAirStrike(HouseClass* house, AircraftType aircraft_type, int count,
                    MissionType mission, Target target, Target destination);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_AI_REINF_H_
