// File: ManualClock, a clock that stands in for real time and moves only when
// told to.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_

#include <cstdint>

#include "engine/platform/clock.h"

namespace engine::platform {

// A Clock that moves only when told to. While one exists it is the clock in
// use (CurrentClock()), so everything that waits on real time -- a
// Countdown<SystemTickSource>, a fade, a frame limiter -- waits on Advance()
// instead. A test, or a scripted run of the game, advances it a tick at a
// time. Sleeping advances it too, instead of blocking: a loop that sleeps
// until a deadline reaches it at once. Code that waits without sleeping or
// anything else advancing the clock waits forever.
//
// It makes itself current for its own lifetime, and puts back the clock that
// was there before. Clocks nested this way must end in the reverse order they
// began.
//
// Example:
//   ManualClock clock;
//   const Countdown<SystemTickSource> wait{3};
//   clock.Advance(3);
//   EXPECT_FALSE(wait.HasTimeLeft());
class ManualClock final : public Clock {
 public:
  ManualClock();
  ~ManualClock() override;

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
  [[nodiscard]] int64_t Ticks() const override {
    return units_ / kUnitsPerTick;
  }
  // The same time in milliseconds, rounded down.
  [[nodiscard]] int64_t Milliseconds() const override {
    return units_ / kUnitsPerMillisecond;
  }

  // Moves the clock on by `milliseconds` and returns at once: the time has
  // passed as far as the caller can tell.
  void Sleep(const int milliseconds) override {
    AdvanceMilliseconds(milliseconds);
  }

 private:
  // The clock counts 1/3000 s, the largest unit both a tick (1/60 s) and a
  // millisecond are a whole number of, so neither kind of step drifts.
  static constexpr int64_t kUnitsPerTick = 50;
  static constexpr int64_t kUnitsPerMillisecond = 3;

  int64_t units_ = 0;
  // The clock this one replaced, put back when it goes.
  Clock* previous_;
};

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_MANUAL_CLOCK_H_
