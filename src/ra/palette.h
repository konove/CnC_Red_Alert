#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include <span>

#include "absl/base/attributes.h"
#include "tech/rgb.h"
#include "base/buffer.h"

class PaletteClass {
 public:
  PaletteClass() = default;
  // palettes pass where raw palette bytes are expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  PaletteClass(const RGBClass& /*col*/) noexcept;

  // Makes this the current palette, blending to it over `fade` ticks
  // (kTimerSecond per second) when fade is nonzero. While fading, calls
  // `callback` once per displayed step, or presents the frame itself when
  // callback is nullptr. Switches at once when fades are disabled or when
  // this palette is already current.
  void Set(int fade = 0, void (*callback)() = nullptr);

  // Makes every later Set() switch at once instead of fading. For automated
  // runs (-NOFADE, -QUITFRAME), which have nobody watching the fade.
  static void DisableFades() { fades_disabled_ = true; }

  void Adjust(int /*unused*/);
  void Adjust(int /*unused*/, PaletteClass& /*unused*/);
  static void Partial_Adjust(int /*unused*/, char* /*unused*/);
  static void Partial_Adjust(int /*unused*/, PaletteClass& /*unused*/,
                             char* /*unused*/);

  [[nodiscard]] int Closest_Color(const RGBClass& /*col*/) const;

  // Returns a palette entry; indices outside [0, 256) fail in every build.
  RGBClass& at(int index) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  [[nodiscard]] const RGBClass& at(int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  RGBClass& operator[](int index) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(index);
  }
  const RGBClass& operator[](int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(index);
  }

  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator unsigned char*() noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator const unsigned char*() const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  static const int COLOR_COUNT = 256;

  // Bounded views retain the number of colors for bulk palette operations.
  [[nodiscard]] std::span<RGBClass, COLOR_COUNT> colors()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return data_;
  }
  [[nodiscard]] std::span<const RGBClass, COLOR_COUNT> colors() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return data_;
  }

  [[nodiscard]] std::span<unsigned char> bytes() { return base::UnsignedBytes(colors()); }
  [[nodiscard]] std::span<const unsigned char> bytes() const { return base::UnsignedBytes(colors()); }
  // Palette objects serve as bounded byte ranges to legacy rendering APIs.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator std::span<unsigned char>() { return bytes(); }
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator std::span<const unsigned char>() const { return bytes(); }

  static PaletteClass CurrentPalette;

 private:
  static inline bool fades_disabled_ = false;

  RGBClass data_[COLOR_COUNT];
};

void Set_Palette(std::span<const unsigned char> palette);

extern "C" unsigned char* CurrentPalette;

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
