// File: DisplayClient, what the game does with the events Display's loop
// polls.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_CLIENT_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_CLIENT_H_

union SDL_Event;

namespace engine::window {

// The game's side of Display::PumpEvents(). The game builds one and hands it
// to its Display, which keeps a pointer to it, so it must outlive the
// Display. Kept apart from display.h so that the games can declare their
// client without pulling in Display.
//
// Example:
//   class WindowEvents final : public engine::window::DisplayClient {
//    public:
//     void HandleEvent(const SDL_Event& event) override { ... }
//   };
class DisplayClient {
 public:
  DisplayClient() = default;
  virtual ~DisplayClient();

  DisplayClient(const DisplayClient&) = delete;
  DisplayClient& operator=(const DisplayClient&) = delete;
  DisplayClient(DisplayClient&&) = delete;
  DisplayClient& operator=(DisplayClient&&) = delete;

  // Called once for each event PumpEvents() polls other than Display's own
  // redraw events, which the loop consumes itself.
  virtual void HandleEvent(const SDL_Event& event) = 0;

  // Called once per PumpEvents() pass, before events are polled. Red Alert
  // pumps its sockets here (Socket_Select); the default does nothing.
  virtual void Pump() {}
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_CLIENT_H_
