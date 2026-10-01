#ifndef CNC_RED_ALERT_RA_ASSETS_PALETTE_H_
#define CNC_RED_ALERT_RA_ASSETS_PALETTE_H_

#include <span>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"

namespace ra {

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

  // Moves every entry `ratio`/256 of the way toward black (ratio 0..255).
  // Rounds toward the start color, so even ratio 255 can leave a gun at 1.
  void Adjust(int ratio);
  // Moves every entry `ratio`/256 of the way toward the same entry of
  // `palette` (ratio 0..255).
  void Adjust(int ratio, const engine::gfx::Palette& palette);
  // As Adjust(), but only for the entries whose `lookup` byte is nonzero; the
  // rest keep their color.
  void Partial_Adjust(int ratio, std::span<const char, kColorCount> lookup);
  void Partial_Adjust(int ratio, const engine::gfx::Palette& palette,
                      std::span<const char, kColorCount> lookup);

  [[nodiscard]] int Closest_Color(
      const engine::gfx::PaletteColor& /*col*/) const;

  static constexpr int COLOR_COUNT = kColorCount;

  static PaletteClass CurrentPalette;

 private:
  static inline bool fades_disabled_ = false;
};

// Makes `palette` current and shows it at once.
void Set_Palette(const engine::gfx::Palette& palette);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_ASSETS_PALETTE_H_
