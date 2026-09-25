#include "engine/window/misc.h"

#include "absl/strings/str_format.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"

bool Set_Video_Mode(int w, int h, int /*bits_per_pixel*/) {
  absl::PrintF("%s\n", __func__);
  return TheDisplay().SetVideoMode(w, h);
}

void Delay(int duration) {
  const auto target = g_tick_timer->TickCount() + duration;

  while (g_tick_timer->TickCount() < target) {
    TheDisplay().EndFrame();
  }
}
