// File: Blocking the calling thread for a while, portably.
//
// Replaces the Win32 Sleep() the older sources call. Kept out of the win32/
// shims deliberately: this one is a real facility any code may want, not a
// Windows name being emulated.

#ifndef CNC_RED_ALERT_PORT_SLEEP_H_
#define CNC_RED_ALERT_PORT_SLEEP_H_

#include <chrono>
#include <thread>

namespace port {

// Blocks the calling thread for at least `milliseconds`. The thread wakes no
// earlier, but the scheduler decides how much later.
inline void SleepMs(int milliseconds) {
  if (milliseconds <= 0) {
    return;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_SLEEP_H_
