#ifndef CNC_RED_ALERT_TECH_2KEYFBUF_H_
#define CNC_RED_ALERT_TECH_2KEYFBUF_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "sdllib/gbuffer.h"
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
                          GraphicViewPortClass& dest, ShapeFlags_Type flags,
                          const ShapeEffects& effects = {});

// Returns the drawing-effect bits of `flags` that a cached shape header is
// keyed on. Buffer_Frame_To_Page precomputes one line-blit flag per scan line
// for a shape, and that cache is only valid for a later draw requesting the
// same effects, so the stored key and the key it is compared against must come
// from here rather than be spelled out twice.
constexpr ShapeFlags_Type ShapeEffectFlags(const ShapeFlags_Type flags) {
  return flags & (SHAPE_TRANS | SHAPE_FADING | SHAPE_PREDATOR | SHAPE_GHOST);
}

// Selects the pre-Win95 shape blitter. Set by the game's CC_Draw_Shape()
// around the rotate-and-scale path, which produces raw shape data the new
// blitter cannot read.
inline bool UseOldShapeDraw = false;

// Uncompressed shape buffers owned by the game's keyframe loader. While
// UseBigShapeBuffer is set, cached shape headers hold offsets from the start of
// the big buffer, or of the theater buffer for theater-specific shapes.
extern char* BigShapeBufferStart;
extern std::span<char> BigShapeBufferBytes;
extern std::span<char> TheaterShapeBufferBytes;
extern char* TheaterShapeBufferStart;
extern bool UseBigShapeBuffer;

#endif  // CNC_RED_ALERT_TECH_2KEYFBUF_H_
