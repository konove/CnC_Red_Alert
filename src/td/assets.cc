// How Assets reads its data out of the game files.

#include "td/assets.h"

#include <span>
#include <vector>

#include "base/enum_array.h"
#include "magic_enum/magic_enum.hpp"
#include "sdllib/font.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/jshell.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"

namespace {

// Where each font comes from, in FontType order. The fonts the archives
// serve are read straight out of the cached data; the rest are loose files
// that have to be read into a buffer of their own.
struct FontSource {
  const char* file;
  bool from_archive;
};

constexpr base::EnumArray<FontType, FontSource> kFontSources = {
    FontSource{FONT3, true},            // k3Point
    FontSource{"6POINT.FNT", false},    // k6Point
    FontSource{"GRAD6FNT.FNT", false},  // k6PointGradient
    FontSource{FONT8, true},            // k8Point
    FontSource{"12GREEN.FNT", false},   // kGreen12
    FontSource{"12GRNGRD.FNT", false},  // kGreen12Gradient
    FontSource{"LED.FNT", true},        // kLed
    FontSource{"8FAT.FNT", false},      // kMap
    FontSource{"12GRNGRD.FNT", false},  // kScore
    FontSource{"VCR.FNT", true},        // kVcr
};

}  // namespace

Assets::Assets() { speech_buffer_.resize(SPEECH_BUFFER_SIZE); }

Assets::~Assets() = default;

void Assets::LoadFonts() {
  for (const FontType type : magic_enum::enum_values<FontType>()) {
    const FontSource& source = kFontSources.at(type);
    if (source.from_archive) {
      fonts_.at(type) = MixArchive::RetrieveData(source.file);
      continue;
    }
    GameFile file(source.file);
    font_data_.at(type) = LoadAllocData(file);
    fonts_.at(type) = font_data_.at(type);
  }
  Set_Font(font(FontType::k8Point));
}

void Assets::LoadStrings() {
  system_strings_ = MixArchive::RetrieveData(Language_Name("CONQUER"));
}
