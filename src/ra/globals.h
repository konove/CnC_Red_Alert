#ifndef CNC_RED_ALERT_RA_GLOBALS_H_
#define CNC_RED_ALERT_RA_GLOBALS_H_

#include <cstdint>

#include "sdllib/gbuffer.h"

extern int64_t Frame;
extern bool GameActive;
extern int32_t LParam;
extern int Seed;
extern int CustomSeed;
extern bool SoundOn;

// The two full-screen video pages: the one on screen and the back buffer the
// game draws into before blitting. InitVideo() sizes them to the video mode.
extern GraphicBufferClass visible_page;
extern GraphicBufferClass hidden_page;

// The 640x400 game area within each page, 40 lines down in a 480-line mode.
extern GraphicViewPortClass visible_view;
extern GraphicViewPortClass hidden_view;

#endif  // CNC_RED_ALERT_RA_GLOBALS_H_
