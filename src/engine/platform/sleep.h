// File: Blocking the calling thread for a while, portably.
//
// Replaces the Win32 Sleep() the older sources call. Kept out of the win32/
// shims deliberately: this one is a real facility any code may want, not a
// Windows name being emulated.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_

#include "engine/platform/clock.h"

namespace engine::platform {

// Lets at least `milliseconds` pass on the clock in use (clock.h). On the
// machine's clock that blocks the calling thread: it wakes no earlier, but
// the scheduler decides how much later. Zero or less returns at once.
inline void SleepMs(const int milliseconds) {
  if (milliseconds > 0) {
    CurrentClock().Sleep(milliseconds);
  }
}

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_
