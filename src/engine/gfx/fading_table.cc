#include "engine/gfx/fading_table.h"

#include <algorithm>
#include <climits>
#include <cstdint>

#include "absl/log/check.h"
#include "base/array.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"

FadingTable BuildFadingTable(const engine::gfx::Palette& palette, int color,
                             int frac) {
  CHECK(color >= 0 && color < 256) << "color " << color;
  frac = std::min<int>(frac, 255);
  FadingTable table{};

  // Record the target components.
  const engine::gfx::PaletteColor& target = palette.at(color);
  const auto targetred = static_cast<uint8_t>(target.red());
  const auto targetgreen = static_cast<uint8_t>(target.green());
  const auto targetblue = static_cast<uint8_t>(target.blue());

  // Entry 0 stays 0: transparent black never gets remapped.
  for (int remap_index = 1; remap_index < 256; remap_index++) {
    const engine::gfx::PaletteColor& orig = palette.at(remap_index);
    const auto origred = static_cast<uint8_t>(orig.red());
    const auto origgreen = static_cast<uint8_t>(orig.green());
    const auto origblue = static_cast<uint8_t>(orig.blue());

    auto tmp = static_cast<uint16_t>((origred - targetred) * (frac / 2));
    const auto idealred = static_cast<uint8_t>(origred - (tmp >> 7));

    tmp = static_cast<uint16_t>((origgreen - targetgreen) * (frac / 2));
    const auto idealgreen = static_cast<uint8_t>(origgreen - (tmp >> 7));

    tmp = static_cast<uint16_t>((origblue - targetblue) * (frac / 2));
    const auto idealblue = static_cast<uint8_t>(origblue - (tmp >> 7));

    // Sweep through the entire existing palette to find the closest
    // matching color.  Never matches with color 0.

    auto matchcolor = static_cast<uint8_t>(color);  // Default color (self).
    int matchvalue = INT_MAX;  // Ridiculous match value init.

    for (int color_index = 1; color_index < 256; color_index++) {
      if (color_index != remap_index) {
        int compval = 0;

        // The comparison value is the sum of the squared component differences.
        const engine::gfx::PaletteColor& candidate = palette.at(color_index);
        int diff = candidate.red() - idealred;
        compval += diff * diff;
        diff = candidate.green() - idealgreen;
        compval += diff * diff;
        diff = candidate.blue() - idealblue;
        compval += diff * diff;

        if (compval == 0)  // If perfect match found then quit early.
        {
          matchcolor = static_cast<uint8_t>(color_index);
          break;
        }

        // Ties go to the later color, as in the original assembly; the
        // tables TD ships were built that way.
        if (compval <= matchvalue) {
          matchcolor = static_cast<uint8_t>(color_index);
          matchvalue = compval;
        }
      }
    }

    // When the loop exits, we have found the closest match.
    base::At(table, remap_index) = matchcolor;
  }

  return table;
}
