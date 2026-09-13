#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include "tech/rgb.h"

class PaletteClass {
 public:
  PaletteClass() = default;
  // palettes pass where raw palette bytes are expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  PaletteClass(const RGBClass& /*col*/) noexcept;

  void Set(int fade = 0, void (*callback)() = nullptr);

  void Adjust(int /*unused*/);
  void Adjust(int /*unused*/, PaletteClass& /*unused*/);
  void Partial_Adjust(int /*unused*/, char* /*unused*/);
  void Partial_Adjust(int /*unused*/, PaletteClass& /*unused*/,
                      char* /*unused*/);

  [[nodiscard]] int Closest_Color(const RGBClass& /*col*/) const;

  RGBClass& operator[](int index);
  const RGBClass& operator[](int index) const;

  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator unsigned char*() noexcept;
  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator const unsigned char*() const;

  static const int COLOR_COUNT = 256;

  static PaletteClass CurrentPalette;

 private:
  RGBClass data_[COLOR_COUNT];
};

void Set_Palette(void* palette);

extern "C" unsigned char* CurrentPalette;

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
