#ifndef CNC_RED_ALERT_TECH_2KEYFBUF_H_
#define CNC_RED_ALERT_TECH_2KEYFBUF_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "sdllib/pixel_buffer.h"
#include "sdllib/shape.h"

// The tables and counts behind the drawing effects a SHAPE_* flag selects.
// Each member is read only while its flag is set.
struct ShapeEffects {
  // SHAPE_GHOST: 256 translucency indices (0xFF for an opaque color) followed
  // by the 256-entry remap rows the index selects from.
  std::span<const uint8_t> ghost_table;
  // SHAPE_FADING: a 256-entry remap applied fading_count times; 0 turns the
  // fade off. Only the low six bits of the count are used.
  std::span<const uint8_t> fading_table;
  int fading_count = 0;
  // SHAPE_PREDATOR: the frame-dependent sample offset that makes a cloaked
  // shape shimmer; negative walks it the other way.
  int predator_offset = 0;
  // SHAPE_PARTIAL: the low byte is the fraction of pixels that get the
  // predator effect.
  int partial_predator = 0;
};

// Draws the w x h shape at `src`, clipped to `dest`, at x,y with the SHAPE_*
// `flags` and the tables in `effects` those flags call for. A nullptr src
// draws nothing.
void Buffer_Frame_To_Page(int x, int y, int w, int h, std::span<std::byte> src,
                          PixelView& dest, ShapeFlags_Type flags,
                          const ShapeEffects& effects = {});

#endif  // CNC_RED_ALERT_TECH_2KEYFBUF_H_
