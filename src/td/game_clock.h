// File: GameClock, the frame counter the simulation runs on.

#ifndef CNC_RED_ALERT_TD_GAME_CLOCK_H_
#define CNC_RED_ALERT_TD_GAME_CLOCK_H_

#include <cstdint>

#include "engine/base/installed.h"

// The number of game frames since the scenario started. Every frame-based
// timer measures against it, the simulation advances it once per logic
// frame, and a saved game stores it, so it is the one piece of state the
// whole simulation is anchored to. Game owns the one GameClock, declared
// ahead of everything that holds such a timer.
//
// Example:
//   if (CurrentFrame() > deadline) { ... }
class GameClock {
 public:
  GameClock() = default;
  ~GameClock() = default;

  GameClock(const GameClock&) = delete;
  GameClock& operator=(const GameClock&) = delete;
  GameClock(GameClock&&) = delete;
  GameClock& operator=(GameClock&&) = delete;

  [[nodiscard]] int64_t frame() const { return frame_; }

  // Sets the frame number. The scenario loader zeroes it, and reading a
  // saved game restores what was saved.
  void set_frame(int64_t frame) { frame_ = frame; }

  // Moves on to the next frame. The main loop calls this once per logic
  // frame, and nothing else may.
  void Advance() { ++frame_; }

 private:
  int64_t frame_ = 0;
};

// Returns the GameClock that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline GameClock& TheGameClock() { return base::Installed<GameClock>::Get(); }

// Returns the current game frame, or 0 before a GameClock is installed.
// Timers inside statically constructed objects read it while they are being
// built, which is before Game exists; the frame counter was a global that
// started at zero, so they saw zero then too.
inline int64_t CurrentFrame() {
  return base::Installed<GameClock>::IsInstalled() ? TheGameClock().frame() : 0;
}

#endif  // CNC_RED_ALERT_TD_GAME_CLOCK_H_
