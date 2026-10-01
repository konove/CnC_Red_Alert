#include "engine/window/palette_fade.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>

#include "engine/gfx/palette.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"

namespace engine::window {

void FadePalette(const gfx::Palette& from, const gfx::Palette& to,
                 const int ticks, void (*show)(const gfx::Palette&),
                 void (*step)()) {
  // A fade to the palette already on screen would spend its whole duration
  // redrawing an unchanged screen, e.g. the fade to black before a movie
  // that starts from black.
  if (ticks <= 0 || from == to) {
    return;
  }

  const int64_t start_time = platform::SystemTicks();
  while (true) {
    const int64_t now = platform::SystemTicks();
    const int elapsed =
        static_cast<int>(std::min<int64_t>(now - start_time, ticks));

    show(gfx::BlendedPalette(from, to, elapsed, ticks));
    if (step != nullptr) {
      step();
    } else {
      TheDisplay().EndFrame();
    }

    if (elapsed == ticks) {
      return;
    }

    // Presenting again before the tick changes would redraw an identical
    // frame. Unpaced, this loop presented over a thousand times a second and
    // kept a core busy for the fade.
    while (platform::SystemTicks() == now) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

}  // namespace engine::window
