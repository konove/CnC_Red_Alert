#ifndef CNC_RED_ALERT_ENGINE_GFX_2KEYFBUF_H_
#define CNC_RED_ALERT_ENGINE_GFX_2KEYFBUF_H_

// File: DrawShapeFrame(), which draws a decompressed shape frame onto
// a page with the transparency, ghost, fade and predator (cloak) effects, and
// ShapeEffects, the tables those effects read.

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"

// The tables and counts behind the drawing effects a SHAPE_* flag selects.
// Each member is read only while its flag is set.
struct ShapeEffects {
  // SHAPE_GHOST: 256 translucency indices, one per shape color, followed by
  // the 256-entry remap rows they select. An index of 0xFF marks an opaque
  // color; any other index N remaps the page pixel under the shape through
  // row N, which is what lets the page show through.
  std::span<const uint8_t> ghost_table;
  // SHAPE_FADING: a 256-entry remap applied to every drawn pixel.
  std::span<const uint8_t> fading_table;
  // SHAPE_PREDATOR: the frame-dependent start of the walk through the sample
  // offsets that makes a cloaked shape shimmer. Only the low three bits of
  // its magnitude matter. Negative uses the second set of offsets, which sample
  // the row below instead of the same row.
  int predator_offset = 0;
};

// Draws the width x height shape frame in `frame` (row-major, width bytes per
// row, color 0 transparent under SHAPE_TRANS) at x,y on `dest`, clipped to it,
// with the SHAPE_* `flags` and the tables in `effects` those flags call for.
// SHAPE_CENTER makes x,y the frame's center; the other flags that do not name
// an effect are ignored. `dest` must be locked. Draws nothing when frame is
// empty or holds fewer than width * height bytes, or when a table the flags
// call for is too short.
//
// A predator pixel samples the page to its right (or, for a negative offset,
// below it), so it picks up whatever was drawn there before this shape.
void DrawShapeFrame(int x, int y, int width, int height,
                    std::span<const std::byte> frame, PixelView& dest,
                    ShapeFlags_Type flags, const ShapeEffects& effects = {});

#endif  // CNC_RED_ALERT_ENGINE_GFX_2KEYFBUF_H_
