// File: Palettes, Red Alert's shared palettes and color remap tables.

#ifndef CNC_RED_ALERT_RA_PALETTES_H_
#define CNC_RED_ALERT_RA_PALETTES_H_

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "base/installed.h"
#include "ra/defines.h"
#include "ra/palette.h"
#include "tech/rgb.h"

// The palettes the game switches between and the remap tables it draws text
// and house colors with. Game owns the one Palettes; everything else reaches
// it through ThePalettes(). A new Palettes has black and white filled in and
// everything else zeroed; Init_Color_Remaps() and the title and theater
// loaders fill the rest.
//
// Example:
//   ThePalettes().black_palette().Set(kFadePaletteSlow);
class Palettes {
 public:
  Palettes() = default;
  ~Palettes() = default;

  Palettes(const Palettes&) = delete;
  Palettes& operator=(const Palettes&) = delete;
  Palettes(Palettes&&) = delete;
  Palettes& operator=(Palettes&&) = delete;

  // The palette the map is drawn in: the theater palette with the player's
  // brightness, saturation, contrast and tint applied.
  PaletteClass& game_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return game_palette_;
  }
  // The theater palette before those adjustments.
  PaletteClass& original_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return original_palette_;
  }
  // The palette of the title and menu screens.
  PaletteClass& title_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return title_palette_;
  }
  // All black and all white, for fades. Movie playback adjusts the black one
  // toward white and back.
  PaletteClass& black_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return black_palette_;
  }
  PaletteClass& white_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return white_palette_;
  }

  // The remap tables for each player color and the dialog color schemes.
  base::EnumArray<PlayerColorType, RemapControlType>& color_remaps()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return color_remaps_;
  }
  // The scheme for text printed over the metallic sidebar tabs.
  RemapControlType& metal_scheme() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return metal_scheme_;
  }
  // Dark grey shades, for dimming things out.
  RemapControlType& grey_scheme() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return grey_scheme_;
  }

  // Whether palette changes fade slowly (the SlowPalette option), which also
  // makes movies set their palettes on vertical blank.
  [[nodiscard]] bool slow_palette() const { return slow_palette_; }
  void set_slow_palette(bool slow_palette) { slow_palette_ = slow_palette; }

 private:
  PaletteClass game_palette_;
  PaletteClass original_palette_;
  PaletteClass title_palette_;
  PaletteClass black_palette_{RGBClass(0, 0, 0)};
  PaletteClass white_palette_{
      RGBClass(RGBClass::kMaxValue, RGBClass::kMaxValue, RGBClass::kMaxValue)};
  base::EnumArray<PlayerColorType, RemapControlType> color_remaps_{};
  RemapControlType metal_scheme_{};
  RemapControlType grey_scheme_{};
  bool slow_palette_ = false;
};

// Returns the Palettes that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Palettes& ThePalettes() { return base::Installed<Palettes>::Get(); }

#endif  // CNC_RED_ALERT_RA_PALETTES_H_
