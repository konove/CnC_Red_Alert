// Implements the machine's clock, the choice of the clock in use, and
// ManualClock.

#include "engine/platform/clock.h"

#include <chrono>
#include <cstdint>
#include <ratio>
#include <thread>
#include <utility>

#include "absl/base/no_destructor.h"
#include "absl/log/check.h"
#include "engine/platform/manual_clock.h"
#include "engine/platform/timer.h"

namespace engine::platform {

namespace {

// Real time, from the machine's monotonic clock: steady_clock, not the wall
// clock, which can be set back.
class SystemClock final : public Clock {
 public:
  [[nodiscard]] int64_t Ticks() const override {
    // Computed from the clock rather than counted by a timer thread, so there
    // is nothing to start or stop, and the rate is exactly 60 Hz: the SDL
    // timer this replaced fired every 1000 / 60 = 16 ms, which is 62.5 Hz.
    using TickDuration =
        std::chrono::duration<int64_t, std::ratio<1, kSystemTicksPerSecond>>;
    return std::chrono::duration_cast<TickDuration>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

  [[nodiscard]] int64_t Milliseconds() const override {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

  void Sleep(const int milliseconds) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }
};

// The clock in use, which starts as the machine's.
Clock*& CurrentClockSlot() {
  static absl::NoDestructor<SystemClock> system_clock;
  static Clock* current = system_clock.get();
  return current;
}

}  // namespace

Clock::~Clock() = default;

Clock& CurrentClock() { return *CurrentClockSlot(); }

Clock& ExchangeCurrentClock(Clock& clock) {
  return *std::exchange(CurrentClockSlot(), &clock);
}

ManualClock::ManualClock() : previous_(&ExchangeCurrentClock(*this)) {}

ManualClock::~ManualClock() {
  CHECK(&CurrentClock() == this) << "manual clocks ended out of order";
  ExchangeCurrentClock(*previous_);
}

}  // namespace engine::platform
