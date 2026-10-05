// File: Blocking the calling thread for a while, portably.
//
// Replaces the Win32 Sleep() the older sources call. Kept out of the win32/
// shims deliberately: this one is a real facility any code may want, not a
// Windows name being emulated.

#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_

#include <chrono>
#include <thread>

#include "base/installed.h"
#include "engine/platform/manual_clock.h"

namespace engine::platform {

// Blocks the calling thread for at least `milliseconds`. The thread wakes no
// earlier, but the scheduler decides how much later.
//
// While a ManualClock exists, moves that clock on by `milliseconds` and
// returns at once: the time has passed as far as the caller can tell.
inline void SleepMs(int milliseconds) {
  if (milliseconds <= 0) {
    return;
  }
  if (base::Installed<ManualClock>::IsInstalled()) {
    base::Installed<ManualClock>::Get().AdvanceMilliseconds(milliseconds);
    return;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

}  // namespace engine::platform

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_SLEEP_H_
