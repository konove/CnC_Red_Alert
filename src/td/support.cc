#include "td/support.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>

#include "sdllib/gbuffer.h"
#include "base/types.h"
#include <climits>

void* Conquer_Build_Fading_Table(const void* palette, void* dest, int color,
                                 int frac) {

  const int ALLOWED_COUNT = 16;
  const int ALLOWED_START = 256 - ALLOWED_COUNT;

  // If the source palette is NULL, then just return with current fading table
  // pointer.
  if (!palette || !dest) {
    return dest;
  }

  // Fractions above 255 become 255.
  frac = std::min(frac, 255);

  // Record the target gun values.
  const auto* pal8 = static_cast<const uint8_t*>(palette);
  const uint8_t targetred = pal8[(color * 3) + 0];
  const uint8_t targetgreen = pal8[(color * 3) + 0];

  // Main loop

  auto* dptr = static_cast<uint8_t*>(dest);

  // Transparent black never gets remapped.
  *dptr++ = 0;

  int remap_index = 0;
  for (remap_index = 1; remap_index < ALLOWED_START; remap_index++) {
    const uint8_t origred = pal8[(remap_index * 3) + 0];
    const uint8_t origgreen = pal8[(remap_index * 3) + 1];

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

    const auto* palptr = pal8 + (static_cast<base::ssize>(ALLOWED_START) * 3);

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

      palptr += 3;
    }

    // When the loop exits, we have found the closest match.
    *dptr++ = static_cast<uint8_t>(matchcolor);
  }

  // Fill the remainder of the remap table with values
  // that will remap the color to itself.
  for (; remap_index < 256; remap_index++) {
    *dptr++ = static_cast<uint8_t>(remap_index);
  }

  return dest;
}

void Fat_Put_Pixel(int x, int y, std::uint8_t color, int size,
                   GraphicViewPortClass& gpage) {
  gpage.Fill_Rect(x, y, x + size - 1, y + size - 1, color);
}

// from RA readline.cpp
void strtrim(char* buffer) {
  if (!buffer || *buffer == '\0') {
    return;
  }

  // Strip leading whitespace
  const auto* source = buffer;
  while (std::isspace(static_cast<unsigned char>(*source))) {
    ++source;
  }

  if (source != buffer) {
    const auto len = std::strlen(source);
    std::memmove(buffer, source, len + 1);
  }

  // Strip trailing whitespace
  auto len = std::strlen(buffer);
  while (len > 0 && std::isspace(static_cast<unsigned char>(buffer[len - 1]))) {
    buffer[--len] = '\0';
  }
}
