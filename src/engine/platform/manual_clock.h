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
// time. SleepMs() (sleep.h) advances it too, instead of sleeping: a loop that
// sleeps until a deadline reaches it at once. Code that waits without
// sleeping or anything else advancing the clock waits forever.
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
  void Advance(int64_t ticks) { units_ += ticks * kUnitsPerTick; }
  // Moves the clock forward by `milliseconds`.
  void AdvanceMilliseconds(int64_t milliseconds) {
    units_ += milliseconds * kUnitsPerMillisecond;
  }

  // The time in ticks, counted from 0 when the clock was made and rounded
  // down.
  [[nodiscard]] int64_t ticks() const { return units_ / kUnitsPerTick; }
  // The same time in milliseconds, rounded down.
  [[nodiscard]] int64_t milliseconds() const {
    return units_ / kUnitsPerMillisecond;
  }

 private:
  // The clock counts 1/3000 s, the largest unit both a tick (1/60 s) and a
  // millisecond are a whole number of, so neither kind of step drifts.
  static constexpr int64_t kUnitsPerTick = 50;
  static constexpr int64_t kUnitsPerMillisecond = 3;

  int64_t units_ = 0;
  base::Installed<ManualClock>::Scope scope_{*this};
};

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_
