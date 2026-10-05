#include "engine/platform/timer.h"

#include <chrono>
#include <cstdint>
#include <ratio>

#include "base/installed.h"
#include "engine/platform/manual_clock.h"

namespace engine::platform {

int64_t SystemMilliseconds() {
  if (base::Installed<ManualClock>::IsInstalled()) {
    return base::Installed<ManualClock>::Get().milliseconds();
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

int64_t SystemTicks() {
  if (base::Installed<ManualClock>::IsInstalled()) {
    return base::Installed<ManualClock>::Get().ticks();
  }
  // Computed from the clock rather than counted by a timer thread, so there is
  // nothing to start or stop, and the rate is exactly 60 Hz: the SDL timer this
  // replaced fired every 1000 / 60 = 16 ms, which is 62.5 Hz.
  using Ticks =
      std::chrono::duration<int64_t, std::ratio<1, kSystemTicksPerSecond>>;
  return std::chrono::duration_cast<Ticks>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace engine::platform
