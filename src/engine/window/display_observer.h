// File: DisplayObserver, a watcher of the frames a Display ends.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_OBSERVER_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_OBSERVER_H_

namespace engine::window {

// Told what a Display does as the game's loops run, so that something outside
// those loops can act between their turns: a scripted player presses its keys
// and moves the clock here. Display::set_observer() attaches one, and the
// Display keeps only a pointer, so the observer must outlive its attachment.
//
// Example:
//   class FrameCounter final : public engine::window::DisplayObserver {
//    public:
//     void OnFrameEnd() override { ++frames; }
//     int frames = 0;
//   };
class DisplayObserver {
 public:
  DisplayObserver() = default;
  virtual ~DisplayObserver();

  DisplayObserver(const DisplayObserver&) = delete;
  DisplayObserver& operator=(const DisplayObserver&) = delete;
  DisplayObserver(DisplayObserver&&) = delete;
  DisplayObserver& operator=(DisplayObserver&&) = delete;

  // Called each time the Display ends a frame, after presenting it and before
  // pumping events, so that input queued here is seen by the loop that ended
  // the frame.
  virtual void OnFrameEnd() = 0;

  // Called at the start of every Display::PumpEvents(), including the one
  // that follows OnFrameEnd(). A loop that polls the keyboard without ever
  // presenting shows up here and nowhere else. The default does nothing.
  virtual void OnPump() {}
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_OBSERVER_H_
