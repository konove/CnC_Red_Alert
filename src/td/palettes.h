// File: Palettes, Tiberian Dawn's shared palettes.

#ifndef CNC_RED_ALERT_TD_PALETTES_H_
#define CNC_RED_ALERT_TD_PALETTES_H_

#include <vector>

#include "absl/base/attributes.h"
#include "engine/base/installed.h"

// The 768-byte palettes (256 entries of 6-bit red, green and blue) the game
// switches between. Game owns the one Palettes; everything else reaches it
// through ThePalettes(). A new Palettes has every palette empty: main() sizes
// title_palette() and Init_Game() the others, and Uninit_Game() empties them
// again.
//
// Example:
//   Set_Palette(ThePalettes().black_palette());
class Palettes {
 public:
  Palettes() = default;
  ~Palettes() = default;

  Palettes(const Palettes&) = delete;
  Palettes& operator=(const Palettes&) = delete;
  Palettes(Palettes&&) = delete;
  Palettes& operator=(Palettes&&) = delete;

  // The palette the map is drawn in: the theater palette with the player's
  // brightness, color and tint applied.
  std::vector<unsigned char>& game_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return game_palette_;
  }
  // The theater palette before those adjustments.
  std::vector<unsigned char>& original_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return original_palette_;
  }
  // The palette of the title screens, movies and other full-screen pictures.
  std::vector<unsigned char>& title_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return title_palette_;
  }
  // All black and all white, for fades.
  std::vector<unsigned char>& black_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return black_palette_;
  }
  std::vector<unsigned char>& white_palette() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return white_palette_;
  }

  // Whether movies set their palettes on vertical blank (the SlowPalette
  // option, on unless the config file turns it off).
  [[nodiscard]] bool slow_palette() const { return slow_palette_; }
  void set_slow_palette(bool slow_palette) { slow_palette_ = slow_palette; }

 private:
  std::vector<unsigned char> game_palette_;
  std::vector<unsigned char> original_palette_;
  std::vector<unsigned char> title_palette_;
  std::vector<unsigned char> black_palette_;
  std::vector<unsigned char> white_palette_;
  bool slow_palette_ = true;
};

// Returns the Palettes that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Palettes& ThePalettes() { return base::Installed<Palettes>::Get(); }

#endif  // CNC_RED_ALERT_TD_PALETTES_H_
