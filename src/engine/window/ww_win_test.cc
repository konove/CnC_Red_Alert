// Tests for PumpEvents: the event and pump handlers the game installs.

#include "engine/window/ww_win.h"

#include <SDL.h>
#include <SDL_events.h>

#include <cstdint>

#include "base/installed.h"
#include "engine/window/display.h"
#include "gtest/gtest.h"

namespace {

int events_seen = 0;
uint32_t last_event_type = 0;
int pumps_seen = 0;

void RecordEvent(const SDL_Event& event) {
  ++events_seen;
  last_event_type = event.type;
}

void RecordPump() { ++pumps_seen; }

void PushQuitEvent() {
  SDL_Event event{};
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

class EventPumpTest : public ::testing::Test {
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

  engine::window::Display display_;
  const base::Installed<engine::window::Display>::Scope display_scope_{
      display_};
};

TEST_F(EventPumpTest, CallsTheInstalledEventHandler) {
  SetEventHandler(&RecordEvent);
  PushQuitEvent();
  PumpEvents();
  EXPECT_EQ(events_seen, 1);
  EXPECT_EQ(last_event_type, static_cast<uint32_t>(SDL_QUIT));
}

TEST_F(EventPumpTest, DropsEventsWithNoHandlerInstalled) {
  PushQuitEvent();
  PumpEvents();
  EXPECT_EQ(events_seen, 0);
}

TEST_F(EventPumpTest, CallsTheInstalledPumpHandlerOncePerPass) {
  SetPumpHandler(&RecordPump);
  PumpEvents();
  PumpEvents();
  EXPECT_EQ(pumps_seen, 2);
}

TEST_F(EventPumpTest, DoesNothingExtraWithNoPumpHandlerInstalled) {
  PumpEvents();
  EXPECT_EQ(pumps_seen, 0);
}

}  // namespace
