// File: ManualClock, a clock that stands in for real time and moves only when
// told to.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_

#include <cstdint>

#include "base/installed.h"

namespace engine::platform {

// While one exists, SystemTicks() and SystemMilliseconds() (timer.h) read it
// instead of the machine's clock, so everything that waits on real time -- a
// Timer<SystemTickSource>, a fade, a frame limiter -- waits on Advance()
// instead. A test, or a scripted run of the game, advances it a tick at a
// time; code that waits without anything advancing it waits forever.
//
// It installs itself for its own lifetime, and puts back the clock that was
// there before, if any.
//
// Example:
//   ManualClock clock;
//   const Timer<SystemTickSource> wait{3};
//   clock.Advance(3);
//   EXPECT_FALSE(wait.HasTimeLeft());
class ManualClock {
 public:
  ManualClock() = default;
  ~ManualClock() = default;

  ManualClock(const ManualClock&) = delete;
  ManualClock& operator=(const ManualClock&) = delete;
  ManualClock(ManualClock&&) = delete;
  ManualClock& operator=(ManualClock&&) = delete;

  // Moves the clock forward by `ticks` sixtieths of a second.
  void Advance(int64_t ticks) { ticks_ += ticks; }

  // The time in ticks, counted from 0 when the clock was made.
  [[nodiscard]] int64_t ticks() const { return ticks_; }
  // The same time in milliseconds, rounded down.
  [[nodiscard]] int64_t milliseconds() const;

 private:
  int64_t ticks_ = 0;
  base::Installed<ManualClock>::Scope scope_{*this};
};

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_
