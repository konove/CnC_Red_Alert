// How Assets reads its data out of the MIX archives.

#include "ra/assets.h"

#include <span>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/defines.h"
#include "ra/ini.h"
#include "sdllib/font.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"

namespace {

// The file each font comes from, in FontType order. TYPE.FNT was dropped
// during development and the 8 point font took its place (V.Grippi, 1996).
constexpr base::EnumArray<FontType, const char*> kFontFiles = {
    "3POINT.FNT",    // k3Point
    "6POINT.FNT",    // k6Point
    "GRAD6FNT.FNT",  // k6PointGradient
    "8POINT.FNT",    // k8Point
    "EDITFNT.FNT",   // kEditor
    "LED.FNT",       // kLed
    "HELP.FNT",      // kMap
    "12METFNT.FNT",  // kMetal12
    "SCOREFNT.FNT",  // kScore
    "8POINT.FNT",    // kType
    "VCR.FNT",       // kVcr
};

}  // namespace

Assets::Assets() {
  for (SpeechSlot& slot : speech_slots_) {
    slot.buffer.resize(kSpeechBufferSize);
  }
}

Assets::~Assets() = default;

void Assets::LoadFonts() {
  for (const FontType type : magic_enum::enum_values<FontType>()) {
    fonts_.at(type) = MixArchive::RetrieveData(kFontFiles.at(type));
  }
  Set_Font(font(FontType::k8Point));
}

void Assets::LoadStrings() {
  // The .ENG suffix is the same in every language build: localized releases
  // ship a translated CONQUER.ENG under the same name.
  system_strings_ = MixArchive::RetrieveData("CONQUER.ENG");
  debug_strings_ = MixArchive::RetrieveData("DEBUG.ENG");
}

void Assets::LoadTutorialText() {
  INIClass ini;
  if (const auto file = OpenGameFile("TUTORIAL.INI")) {
    ini.Load(*file);
  }

  for (int index = 0; index < kTutorialTextCount; ++index) {
    char buffer[128];
    char entry[10];
    absl::SNPrintF(entry, sizeof(entry), "%d", index);
    std::string& text = base::At(std::span(tutorial_text_), index);
    if (ini.Get_String("Tutorial", entry, "", buffer, sizeof(buffer))) {
      text = std::string_view(buffer);
    } else {
      text.clear();
    }
  }
}

const char* Assets::tutorial_text(const int index) const {
  if (index < 0 || index >= kTutorialTextCount) {
    return nullptr;
  }
  const std::string& text = base::At(std::span(tutorial_text_), index);
  return text.empty() ? nullptr : text.c_str();
}
