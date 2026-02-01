#ifndef CNC_RED_ALERT_TD_PALETTE_H_
#define CNC_RED_ALERT_TD_PALETTE_H_

extern "C" unsigned char CurrentPalette[];

void Set_Palette(void* palette);
void Fade_Palette_To(unsigned char* palette, int fade, void (*callback)());

#endif  // CNC_RED_ALERT_TD_PALETTE_H_
