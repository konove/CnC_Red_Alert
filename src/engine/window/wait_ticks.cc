#include "engine/window/wait_ticks.h"

#include "engine/platform/timer.h"
#include "engine/window/display.h"

namespace engine::window {

void WaitTicks(int ticks) {
  const auto target = g_tick_timer->TickCount() + ticks;

  while (g_tick_timer->TickCount() < target) {
    TheDisplay().EndFrame();
  }
}

}  // namespace engine::window
