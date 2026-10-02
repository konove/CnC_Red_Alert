// Pins what TextStyleFor() draws with -- font, spacing, glyph palette,
// colour and the flags after the font's fixups -- for every font and a
// spread of style flags, so the TextPrintType split
// (docs/TEXT_PRINT_SPLIT_PLAN.md) can show it changed nothing.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/flags.h"
#include "base/installed.h"
#include "base/numeric.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/assets.h"
#include "ra/core/defines.h"
#include "ra/ui/dialog.h"

namespace ra {
namespace {

struct NamedFont {
  std::string_view name;
  FontType font;
};

struct NamedFlags {
  std::string_view name;
  TextPrintType flags;
};

constexpr NamedFont kFonts[] = {
    {"6POINT", FontType::k6Point}, {"8POINT", FontType::k8Point},
    {"3POINT", FontType::k3Point}, {"LED", FontType::kLed},
    {"VCR", FontType::kVcr},       {"6PT_GRAD", FontType::k6PointGradient},
    {"MAP", FontType::kMap},       {"METAL12", FontType::kMetal12},
    {"EFNT", FontType::kEditor},   {"TYPE", FontType::kType},
    {"SCORE", FontType::kScore},
};

constexpr NamedFlags kShadows[] = {
    {"-", TPF_NONE},
    {"NOSHADOW", TPF_NOSHADOW},
    {"DROPSHADOW", TPF_DROPSHADOW},
    {"FULLSHADOW", TPF_FULLSHADOW},
    {"LIGHTSHADOW", TPF_LIGHTSHADOW},
};

constexpr NamedFlags kColors[] = {
    {"-", TPF_NONE},
    {"GRAD", TPF_USE_GRAD_PAL},
    {"MEDIUM", TPF_MEDIUM_COLOR},
    {"BRIGHT", TPF_BRIGHT_COLOR},
    {"GRAD|MEDIUM", TPF_USE_GRAD_PAL | TPF_MEDIUM_COLOR},
    {"GRAD|BRIGHT", TPF_USE_GRAD_PAL | TPF_BRIGHT_COLOR},
};

constexpr NamedFlags kJustify[] = {
    {"-", TPF_NONE}, {"CENTER", TPF_CENTER}, {"RIGHT", TPF_RIGHT}};

constexpr int kBack = 0x0C;

// FNV-1a: a stable 64-bit fingerprint of the whole table.
uint64_t Fingerprint(std::string_view text) {
  uint64_t hash = 0xCBF29CE484222325U;
  for (const char c : text) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 0x100000001B3U;
  }
  return hash;
}

class TextStyleTest : public testing::Test {
 protected:
  TextStyleTest() {
    // A distinct one-byte "font" per type, so the chosen font is known by
    // its address. FontView reads no header from so short a buffer.
    for (const FontType type : magic_enum::enum_values<FontType>()) {
      font_bytes_.at(type).assign(1, std::byte{0});
      assets_.set_font(type, font_bytes_.at(type));
    }
    remap_.Color = 0x40;
    remap_.BrightColor = 0x41;
    remap_.Bright = 0x42;
    for (int i = 0; i < 16; ++i) {
      base::At(remap_.FontRemap, i) = static_cast<unsigned char>(0x50 + i);
    }
  }

  [[nodiscard]] std::string_view FontName(
      std::span<const std::byte> font) const {
    for (const FontType type : magic_enum::enum_values<FontType>()) {
      if (font.data() == font_bytes_.at(type).data()) {
        return magic_enum::enum_name(type);
      }
    }
    return "?";
  }

  [[nodiscard]] std::string Describe(const TextStyle& style) const {
    std::string palette;
    for (const uint8_t entry : style.font_style.palette) {
      absl::StrAppendFormat(&palette, "%02x", entry);
    }
    return absl::StrFormat("font=%s x=%d y=%d fore=%d flags=%#x pal=%s",
                           FontName(style.font_style.font.data()),
                           style.font_style.x_spacing,
                           style.font_style.y_spacing, style.forecolor,
                           base::Bits(style.flag), palette);
  }

  std::string Table() {
    std::string table;
    for (const NamedFont& font : kFonts) {
      for (const NamedFlags& shadow : kShadows) {
        for (const NamedFlags& color : kColors) {
          for (const NamedFlags& justify : kJustify) {
            const TextPrintType flags =
                shadow.flags | color.flags | justify.flags;
            const TextStyle style =
                TextStyleFor(font.font, flags, &remap_, kBack);
            absl::StrAppend(&table, font.name, " ", shadow.name, " ",
                            color.name, " ", justify.name, " -> ",
                            Describe(style), "\n");
          }
        }
      }
    }
    return table;
  }

  base::EnumArray<FontType, std::vector<std::byte>> font_bytes_;
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  RemapControlType remap_{};
};

// The table of 1f60381e, before the split, less its rows for a print with no
// font, which the split leaves no way to ask for; the other 990 rows are
// byte for byte the same (docs/TEXT_PRINT_SPLIT_PLAN.md, Task 4). If this
// changes, some text draws differently: diff the printed table against the
// one this commit prints.
constexpr uint64_t kGoldenFingerprint = 0xEE606E09CAAC3CFB;

TEST_F(TextStyleTest, EveryFontAndFlagDrawsAsBeforeTheSplit) {
  const std::string table = Table();
  EXPECT_EQ(Fingerprint(table), kGoldenFingerprint) << table;
}

TEST_F(TextStyleTest, ThreePointFontDropsTheShadowsItHasNoRoomFor) {
  const TextStyle style = TextStyleFor(
      FontType::k3Point, TPF_FULLSHADOW | TPF_CENTER, &remap_, kBack);
  EXPECT_FALSE(base::Any(style.flag & TPF_FULLSHADOW));
  EXPECT_TRUE(base::Any(style.flag & TPF_CENTER));
}

TEST(HasGradientFontBitsTest, MatchesWhatTheLegacyMaskMatched) {
  // `flags & TPF_6PT_GRAD` (6) was true for every font index with bit 1 or 2
  // set: 2-7, 10 and 11.
  for (const FontType font :
       {FontType::k8Point, FontType::k3Point, FontType::kLed, FontType::kVcr,
        FontType::k6PointGradient, FontType::kMap, FontType::kType,
        FontType::kScore}) {
    EXPECT_TRUE(HasGradientFontBits(font)) << magic_enum::enum_name(font);
  }
  for (const FontType font :
       {FontType::k6Point, FontType::kMetal12, FontType::kEditor}) {
    EXPECT_FALSE(HasGradientFontBits(font)) << magic_enum::enum_name(font);
  }
}

}  // namespace
}  // namespace ra
