#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include "ra/rgb.h"

class PaletteClass {
 public:
  PaletteClass();
  PaletteClass(const RGBClass &);

  void Set(int fade = 0, void (*callback)() = nullptr);

  void Adjust(int);
  void Adjust(int, PaletteClass &);
  void Partial_Adjust(int, char *);
  void Partial_Adjust(int, PaletteClass &, char *);

  int Closest_Color(const RGBClass &) const;

  RGBClass &operator[](int index);
  const RGBClass &operator[](int index) const;

  operator unsigned char *();
  operator const unsigned char *() const;

  static const int COLOR_COUNT = 256;

  static PaletteClass CurrentPalette;

 private:
  RGBClass data_[COLOR_COUNT];
};

void Set_Palette(void *palette);

extern "C" unsigned char *CurrentPalette;

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
