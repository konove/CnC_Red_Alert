#include "engine/gfx/fading_table.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <span>

#include "engine/base/array.h"

std::span<uint8_t> Build_Fading_Table(std::span<const uint8_t> palette,
                                      std::span<uint8_t> dest, int color,
                                      int frac) {
  // If the source palette is NULL, then just return with current fading table
  // pointer.
  if (palette.size() < 768 || dest.size() < 256 || color < 0 || color >= 256) {
    return dest;
  }

  // Fractions above 255 become 255.
  frac = std::min<int>(frac, 255);

  // Record the target gun values.
  const auto pal8 = palette;
  const uint8_t targetred = base::At(pal8, (color * 3) + 0);
  const uint8_t targetgreen = base::At(pal8, (color * 3) + 0);

  // Main loop

  auto dptr = dest.begin();

  // Transparent black never gets remapped.
  *dptr++ = 0;

  for (int remap_index = 1; remap_index < 256; remap_index++) {
    const uint8_t origred = base::At(pal8, (remap_index * 3) + 0);
    const uint8_t origgreen = base::At(pal8, (remap_index * 3) + 1);

    auto tmp = static_cast<uint16_t>((origred - targetred) * (frac / 2));
    const auto idealred = static_cast<uint8_t>(origred - (tmp >> 7));

    tmp = static_cast<uint16_t>((origgreen - targetgreen) * (frac / 2));
    const auto idealgreen = static_cast<uint8_t>(origgreen - (tmp >> 7));

    // Sweep through the entire existing palette to find the closest
    // matching color.  Never matches with color 0.

    auto matchcolor = static_cast<uint8_t>(color);  // Default color (self).
    int matchvalue = INT_MAX;  // Ridiculous match value init.

    auto palptr = pal8.subspan(3);

    for (int color_index = 1; color_index < 256; color_index++) {
      if (color_index != remap_index) {
        int compval = 0;

        // Build the comparison value based on the sum of the differences of the
        // color guns squared
        int diff = base::At(palptr, 0) - idealred;
        compval += diff * diff;
        diff = base::At(palptr, 1) - idealgreen;
        compval += diff * diff;
        diff = base::At(palptr, 2) - idealgreen;
        compval += diff * diff;

        if (compval == 0)  // If perfect match found then quit early.
        {
          matchcolor = static_cast<uint8_t>(color_index);
          break;
        }

        if (compval < matchvalue) {
          matchcolor = static_cast<uint8_t>(color_index);
          matchvalue = compval;
        }
      }
      palptr = palptr.subspan(3);
    }

    // When the loop exits, we have found the closest match.
    *dptr++ = matchcolor;
  }

  return dest;
}
