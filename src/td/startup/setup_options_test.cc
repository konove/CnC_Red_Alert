// Tests the settings CONQUER.INI gives the game before its window opens, and
// that the command line wins over them.

#include <string_view>

#include "base/installed.h"
#include "engine/file/ini_file.h"
#include "engine/net/ipxaddr.h"
#include "gtest/gtest.h"
#include "td/assets/screen.h"
#include "td/data/ini_io.h"
#include "td/net/network.h"
#include "td/startup/game_state.h"
#include "td/startup/startup.h"
#include "td/startup/startup_options.h"

namespace td {

namespace {

// The settings land in the screen, the game state and the network, so each test
// installs its own and leaves the shared ones alone.
class SetupOptionsTest : public testing::Test {
 private:
  Screen screen_;
  base::Installed<Screen>::Scope screen_scope_{screen_};
  GameState game_state_;
  base::Installed<GameState>::Scope game_state_scope_{game_state_};
  Network network_;
  base::Installed<Network>::Scope network_scope_{network_};
};

TEST_F(SetupOptionsTest, ReadsTheScreenHeightAndTheCompatibilityFlag) {
  engine::file::IniFile ini;
  LoadIniText("[Options]\r\nResolution=1\r\nCompatibility=1\r\n", ini);
  ApplySetupOptions(ini, StartupOptions{});

  EXPECT_EQ(TheScreen().mode_height(), 480);
  EXPECT_TRUE(TheGameState().compatibility_v107());
}

TEST_F(SetupOptionsTest, TheCommandLineWinsOverTheFile) {
  engine::file::IniFile ini;
  LoadIniText("[Options]\r\nDestNet=1.2.3.4\r\n", ini);
  const engine::net::NetNumType net = {9, 8, 7, 6};
  const engine::net::NetNodeType node = {};
  StartupOptions options;
  options.bridge_net = engine::net::IPXAddressClass(net, node);
  ApplySetupOptions(ini, options);

  EXPECT_EQ(TheNetwork().is_bridge(), 1);
  engine::net::NetNumType bridge_net;
  engine::net::NetNodeType bridge_node;
  TheNetwork().bridge_net().Get_Address(bridge_net, bridge_node);
  EXPECT_EQ(bridge_net[0], 9);
  EXPECT_EQ(bridge_net[3], 6);
}

TEST_F(SetupOptionsTest, TheCommandLineScreenAndCompatibilityOverrideTheFile) {
  engine::file::IniFile ini;
  LoadIniText("[Options]\r\nResolution=0\r\nCompatibility=0\r\n", ini);
  StartupOptions options;
  options.tall_screen = true;
  options.compatibility_v107 = true;
  ApplySetupOptions(ini, options);

  EXPECT_EQ(TheScreen().mode_height(), 480);
  EXPECT_TRUE(TheGameState().compatibility_v107());
}

TEST(IntroFlagTest, AnIntroAlreadyMarkedPlayedIsNotSavedAgain) {
  for (const std::string_view text : {"No", "no", "NO"}) {
    engine::file::IniFile ini;
    ini.PutString("Intro", "PlayIntro", text);
    EXPECT_FALSE(MarkIntroPlayed(ini)) << text;
    EXPECT_EQ(ini.GetString("Intro", "PlayIntro", ""), text);
  }
}

TEST(IntroFlagTest, AnIntroNotYetMarkedPlayedIsSavedAsNo) {
  engine::file::IniFile yes;
  yes.PutString("Intro", "PlayIntro", "Yes");
  EXPECT_TRUE(MarkIntroPlayed(yes));
  EXPECT_EQ(yes.GetString("Intro", "PlayIntro", ""), "No");

  engine::file::IniFile absent;
  EXPECT_TRUE(MarkIntroPlayed(absent));
  EXPECT_EQ(absent.GetString("Intro", "PlayIntro", ""), "No");
}

TEST_F(SetupOptionsTest, ADestNetGivesTheBridgeNetwork) {
  engine::file::IniFile ini;
  LoadIniText("[Options]\r\nDestNet=1.2.3.4\r\n", ini);
  ApplySetupOptions(ini, StartupOptions{});

  EXPECT_EQ(TheNetwork().is_bridge(), 1);
  engine::net::NetNumType net;
  engine::net::NetNodeType node;
  TheNetwork().bridge_net().Get_Address(net, node);
  EXPECT_EQ(net[0], 1);
  EXPECT_EQ(net[1], 2);
  EXPECT_EQ(net[2], 3);
  EXPECT_EQ(net[3], 4);
  // The node is the broadcast address, so every machine behind the bridge
  // hears.
  for (const unsigned char byte : node) {
    EXPECT_EQ(byte, 0xff);
  }
}

// The shipped CONQUER.INI has "DestNet=" with nothing after it. IniFile does
// not keep such an entry, so it reads the same as a missing one: no bridge.
TEST_F(SetupOptionsTest, AnEmptyOrMissingDestNetGivesNoBridge) {
  engine::file::IniFile empty;
  LoadIniText("[Options]\r\nSocket=\r\nDestNet=\r\nResolution=0\r\n", empty);
  ApplySetupOptions(empty, StartupOptions{});
  EXPECT_EQ(TheNetwork().is_bridge(), 0);
  EXPECT_FALSE(empty.HasEntry("Options", "DestNet"));

  const engine::file::IniFile missing;
  ApplySetupOptions(missing, StartupOptions{});
  EXPECT_EQ(TheNetwork().is_bridge(), 0);
  EXPECT_EQ(TheScreen().mode_height(), Screen::kHeight);
  EXPECT_FALSE(TheGameState().compatibility_v107());
}

}  // namespace

}  // namespace td
