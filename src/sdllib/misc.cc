#include "sdllib/misc.h"

#include "absl/strings/str_format.h"
#include "engine/platform/timer.h"
#include "sdllib/display.h"

void (*Misc_Focus_Loss_Function)();
void (*Misc_Focus_Restore_Function)();

bool Set_Video_Mode(int w, int h, int /*bits_per_pixel*/) {
  absl::PrintF("%s\n", __func__);
  return TheDisplay().SetVideoMode(w, h);
}

void Wait_Blit() {
  // nothing to wait for
}

void Delay(int duration) {
  const auto target = g_tick_timer->TickCount() + duration;

  while (g_tick_timer->TickCount() < target) {
    TheDisplay().EndFrame();
  }
}
