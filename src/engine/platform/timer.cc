#include "engine/platform/timer.h"

#include <cstdint>

#include "engine/platform/clock.h"

namespace engine::platform {

int64_t SystemMilliseconds() { return CurrentClock().Milliseconds(); }

int64_t SystemTicks() { return CurrentClock().Ticks(); }

}  // namespace engine::platform
