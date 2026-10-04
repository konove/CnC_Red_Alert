// The fixture shared by the tests that run against a real Game with no game
// data.

#ifndef CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_
#define CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_

#include "gtest/gtest.h"
#include "ra/startup/game.h"

namespace ra {

// A Game with the house and vehicle types, room for two houses and four
// vehicles, and an empty map. A test fixture derives from it and builds the
// other types, heaps and houses it needs in its own constructor.
//
// Example:
//   class CarryoverTest : public GameFixture {
//    protected:
//     CarryoverTest() { base::NewPooled<HouseClass>(HOUSE_GREECE); }
//   };
class GameFixture : public ::testing::Test {
 protected:
  GameFixture();

  // Empties the display layers, which are static and outlive the Game.
  void TearDown() override;

 private:
  Game game_;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_
