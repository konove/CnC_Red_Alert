#include "engine/window/misc.h"

#include "absl/strings/str_format.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"

bool Set_Video_Mode(int w, int h, int /*bits_per_pixel*/) {
  absl::PrintF("%s\n", __func__);
  return TheDisplay().SetVideoMode(w, h);
}

void WaitTicks(int ticks) {
  const auto target = g_tick_timer->TickCount() + ticks;

  while (g_tick_timer->TickCount() < target) {
    TheDisplay().EndFrame();
  }
}
