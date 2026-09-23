// File: Assets, the game data Red Alert loads once from its MIX archives.

#ifndef CNC_RED_ALERT_RA_ASSETS_H_
#define CNC_RED_ALERT_RA_ASSETS_H_

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "base/installed.h"
#include "ra/defines.h"
#include "tech/pk.h"

class MixArchive;

// The fonts the game prints with. Fancy_Text_Print() picks one from the
// TextPrintType it is given; the names say what the font looks like, not
// what it is used for.
enum class FontType {
  k3Point,
  k6Point,
  k6PointGradient,
  k8Point,
  kEditor,  // The map editor's own font.
  kLed,     // Seven-segment digits, for the credits counter.
  kMap,     // The font of the mission briefing screen.
  kMetal12,
  kScore,  // The font of the score screen.
  kType,   // The typewriter font of the tooltips.
  kVcr,    // The font of the movie playback controls.
};

// The data that is loaded once, from the MIX archives, and read for the rest
// of the process: fonts, the string tables, the tutorial messages and the
// speech buffers. Game owns the one Assets; everything else reaches it
// through TheAssets().
//
// Constructing an Assets reads nothing -- the spans are empty and the string
// tables are blank until the Load* members run, which needs the archives to
// be registered and the ones that hold the data cached. A test builds one
// and fills in only what the code under test reads.
//
// The spans point into the cached data of the archives the files came from,
// so they stay valid only as long as those archives stay registered.
//
// Example:
//   const FontView font(TheAssets().font(FontType::kScore));
class Assets {
 public:
  // The number of tutorial messages TUTORIAL.INI can hold. The trigger
  // action that shows one stores the message number in a single byte.
  static constexpr int kTutorialTextCount = 225;

  // The speech holding tank buffers. Speech does not mix, so two buffers are
  // enough: one holds the voice being said and the other the one before it,
  // which is often said again soon.
  static constexpr int kSpeechSlotCount = 2;

  Assets();
  ~Assets();

  Assets(const Assets&) = delete;
  Assets& operator=(const Assets&) = delete;
  Assets(Assets&&) = delete;
  Assets& operator=(Assets&&) = delete;

  // Reads the fonts out of the archives and makes the 8 point font current.
  // Nothing can print text before this runs.
  void LoadFonts();

  // Reads the game's and the developer text tables. Text_String() serves
  // both.
  void LoadStrings();

  // Reads the tutorial messages from TUTORIAL.INI.
  void LoadTutorialText();

  // Returns the named font, or an empty span before LoadFonts().
  [[nodiscard]] std::span<const std::byte> font(FontType type) const {
    return fonts_.at(type);
  }

  // The game's text table, and the developer messages that Text_String()
  // serves under numbers from 1000 up.
  [[nodiscard]] std::span<const std::byte> system_strings() const {
    return system_strings_;
  }
  [[nodiscard]] std::span<const std::byte> debug_strings() const {
    return debug_strings_;
  }

  // The shapes of the chronosphere's lightning bolts, loaded with the
  // chronosphere's building type.
  [[nodiscard]] std::span<const std::byte> lightning_shapes() const {
    return lightning_shapes_;
  }
  void set_lightning_shapes(std::span<const std::byte> shapes) {
    lightning_shapes_ = shapes;
  }

  // Returns tutorial message `index`, or nullptr if TUTORIAL.INI has no
  // entry for it. The result stays valid until the next LoadTutorialText().
  [[nodiscard]] const char* tutorial_text(int index) const;

  // A speech holding tank: the buffer a voice is read into and the voice it
  // holds. `voice` is VOX_NONE when the buffer has never been filled.
  struct SpeechSlot {
    std::vector<std::byte> buffer;
    VoxType voice = VOX_NONE;
  };

  // The speech buffers, sized at construction and reused for every voice.
  std::span<SpeechSlot> speech_slots() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return speech_slots_;
  }

  // The archives that come off the CD, and so have to be freed and
  // registered again when the player changes disc. They are owned by the
  // archive registry, not by Assets; these are only the handles the disc
  // change needs. A null handle means the archive was never registered.
  struct DiscArchives {
    MixArchive* main = nullptr;
    MixArchive* general = nullptr;
    MixArchive* movies = nullptr;
    MixArchive* score = nullptr;
  };
  DiscArchives& disc_archives() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return disc_archives_;
  }

  // The public key the encrypted MIX archives are unlocked with, and whose
  // raw bytes also serve as the saved game's Blowfish key. Zero until
  // Init_Keys() reads it out of the built-in key table.
  [[nodiscard]] const PKey& mix_key() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return mix_key_;
  }
  void set_mix_key(const PKey& key) { mix_key_ = key; }

 private:
  base::EnumArray<FontType, std::span<const std::byte>> fonts_{};
  std::span<const std::byte> system_strings_;
  std::span<const std::byte> debug_strings_;
  std::span<const std::byte> lightning_shapes_;

  // Empty where TUTORIAL.INI has no message, which is also how a message the
  // file leaves blank reads, exactly as the original offset table worked.
  std::array<std::string, kTutorialTextCount> tutorial_text_;

  std::array<SpeechSlot, kSpeechSlotCount> speech_slots_;
  DiscArchives disc_archives_;
  PKey mix_key_;
};

// Returns the Assets that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Assets& TheAssets() { return base::Installed<Assets>::Get(); }

#endif  // CNC_RED_ALERT_RA_ASSETS_H_
