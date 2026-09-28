// Tests for WindowEvents: where each kind of SDL event goes.

#include "engine/window/window_events.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <SDL_video.h>

#include <cstdint>
#include <vector>

#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// Records the calls WindowEvents leaves to the game.
class RecordingWindowEvents final : public WindowEvents {
 public:
  RecordingWindowEvents(KeyBuffer& keyboard, HardwareCursor& mouse)
      : WindowEvents(keyboard, mouse) {}

  std::vector<bool> focus_changes;
  int quits = 0;

 private:
  void FocusChanged(const bool in_focus) override {
    focus_changes.push_back(in_focus);
  }
  void Quit() override { ++quits; }
};

SDL_Event MakeWindowEvent(const uint8_t window_event) {
  SDL_Event event{};
  event.type = SDL_WINDOWEVENT;
  event.window.event = window_event;
  return event;
}

class WindowEventsTest : public ::testing::Test {
 protected:
  KeyBuffer keyboard_;
  HardwareCursor mouse_{48, 48};
  RecordingWindowEvents events_{keyboard_, mouse_};
};

TEST_F(WindowEventsTest, QueuesKeysInTheKeyBuffer) {
  SDL_Event event{};
  event.type = SDL_KEYDOWN;
  event.key.keysym.scancode = SDL_SCANCODE_A;
  events_.HandleEvent(event);

  EXPECT_TRUE(keyboard_.Peek());
}

TEST_F(WindowEventsTest, QueuesClicksInTheKeyBuffer) {
  SDL_Event event{};
  event.button.type = SDL_MOUSEBUTTONDOWN;
  event.button.button = SDL_BUTTON_LEFT;
  event.button.state = SDL_PRESSED;
  events_.HandleEvent(event);

  EXPECT_TRUE(keyboard_.Peek());
}

TEST_F(WindowEventsTest, RecordsMouseMotionOnTheCursor) {
  SDL_Event event{};
  event.type = SDL_MOUSEMOTION;
  event.motion.x = 10;
  event.motion.y = 20;
  events_.HandleEvent(event);

  EXPECT_EQ(mouse_.x(), 10);
  EXPECT_EQ(mouse_.y(), 20);
}

TEST_F(WindowEventsTest, ReportsFocusChanges) {
  events_.HandleEvent(MakeWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST));
  events_.HandleEvent(MakeWindowEvent(SDL_WINDOWEVENT_FOCUS_GAINED));

  EXPECT_EQ(events_.focus_changes, (std::vector<bool>{false, true}));
}

TEST_F(WindowEventsTest, ReportsTheWindowClosing) {
  SDL_Event event{};
  event.type = SDL_QUIT;
  events_.HandleEvent(event);

  EXPECT_EQ(events_.quits, 1);
}

TEST_F(WindowEventsTest, IgnoresOtherWindowEvents) {
  events_.HandleEvent(MakeWindowEvent(SDL_WINDOWEVENT_MOVED));

  EXPECT_TRUE(events_.focus_changes.empty());
  EXPECT_EQ(events_.quits, 0);
}

}  // namespace
}  // namespace engine::window
