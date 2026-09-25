#include "engine/gfx/text_window.h"

// The window rows the games draw text into. engine_gfx owns the storage and
// each game fills in its own rows at startup, which is what kept every
// game and every test that links engine_gfx defining this array for itself.
int WindowList[kWindowCount][8]{};

unsigned int WinX;
unsigned int WinY;
unsigned int Window;
