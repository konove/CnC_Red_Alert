#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include "tech/rgb.h"

class PaletteClass {
 public:
  PaletteClass() = default;
  // NOLINTNEXTLINE(*-explicit-constructor): palettes pass where raw palette bytes are expected.
  PaletteClass(const RGBClass&) noexcept;

  void Set(int fade = 0, void (*callback)() = nullptr);

  void Adjust(int);
  void Adjust(int, PaletteClass&);
  void Partial_Adjust(int, char*);
  void Partial_Adjust(int, PaletteClass&, char*);

  int Closest_Color(const RGBClass&) const;

  RGBClass& operator[](int index);
  const RGBClass& operator[](int index) const;

  // NOLINTNEXTLINE(*-explicit-constructor): legacy C interfaces take the object where a pointer or name is expected.
  operator unsigned char*() noexcept;
  // NOLINTNEXTLINE(*-explicit-constructor): legacy C interfaces take the object where a pointer or name is expected.
  operator const unsigned char*() const;

  static const int COLOR_COUNT = 256;

  static PaletteClass CurrentPalette;

 private:
  RGBClass data_[COLOR_COUNT];
};

void Set_Palette(void* palette);

extern "C" unsigned char* CurrentPalette;

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
