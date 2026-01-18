#include "td/winasm.h"

#include <cstdio>

#include "sdllib/include/gbuffer.h"

#ifndef LORES

void ModeX_Blit(GraphicBufferClass* /*source*/) { printf("%s\n", __func__); }

#endif  // LORES
