#ifndef CNC_RED_ALERT_TECH_2KEYBUF_H_
#define CNC_RED_ALERT_TECH_2KEYBUF_H_

#include "sdllib/include/gbuffer.h"

extern "C" {
long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void *Buffer,
                                  GraphicViewPortClass &view, int flags, ...);
}

#endif  // CNC_RED_ALERT_TECH_2KEYBUF_H_
