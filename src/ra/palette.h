#ifndef CNC_RED_ALERT_RA_PALETTE_H_
#define CNC_RED_ALERT_RA_PALETTE_H_

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"

// Red Alert's palette: the shared engine::gfx::Palette plus fading to it on
// screen and the closest-color search the remap tables use.
class PaletteClass : public engine::gfx::Palette {
 public:
  PaletteClass() = default;
  // Fills every entry with `col`. Implicit, so that a color converts where a
  // palette is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  PaletteClass(const engine::gfx::PaletteColor& col) noexcept : Palette(col) {}
  // NOLINTNEXTLINE(*-explicit-constructor)
  PaletteClass(const engine::gfx::Palette& palette) noexcept
      : Palette(palette) {}

  // Makes this the current palette, blending to it over `fade` ticks
  // (kTimerSecond per second) when fade is positive. While fading, calls
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

  [[nodiscard]] int Closest_Color(
      const engine::gfx::PaletteColor& /*col*/) const;

  static constexpr int COLOR_COUNT = kColorCount;

  static PaletteClass CurrentPalette;

 private:
  static inline bool fades_disabled_ = false;
};

// Makes `palette` current and shows it at once.
void Set_Palette(const engine::gfx::Palette& palette);

#endif  // CNC_RED_ALERT_RA_PALETTE_H_
