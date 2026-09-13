#ifndef CNC_RED_ALERT_TECH_2KEYFBUF_H_
#define CNC_RED_ALERT_TECH_2KEYFBUF_H_

#include "sdllib/gbuffer.h"
#include "sdllib/shape.h"

extern "C" {
long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void* src,
                                  GraphicViewPortClass& dest, int flags, ...);
}

// Returns the drawing-effect bits of `flags` that a cached shape header is
// keyed on. Buffer_Frame_To_Page precomputes one line-blit flag per scan line
// for a shape, and that cache is only valid for a later draw requesting the
// same effects, so the stored key and the key it is compared against must come
// from here rather than be spelled out twice.
constexpr int ShapeEffectFlags(const int flags) {
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
extern char* TheaterShapeBufferStart;
extern bool UseBigShapeBuffer;

#endif  // CNC_RED_ALERT_TECH_2KEYFBUF_H_
