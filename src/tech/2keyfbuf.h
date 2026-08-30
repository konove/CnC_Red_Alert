#ifndef CNC_RED_ALERT_TECH_2KEYFBUF_H_
#define CNC_RED_ALERT_TECH_2KEYFBUF_H_

#include "sdllib/gbuffer.h"

extern "C" {
long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void* Buffer,
                                  GraphicViewPortClass& view, int flags, ...);
}

// Selects the pre-Win95 shape blitter. Set by the game's CC_Draw_Shape()
// around the rotate-and-scale path, which produces raw shape data the new
// blitter cannot read.
inline bool UseOldShapeDraw = false;

#endif  // CNC_RED_ALERT_TECH_2KEYFBUF_H_
