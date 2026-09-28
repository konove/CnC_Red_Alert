// File: WindowEvents, the event routing both games share.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_EVENTS_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_EVENTS_H_

#include "absl/base/attributes.h"
#include "engine/window/display_client.h"

union SDL_Event;

namespace engine::window {

class HardwareCursor;
class KeyBuffer;

// The DisplayClient of a game window: queues keys and clicks in the
// KeyBuffer, records mouse motion on the HardwareCursor and rescales it when
// the window is resized, and leaves focus changes and the window closing to
// the game, which subclasses it for those.
//
// It keeps pointers to the two devices, so they must outlive it.
//
// Example:
//   class GameWindowEvents final : public engine::window::WindowEvents {
//    public:
//     using WindowEvents::WindowEvents;
//
//    private:
//     void FocusChanged(bool in_focus) override { ... }
//     void Quit() override { ... }
//   };
class WindowEvents : public DisplayClient {
 public:
  WindowEvents(KeyBuffer& keyboard ABSL_ATTRIBUTE_LIFETIME_BOUND,
               HardwareCursor& mouse ABSL_ATTRIBUTE_LIFETIME_BOUND);

  void HandleEvent(const SDL_Event& event) final;

 private:
  // Called when the window gains (`in_focus` true) or loses the input focus.
  virtual void FocusChanged(bool in_focus) = 0;
  // Called when the player closes the window.
  virtual void Quit() = 0;

  KeyBuffer* keyboard_;
  HardwareCursor* mouse_;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_EVENTS_H_
