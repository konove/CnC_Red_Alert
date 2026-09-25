// File: Assets, the game data Tiberian Dawn loads once from its MIX archives.

#ifndef CNC_RED_ALERT_TD_ASSETS_H_
#define CNC_RED_ALERT_TD_ASSETS_H_

#include <cstddef>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "base/installed.h"

class MixArchive;

// The fonts the game prints with. Fancy_Text_Print() picks one from the
// TextPrintType it is given; the names say what the font looks like, not
// what it is used for.
enum class FontType {
  k3Point,
  k6Point,
  k6PointGradient,
  k8Point,
  kGreen12,
  kGreen12Gradient,
  kLed,    // Seven-segment digits, for the credits counter.
  kMap,    // The fat font of the map selection screen.
  kScore,  // The font of the score screen.
  kVcr,    // The font of the movie playback controls.
};

// The data that is loaded once, from the game files, and read for the rest of
// the process: the fonts, the string table and the speech buffer. Game owns
// the one Assets; everything else reaches it through TheAssets().
//
// Constructing an Assets reads nothing -- the fonts and the string table are
// empty until LoadFonts() and LoadStrings() run, which needs the archives to
// be registered. A test builds one and fills in only what the code under
// test reads.
//
// Six of the fonts are read into buffers Assets owns; the other four, and
// the string table, are spans into the cached data of the archive they came
// from, and stay valid only as long as that archive stays registered.
//
// Example:
//   const FontView font(TheAssets().font(FontType::kScore));
class Assets {
 public:
  Assets();
  ~Assets();

  Assets(const Assets&) = delete;
  Assets& operator=(const Assets&) = delete;
  Assets(Assets&&) = delete;
  Assets& operator=(Assets&&) = delete;

  // Reads the fonts and makes the 8 point font current. Nothing can print
  // text before this runs.
  void LoadFonts();

  // Reads the text table of the language this build was compiled for.
  void LoadStrings();

  // Returns the named font, or an empty span before LoadFonts().
  [[nodiscard]] std::span<const std::byte> font(FontType type) const {
    return fonts_.at(type);
  }

  // The game's text table, which Text_String() serves.
  [[nodiscard]] std::span<const std::byte> system_strings() const {
    return system_strings_;
  }

  // The speech holding tank. Speech does not mix, so one buffer, as large as
  // the largest speech file, is enough.
  std::vector<std::byte>& speech_buffer() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return speech_buffer_;
  }

  // The archives that come off the CD, and so have to be freed and
  // registered again when the player changes disc. They are owned by the
  // archive registry, not by Assets; these are only the handles the disc
  // change needs. A null handle means the archive was never registered.
  struct DiscArchives {
    MixArchive* general = nullptr;
    MixArchive* movies = nullptr;
    MixArchive* score = nullptr;
  };
  DiscArchives& disc_archives() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return disc_archives_;
  }

  // The overlay frames drawn over the weapons factory while it builds.
  // Empty until the building type data is loaded.
  std::span<const std::byte>& war_factory_overlay()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return war_factory_overlay_;
  }

 private:
  base::EnumArray<FontType, std::span<const std::byte>> fonts_{};
  // The fonts that are read from a file rather than served out of a cached
  // archive; the matching fonts_ entry points into these. Empty for the
  // fonts that come from an archive.
  base::EnumArray<FontType, std::vector<std::byte>> font_data_;

  std::span<const std::byte> system_strings_;
  std::vector<std::byte> speech_buffer_;
  DiscArchives disc_archives_;
  std::span<const std::byte> war_factory_overlay_;
};

// Returns the Assets that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Assets& TheAssets() { return base::Installed<Assets>::Get(); }

#endif  // CNC_RED_ALERT_TD_ASSETS_H_
