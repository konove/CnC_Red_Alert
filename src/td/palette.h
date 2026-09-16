#ifndef CNC_RED_ALERT_TD_PALETTE_H_
#define CNC_RED_ALERT_TD_PALETTE_H_

#include <span>

extern "C" unsigned char CurrentPalette[3 * 256];

void Set_Palette(std::span<const unsigned char> palette);
void Fade_Palette_To(std::span<const unsigned char> palette, int fade,
                     void (*callback)());

#endif  // CNC_RED_ALERT_TD_PALETTE_H_
