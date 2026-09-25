// Tests for SDL_Event_Loop: the event and pump handlers the game installs.

#include "engine/window/ww_win.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_stdinc.h>

#include "engine/base/installed.h"
#include "engine/window/display.h"
#include "gtest/gtest.h"

namespace {

int events_seen = 0;
Uint32 last_event_type = 0;
int pumps_seen = 0;

void RecordEvent(SDL_Event* event) {
  ++events_seen;
  last_event_type = event->type;
}

void RecordPump() { ++pumps_seen; }

class WwWinTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SDL_Init(SDL_INIT_EVENTS);
    events_seen = 0;
    last_event_type = 0;
    pumps_seen = 0;
    SetEventHandler(nullptr);
    SetPumpHandler(nullptr);
  }

  void TearDown() override {
    SetEventHandler(nullptr);
    SetPumpHandler(nullptr);
  }

  Display display_;
  const base::Installed<Display>::Scope display_scope_{display_};
};

TEST_F(WwWinTest, EventLoopCallsTheInstalledHandler) {
  SetEventHandler(&RecordEvent);
  SDL_Send_Quit();
  SDL_Event_Loop();
  EXPECT_EQ(events_seen, 1);
  EXPECT_EQ(last_event_type, static_cast<Uint32>(SDL_QUIT));
}

TEST_F(WwWinTest, EventLoopDropsEventsWithNoHandlerInstalled) {
  SDL_Send_Quit();
  SDL_Event_Loop();
  EXPECT_EQ(events_seen, 0);
}

TEST_F(WwWinTest, EventLoopCallsTheInstalledPumpHandlerOncePerPass) {
  SetPumpHandler(&RecordPump);
  SDL_Event_Loop();
  SDL_Event_Loop();
  EXPECT_EQ(pumps_seen, 2);
}

TEST_F(WwWinTest, EventLoopDoesNothingExtraWithNoPumpHandlerInstalled) {
  SDL_Event_Loop();
  EXPECT_EQ(pumps_seen, 0);
}

}  // namespace
