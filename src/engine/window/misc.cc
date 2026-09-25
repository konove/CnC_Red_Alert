#include "engine/window/misc.h"

#include "engine/platform/timer.h"
#include "engine/window/display.h"

void WaitTicks(int ticks) {
  const auto target = g_tick_timer->TickCount() + ticks;

  while (g_tick_timer->TickCount() < target) {
    TheDisplay().EndFrame();
  }
}
