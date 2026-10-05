// File: Clock, the source of the game's real time, and CurrentClock(), the
// one in use.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_CLOCK_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_CLOCK_H_

#include <cstdint>

namespace engine::platform {

// Where SystemTicks(), SystemMilliseconds() (timer.h) and SleepMs()
// (sleep.h) get their time from. The game runs on the machine's monotonic
// clock; a test, or a scripted run of the game, puts a ManualClock
// (manual_clock.h) in its place, which moves only when told to.
//
// Example:
//   const int64_t start = CurrentClock().Milliseconds();
//   CurrentClock().Sleep(20);
class Clock {
 public:
  virtual ~Clock();

  Clock(const Clock&) = delete;
  Clock& operator=(const Clock&) = delete;
  Clock(Clock&&) = delete;
  Clock& operator=(Clock&&) = delete;

  // The time in ticks of 1/60 second and in milliseconds. Neither ever goes
  // back, and the zero point means nothing on its own: callers compare two
  // readings.
  [[nodiscard]] virtual int64_t Ticks() const = 0;
  [[nodiscard]] virtual int64_t Milliseconds() const = 0;

  // Lets `milliseconds`, which is positive, pass: by blocking the calling
  // thread, or by whatever passes for time on this clock.
  virtual void Sleep(int milliseconds) = 0;

 protected:
  Clock() = default;
};

// Returns the clock in use: the machine's, unless another has been made
// current.
Clock& CurrentClock();

// Makes `clock` the clock in use and returns the one it replaces, which the
// caller puts back the same way when `clock` is done. `clock` must stay alive
// while it is current. Main thread only.
Clock& ExchangeCurrentClock(Clock& clock);

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_CLOCK_H_
