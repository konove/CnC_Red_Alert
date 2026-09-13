#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include "absl/base/attributes.h"
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
  static void Partial_Adjust(int /*unused*/, char* /*unused*/);
  static void Partial_Adjust(int /*unused*/, PaletteClass& /*unused*/,
                             char* /*unused*/);

  [[nodiscard]] int Closest_Color(const RGBClass& /*col*/) const;

  RGBClass& operator[](int index) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  const RGBClass& operator[](int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator unsigned char*() noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator const unsigned char*() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  static const int COLOR_COUNT = 256;

  static PaletteClass CurrentPalette;

 private:
  RGBClass data_[COLOR_COUNT];
};

void Set_Palette(void* palette);

extern "C" unsigned char* CurrentPalette;

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
