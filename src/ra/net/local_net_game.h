// File: StartLocalNetGame(), a two-player network game without the lobby.

#ifndef CNC_RED_ALERT_RA_NET_LOCAL_NET_GAME_H_
#define CNC_RED_ALERT_RA_NET_LOCAL_NET_GAME_H_

#include <cstdint>
#include <string>

namespace ra {

// Sets the session up as the network lobby would have for a two-player game
// of the multiplayer map `scenario_file` ("SCM01EA.INI") between two
// instances of the game on this machine, so that Select_Game() can go
// straight on to the scenario. `me` is 1 or 2: player 1 binds the loopback
// address 127.0.0.1 and plays Greece in gold, player 2 binds 127.0.0.2 and
// plays the USSR in light blue; each sends to the other's address. Both
// instances must be given the same map and `seed`, and be started within a
// few seconds of each other.
//
// For the -NETGAME switch: an unattended check that two instances exchange
// their orders and stay in step, which tools/net_check.py runs. Returns
// false if the network could not be opened.
bool StartLocalNetGame(int me, const std::string& scenario_file,
                       uint16_t seed);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_LOCAL_NET_GAME_H_
