#include "engine/window/wait_ticks.h"

#include "engine/platform/timer.h"
#include "engine/window/display.h"

namespace engine::window {

void WaitTicks(int ticks) {
  const auto target = SystemTicks() + ticks;

  while (SystemTicks() < target) {
    TheDisplay().EndFrame();
  }
}

}  // namespace engine::window
