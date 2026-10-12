#ifndef CNC_RED_ALERT_TD_CORE_SUPPORT_H_
#define CNC_RED_ALERT_TD_CORE_SUPPORT_H_

#include <cstdint>
#include <source_location>

#include "absl/log/check.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "td/core/config.h"

namespace td {

// Returns a fading table toward `color` by `frac` (0..255; larger values
// count as 255) that stays inside the shadow range, the last 16 palette
// colors: colors 1..239 fade to the closest of colors 240..255, which keep
// their own color, as does color 0. A shadow drawn over a shadow therefore
// does not darken further. `color` must be a palette index (0..255).
FadingTable ConquerBuildFadingTable(const engine::gfx::Palette& palette,
                                    engine::gfx::PaletteIndex color, int frac);
void Fat_Put_Pixel(PixelView& view, int x, int y,
                   engine::gfx::PaletteIndex color, int size);
void strtrim(char* buffer);

// Null pointer check that fires only in cheat-key builds.
inline void Check_Ptr(const void* ptr, std::source_location loc =
                                           std::source_location::current()) {
  if constexpr (config::kCheatKeysEnabled) {
    CHECK(ptr != nullptr) << "NULL pointer, module:" << loc.file_name()
                          << ", line:" << loc.line();
  }
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_SUPPORT_H_
