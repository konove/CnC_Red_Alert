#include "sdllib/misc.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "absl/log/check.h"
#include "base/hsv.h"
#include "sdllib/timer.h"
#include "sdllib/ww_win.h"
#include <climits>

SurfaceMonitorClass AllSurfaces;

extern "C" {
long RandNumb;
}

void (*Misc_Focus_Loss_Function)();
void (*Misc_Focus_Restore_Function)();

bool Set_Video_Mode(void* /*hwnd*/, int /*w*/, int /*h*/,
                    int /*bits_per_pixel*/) {
  printf("%s\n", __func__);
  return true;
}

void Wait_Blit() {
  // nothing to wait for
}

void Delay(int duration) {
  auto target = g_tick_timer->TickCount() + duration;

  while (g_tick_timer->TickCount() < target) {
    Video_End_Frame();
  }
}

void* Build_Fading_Table(const void* palette, void* dest, long int color,
                         long int frac) {
  int matchvalue;
  uint8_t targetred;
  uint8_t targetgreen;
  uint8_t idealred;
  uint8_t idealgreen;
  uint8_t matchcolor;

  // If the source palette is NULL, then just return with current fading table
  // pointer.
  if (!palette || !dest) {
    return dest;
  }

  // Fractions above 255 become 255.
  frac = std::min<long>(frac, 255);

  // Record the target gun values.
  const auto* pal8 = (uint8_t*)palette;
  targetred = pal8[color * 3 + 0];
  targetgreen = pal8[color * 3 + 0];

  // Main loop

  auto* dptr = static_cast<uint8_t*>(dest);

  // Transparent black never gets remapped.
  *dptr++ = 0;

  for (int remap_index = 1; remap_index < 256; remap_index++) {
    uint8_t origred = pal8[remap_index * 3 + 0];
    uint8_t origgreen = pal8[remap_index * 3 + 1];

    uint16_t tmp = (origred - targetred) * (frac >> 1);
    idealred = origred - (tmp >> 7);

    tmp = (origgreen - targetgreen) * (frac >> 1);
    idealgreen = origgreen - (tmp >> 7);

    // Sweep through the entire existing palette to find the closest
    // matching color.  Never matches with color 0.

    matchcolor = color;  // Default color (self).
    matchvalue = INT_MAX;  // Ridiculous match value init.

    const auto* palptr = pal8 + 3;

    for (int color_index = 1; color_index < 256; color_index++) {
      if (color_index != remap_index) {
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
      }
      palptr += 3;
    }

    // When the loop exits, we have found the closest match.
    *dptr++ = matchcolor;
  }

  return dest;
}

int Confine_Rect(int* x, int* y, int dw, int dh, int width, int height) {
  int ret = 0;

  if (*x < 0) {
    *x = 0;
    ret = 1;
  } else if (*x + dw > width) {
    *x -= *x + dw - width;

    *x = std::max(*x, 0);

    ret = 1;
  }

  if (*y < 0) {
    *y = 0;
    ret = 1;
  } else if (*y + dh > height) {
    *y -= *y + dh - height;

    *y = std::max(*y, 0);

    ret = 1;
  }

  return ret;
}

// these are used by TD
static int Get_Random_Mask(unsigned int maxval) {
  // original asm did something using bsr but I can't be bothered
  maxval |= maxval >> 1;
  maxval |= maxval >> 2;
  maxval |= maxval >> 4;
  maxval |= maxval >> 8;
  maxval |= maxval >> 16;
  return maxval;
}

int IRandom(int minval, int maxval) {
  int num, mask;

  // Keep minval and maxval straight.
  if (minval > maxval) {
    minval ^= maxval;
    maxval ^= minval;
    minval ^= maxval;
  }

  mask = Get_Random_Mask(maxval - minval);

  while ((num = (rand() & mask) + minval) > maxval);
  return num;
}

uint8_t Random() {
  // mmm
  auto* r = (uint8_t*)&RandNumb;

  uint8_t tmp = r[0] >> 1;
  int c = tmp & 1;
  tmp >>= 1;

  int c1 = r[2] & 0x80;
  r[2] = r[2] << 1 | c;

  int c2 = r[1] & 0x80;
  r[1] = r[1] << 1 | c1 >> 7;

  tmp = tmp - ((RandNumb & 0xFF) + (1 - c2));
  int c3 = tmp & 1;

  r[0] = r[0] >> 1 | c3 << 7;

  return r[0] ^ r[1];
}

// from WIN32LIB/MISC/LIB.CPP
static unsigned Divide_With_Round(unsigned num, unsigned den) {
  return static_cast<unsigned>(
      base::DivideWithRound(static_cast<int>(num), static_cast<int>(den)));
}

#define HSV_BASE \
  255  // This is used to get a little better persion on HSV conversion.
#define RGB_BASE 63  // Not 64, this is really the max value.

void Convert_RGB_To_HSV(unsigned int r, unsigned int g, unsigned int b,
                        unsigned int* h, unsigned int* s, unsigned int* v) {
  unsigned int m, r1, g1, b1, tmp;

  // Convert RGB base to HSV base.
  r = Divide_With_Round(r * HSV_BASE, RGB_BASE);
  g = Divide_With_Round(g * HSV_BASE, RGB_BASE);
  b = Divide_With_Round(b * HSV_BASE, RGB_BASE);

  // Set hue to default.
  *h = 0;

  // Set v = Max(r,g,b) to find dominant primary color.
  *v = r > g ? r : g;
  *v = std::max(b, *v);

  // Set m = min(r,g,b) to find amount of white.
  m = r < g ? r : g;
  m = std::min(b, m);

  // Determine the normalized saturation.
  if (*v != 0) {
    *s = Divide_With_Round((*v - m) * HSV_BASE, *v);
  } else {
    *s = 0;
  }

  if (*s != 0) {
    tmp = *v - m;
    CHECK_NE(tmp, 0U);
    r1 = Divide_With_Round((*v - r) * HSV_BASE, tmp);
    g1 = Divide_With_Round((*v - g) * HSV_BASE, tmp);
    b1 = Divide_With_Round((*v - b) * HSV_BASE, tmp);

    // Find effect of second most predominant color.
    // In which section of the hexagon of colors does the color lie?
    if (*v == r) {
      if (m == g) {
        *h = 5 * HSV_BASE + b1;
      } else {
        *h = 1 * HSV_BASE - g1;
      }
    } else {
      if (*v == g) {
        if (m == b) {
          *h = 1 * HSV_BASE + r1;
        } else {
          *h = 3 * HSV_BASE - b1;
        }
      } else {
        // *v == b
        if (m == r) {
          *h = 3 * HSV_BASE + g1;
        } else {
          *h = 5 * HSV_BASE - r1;
        }
      }
    }

    // Divide by six and round.
    *h = Divide_With_Round(*h, 6);
  }
}

void Convert_HSV_To_RGB(unsigned int h, unsigned int s, unsigned int v,
                        unsigned int* r, unsigned int* g, unsigned int* b) {
  const base::Rgb8 color = base::HsvToRgb8(
      static_cast<int>(h), static_cast<int>(s), static_cast<int>(v));

  // HsvToRgb8 works at HSV_BASE; scale each gun down to the VGA RGB_BASE.
  *r = static_cast<unsigned int>(
      base::DivideWithRound(color.red * RGB_BASE, HSV_BASE));
  *g = static_cast<unsigned int>(
      base::DivideWithRound(color.green * RGB_BASE, HSV_BASE));
  *b = static_cast<unsigned int>(
      base::DivideWithRound(color.blue * RGB_BASE, HSV_BASE));
}
