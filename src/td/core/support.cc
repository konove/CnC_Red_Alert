#include "td/core/support.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstring>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/strings/safe_string.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"

namespace td {

FadingTable ConquerBuildFadingTable(const engine::gfx::Palette& palette,
                                    int color, int frac) {
  const int ALLOWED_COUNT = 16;
  const int ALLOWED_START = 256 - ALLOWED_COUNT;

  CHECK(color >= 0 && color < 256) << "color " << color;
  frac = std::min(frac, 255);
  FadingTable table{};

  // Record the target components.
  const engine::gfx::PaletteColor& target = palette.at(color);
  const int targetred = target.red();
  const int targetgreen = target.green();
  const int targetblue = target.blue();

  // Entry 0 stays 0: transparent black never gets remapped.
  for (int remap_index = 1; remap_index < ALLOWED_START; remap_index++) {
    const engine::gfx::PaletteColor& orig = palette.at(remap_index);
    const int origred = orig.red();
    const int origgreen = orig.green();
    const int origblue = orig.blue();

    // The products can be negative; the shifts floor them as the original
    // table builder did, so the palette comes out identical.
    int tmp = (origred - targetred) * (frac / 2);
    const int idealred =
        origred - (tmp >> 7);  // NOLINT(bugprone-signed-bitwise)

    tmp = (origgreen - targetgreen) * (frac / 2);
    const int idealgreen =
        origgreen - (tmp >> 7);  // NOLINT(bugprone-signed-bitwise)

    tmp = (origblue - targetblue) * (frac / 2);
    const int idealblue =
        origblue - (tmp >> 7);  // NOLINT(bugprone-signed-bitwise)

    // Sweep through a limited set of existing colors to find the closest
    // matching color.

    int matchcolor = color;    // Default color (self).
    int matchvalue = INT_MAX;  // Ridiculous match value init.

    for (int color_index = ALLOWED_START; color_index < 256; color_index++) {
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
        matchcolor = color_index;
        break;
      }

      if (compval < matchvalue) {
        matchcolor = color_index;
        matchvalue = compval;
      }
    }

    // When the loop exits, we have found the closest match.
    base::At(table, remap_index) = static_cast<uint8_t>(matchcolor);
  }

  // The candidate colors remap to themselves.
  for (int remap_index = ALLOWED_START; remap_index < 256; remap_index++) {
    base::At(table, remap_index) = static_cast<uint8_t>(remap_index);
  }

  return table;
}

void Fat_Put_Pixel(PixelView& view, int x, int y, std::uint8_t color,
                   int size) {
  view.FillRect(Rect{x, y, size, size}, color);
}

// from RA readline.cpp
void strtrim(char* buffer) {
  const auto storage = base::MutableCString(buffer);
  if (storage.empty()) {
    return;
  }
  const auto text = storage.first(storage.size() - 1);
  size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(base::At(text, first)))) {
    ++first;
  }
  size_t last = text.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(base::At(text, last - 1)))) {
    --last;
  }
  base::MoveBytes(std::as_writable_bytes(storage),
                  std::as_bytes(text.subspan(first, last - first)),
                  last - first);
  base::At(storage, last - first) = '\0';
}

}  // namespace td
