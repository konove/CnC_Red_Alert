#include "ra/net/local_net_game.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "base/numeric.h"
#include "base/strings/safe_string.h"
#include "engine/net/ipxaddr.h"
#include "ra/ai/scenario.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/net/netdlg.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/net/wspudp.h"

namespace ra {

namespace {

// One of the two players of a local network game.
struct LocalPlayer {
  const char* name;
  const char* address;  // the loopback address the instance binds
  std::array<uint8_t, 4> address_bytes;
  HousesType house;
  PlayerColorType color;
};

constexpr std::array<LocalPlayer, 2> kPlayers = {{
    {.name = "One",
     .address = "127.0.0.1",
     .address_bytes = {127, 0, 0, 1},
     .house = HOUSE_GREECE,
     .color = PCOLOR_GOLD},
    {.name = "Two",
     .address = "127.0.0.2",
     .address_bytes = {127, 0, 0, 2},
     .house = HOUSE_USSR,
     .color = PCOLOR_LTBLUE},
}};

// Adds `player` to the session's player list.
void AddPlayer(const LocalPlayer& player) {
  PlayerNode* const who =
      TheSession().Players.Add(std::make_unique<PlayerNode>());
  base::SafeCopy(who->Name, player.name);
  // The UDP transport keeps a player's IP address in the first four bytes of
  // the IPX node number, which is where it puts a sender's.
  const engine::net::NetNumType net{};
  engine::net::NetNodeType node{};
  std::ranges::copy(player.address_bytes, std::begin(node));
  who->Address.Set_Address(net, node);
  who->House = player.house;
  who->Color = player.color;
}

}  // namespace

bool StartLocalNetGame(const int me, const std::string& scenario_file,
                       const uint16_t seed) {
  const LocalPlayer& mine = kPlayers.at(base::ToSize(me - 1));
  const LocalPlayer& theirs = kPlayers.at(base::ToSize(2 - me));

  TheSession().Type = GAME_IPX;
  auto transport = std::make_unique<UDPInterfaceClass>();
  transport->Set_Bind_Address(mine.address);
  transport->Set_Broadcast_Address(theirs.address);
  TheNetwork().packet_transport() = std::move(transport);
  if (!Init_Network()) {
    TheNetwork().packet_transport().reset();
    TheSession().Type = GAME_NORMAL;
    return false;
  }

  // The local player comes first in the list, as in the lobby; the houses
  // are handed out by color, so both instances agree on who is which.
  TheSession().Players.Clear();
  AddPlayer(mine);
  AddPlayer(theirs);
  TheSession().NumPlayers = 2;

  TheSession().Options.Bases = 1;
  TheSession().Options.Credits = 10000;
  TheSession().Options.Tiberium = 1;
  TheSession().Options.Goodies = 0;
  TheSession().Options.Ghosts = 0;
  TheSession().Options.UnitCount = 5;
  TheSession().Options.AIPlayers = 0;
  TheWorld().build_level() = 10;
  TheScenario().Difficulty = DifficultyLevel::kNormal;
  TheScenario().CDifficulty = DifficultyLevel::kNormal;
  TheScenario().Set_Scenario_Name(scenario_file.c_str());

  // What the host works out from the lobby's pings and sends with its "go":
  // how many frames ahead orders are scheduled. Both ends are on one
  // machine, so the smallest the protocol allows will do.
  TheSession().MaxAhead =
      static_cast<int>(TheSession().FrameSendRate) * 2;
  TheNetwork().ipx().Set_Timing(25, -1, 1000);

  // The host's seed, which the lobby hands the guests.
  TheWorld().seed() = seed != 0 ? seed : 1;
  return true;
}

}  // namespace ra
