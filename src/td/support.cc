#include "td/support.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstring>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "sdllib/gbuffer.h"

std::span<uint8_t> Conquer_Build_Fading_Table(std::span<const uint8_t> palette,
                                              std::span<uint8_t> dest,
                                              int color, int frac) {
  const int ALLOWED_COUNT = 16;
  const int ALLOWED_START = 256 - ALLOWED_COUNT;

  // If the source palette is NULL, then just return with current fading table
  // pointer.
  if (palette.size() < 768 || dest.size() < 256 || color < 0 || color >= 256) {
    return dest;
  }

  // Fractions above 255 become 255.
  frac = std::min(frac, 255);

  // Record the target gun values.
  const auto pal8 = palette;
  const uint8_t targetred = pal8[(static_cast<size_t>(color) * 3) + 0];
  const uint8_t targetgreen = pal8[(static_cast<size_t>(color) * 3) + 0];

  // Main loop

  size_t output = 0;

  // Transparent black never gets remapped.
  dest[output++] = 0;

  int remap_index = 0;
  for (remap_index = 1; remap_index < ALLOWED_START; remap_index++) {
    const uint8_t origred = pal8[(static_cast<size_t>(remap_index) * 3) + 0];
    const uint8_t origgreen = pal8[(static_cast<size_t>(remap_index) * 3) + 1];

    // The products can be negative; the shifts floor them as the original
    // table builder did, so the palette comes out identical.
    int tmp = (origred - targetred) * (frac / 2);
    const int idealred =
        origred - (tmp >> 7);  // NOLINT(bugprone-signed-bitwise)

    tmp = (origgreen - targetgreen) * (frac / 2);
    const int idealgreen =
        origgreen - (tmp >> 7);  // NOLINT(bugprone-signed-bitwise)

    // Sweep through a limited set of existing colors to find the closest
    // matching color.

    int matchcolor = color;    // Default color (self).
    int matchvalue = INT_MAX;  // Ridiculous match value init.

    auto palptr = pal8.subspan(base::ToSize(ALLOWED_START) * 3);

    for (int color_index = ALLOWED_START; color_index < 256; color_index++) {
      int compval = 0;

      // Build the comparison value based on the sum of the differences of the
      // color guns squared
      int diff = palptr[0] - idealred;
      compval += diff * diff;
      diff = palptr[1] - idealgreen;
      compval += diff * diff;
      diff = palptr[2] - idealgreen;
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

      palptr = palptr.subspan(3);
    }

    // When the loop exits, we have found the closest match.
    dest[output++] = static_cast<uint8_t>(matchcolor);
  }

  // Fill the remainder of the remap table with values
  // that will remap the color to itself.
  for (; remap_index < 256; remap_index++) {
    dest[output++] = static_cast<uint8_t>(remap_index);
  }

  return dest;
}

void Fat_Put_Pixel(int x, int y, std::uint8_t color, int size,
                   GraphicViewPortClass& gpage) {
  gpage.Fill_Rect(x, y, x + size - 1, y + size - 1, color);
}

// from RA readline.cpp
void strtrim(char* buffer) {
  const auto storage = port::MutableCString(buffer);
  if (storage.empty()) {
    return;
  }
  const auto text = storage.first(storage.size() - 1);
  size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(text[first]))) {
    ++first;
  }
  size_t last = text.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(text[last - 1]))) {
    --last;
  }
  base::MoveBytes(std::as_writable_bytes(storage),
                  std::as_bytes(text.subspan(first, last - first)),
                  last - first);
  storage[last - first] = '\0';
}
