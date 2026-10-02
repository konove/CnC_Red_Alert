# TextPrintType Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development
> (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split each game's `TextPrintType` into the font it selects and the style flags it sets, so
every text printer and text widget takes `(FontType font, TextPrintType flags)` as two parameters.

**Architecture:** The font half already exists: both games' `Assets` index their fonts by
`FontType`, which the low four `TPF_` bits map onto one to one. `TextPrintType` keeps only the
shadow, justification and colour flags. The migration is compiler-driven: a characterization test
pins what every font and flag combination draws with, a rewriter splits the literal `TPF_` arguments
at call sites, and the compiler finds the stored combined values for hand edits. The printers move
first, one game per commit, through two transitional helpers (`FontOf`/`StyleOf`); the widgets
follow in one commit for both games, because the `game::` widget templates are shared; the font
enumerators go last, one game per commit.

**Tech Stack:** C++23, GoogleTest, Python 3 (the rewriter), the strict clang build
(`cmake-build-strict-ra-clang`), the GCC build (`build/`).

**Spec:** the user's request of 2026-10-02 ("this needs to be split into two enums", choosing two
parameters over a combined value type); this document is the spec.

## Global Constraints

- Every commit builds with GCC (`build/`) and with the strict clang build, and passes `ctest` in
  both.
- One commit per game where the code allows it (Tasks 1, 2, 4, 5, 7, 8); Task 6 is one commit
  because `game/ui` is shared.
- Rendering is unchanged: the golden fingerprints of Tasks 1 and 2 hold, unedited, through Task 8.
- No commit trailers. Commit messages follow `git log` (imperative subject, prose body).
- New names follow the Google style (`FontStyleFor`, `kTextFont`); untouched legacy names stay.
- Format with `git clang-format -f -- <files>`, never `clang-format -i`.
- Strict build: `cmake --build cmake-build-strict-ra-clang --parallel 14 -- -k 0`, in the
  foreground. GCC build: `cmake --build build --parallel 22`.

## Review Focus

1. **A font bit left in the flags.** While the font enumerators still exist (Tasks 4-6), a hand edit
   can pass `TPF_6PT_GRAD | TPF_NOSHADOW` as the flags and the font bits are silently ignored. Tasks
   4 and 5 add a `DCHECK` in `TextFontStyle()`/`TextStyleFor()`; Task 7/8 delete the enumerators,
   which turns any survivor into a compile error.
2. **No font given.** A legacy value without font bits (`TPF_CENTER` alone, or `TPF_LASTPOINT`)
   printed in the 8-point font. The rewriter and `FontOf()` both map it to `FontType::k8Point`; the
   golden tables include the `LASTPOINT` row.
3. **Two fonts in one expression.** `kTpfText | TPF_8POINT` ORs two font indices into a third font.
   The rewriter refuses such an argument and reports it; decide by hand which font the code meant.
4. **German and WOL builds.** `TPF_TYPE` spacing depends on `config::kWolapiEnabled` and
   `config::kIsGerman`; the RA test applies the language setting, so it pins the configured build.
5. **Widgets that reformat their flags.** `UiSkin::Edit_Flags()` strips `TPF_CENTER` (RA) and the
   list/drop widgets OR in extra flags; the font must survive beside them. Task 6 Step 1 tests it.

---

## Findings

Measured 2026-10-02 on `1f60381e`.

`TextPrintType` (`ra/core/defines.h`, `td/core/defines.h`) packs two things into one flag enum: a
font index in the low four bits (`TPF_LASTPOINT` = 0 through `TPF_SCORE` = 0xB in RA,
`TPF_GREEN12_GRAD` = 9 in TD) and the style flags above them (shadow 0x10-0x80, justification
0x100-0x200, colour 0x1000-0x4000). `TPF_LASTPOINT` and `TPF_LASTSHADOW` are both 0 and no caller
uses either. Each printer recovers the font with `flag & static_cast<TextPrintType>(0x000F)`.

| `TPF_` font  | RA `FontType`        | TD `FontType`        |
| ------------ | -------------------- | -------------------- |
| LASTPOINT    | `k8Point` (fallback) | `k8Point` (fallback) |
| 6POINT       | `k6Point`            | `k6Point`            |
| 8POINT       | `k8Point`            | `k8Point`            |
| 3POINT       | `k3Point`            | `k3Point`            |
| LED          | `kLed`               | `kLed`               |
| VCR          | `kVcr`               | `kVcr`               |
| 6PT_GRAD     | `k6PointGradient`    | `k6PointGradient`    |
| MAP          | `kMap`               | `kMap`               |
| METAL12      | `kMetal12`           | —                    |
| EFNT         | `kEditor`            | —                    |
| TYPE         | `kType`              | —                    |
| SCORE        | `kScore`             | —                    |
| GREEN12      | —                    | `kGreen12`           |
| GREEN12_GRAD | —                    | `kGreen12Gradient`   |

`FontType` is declared in `ra/assets/assets.h` and `td/assets/assets.h`.

Uses across `src/ra`, `src/td` and `src/game`: 965 font constants (573 `TPF_6PT_GRAD`, 183
`TPF_EFNT`), 1,925 style flags, 311 declarations of `TextPrintType`/`PrintFlags` in 67 files.
Printer calls: 475 `Fancy_Text_Print`, 78 `TextFontStyle`, 64 `Conquer_Clip_Text_Print`, 33
`Plain_Text_Print`, 15 `Simple_Text_Print`, 6 `TextStyleFor`. RA's combined presets `kTpfButton`
(114 uses), `kTpfEButton` (53) and `kTpfText` (226) live in `ra/core/defines.h`; `kMenuTextFlags`
(5) in `ra/screens/menus.cc`.

Stored combined values: the `game::` widget templates (`TextButtonClass`, `EditClass`, `ListClass`,
`TextLabelClass`, in `game/ui/`) keep a `Skin::PrintFlags` and hand it to each game's `UiSkin`
(`ra/ui/ui_skin.h`, `td/ui/ui_skin.h`); the games' own widgets (`DropListClass`, `TDropListClass`,
`CheckListClass`, `ColorListClass`, `IconListClass`, `WOLEditClass`, `PassEditClass`,
`BigCheckBoxClass`, `StaticButtonClass`, `TListClass<T>`, `EgoClass`, `MessageListClass`) and the
editor's `Draw_It(..., TextPrintType flags)` members (`TEventClass`, `TActionClass`, `TriggerClass`,
`TriggerTypeClass`, `TeamTypeClass`, `SessionClass`) take or keep one too. The editor's `Draw_It`
members compare the font bits by hand (`ra/ai/tevent.cc:84`, `ra/ai/trigtype.cc:120`,
`ra/ai/taction.cc:222`, `ra/ai/trigger.cc:124`, `ra/ai/teamtype.cc:155,603`), as does
`ra/net/session.cc:1286`.

Nothing serializes a `TextPrintType`; the split does not touch save games or packets.

## Plan

### Task 1: Red Alert groundwork and golden test

**Files:**

- Modify: `src/ra/core/defines.h` (add `FontType` above `TextPrintType`, add `TPF_NONE`)
- Modify: `src/ra/assets/assets.h` (drop `FontType`, add `set_font()`)
- Create: `src/ra/ui/text_style_test.cc`
- Modify: `src/ra/CMakeLists.txt` (register `ra_text_style_test`)

**Interfaces:**

- Produces: `ra::FontType` in `ra/core/defines.h`; `TPF_NONE = 0x0000`;
  `void Assets::set_font(FontType type, std::span<const std::byte> font)`; the golden constant
  `kGoldenFingerprint` in the test, which no later task edits.

- [ ] **Step 1: Move `FontType`.** Cut the enum and its comment from `ra/assets/assets.h` and paste
      it into `ra/core/defines.h` directly above the `TextPrintType` comment block, inside
      `namespace ra`. Reword the comment's second sentence to: "The low four bits of a TextPrintType
      select one; docs/TEXT_PRINT_SPLIT_PLAN.md moves that choice into a FontType parameter."
      `assets.h` already includes `ra/core/defines.h`.

- [ ] **Step 2: Add `TPF_NONE`.** In `TextPrintType`, replace
      `TPF_LASTSHADOW = 0x0000, // Use previous font palette.` with
      `TPF_NONE = 0x0000, // No style flags.` (no caller uses `TPF_LASTSHADOW`).

- [ ] **Step 3: Add the setter** next to `set_lightning_shapes()` in `ra/assets/assets.h`:

```cpp
  // Installs `font` as the font `type` names; for tests, which cannot
  // LoadFonts() without the archives.
  void set_font(FontType type, std::span<const std::byte> font) {
    fonts_.at(type) = font;
  }
```

- [ ] **Step 4: Write the golden test** `src/ra/ui/text_style_test.cc`:

```cpp
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
  TextPrintType font;  // Task 4 turns this into a FontType.
};

struct NamedFlags {
  std::string_view name;
  TextPrintType flags;
};

constexpr NamedFont kFonts[] = {
    {"LASTPOINT", TPF_LASTPOINT}, {"6POINT", TPF_6POINT},
    {"8POINT", TPF_8POINT},       {"3POINT", TPF_3POINT},
    {"LED", TPF_LED},             {"VCR", TPF_VCR},
    {"6PT_GRAD", TPF_6PT_GRAD},   {"MAP", TPF_MAP},
    {"METAL12", TPF_METAL12},     {"EFNT", TPF_EFNT},
    {"TYPE", TPF_TYPE},           {"SCORE", TPF_SCORE},
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

  std::string_view FontName(std::span<const std::byte> font) const {
    for (const FontType type : magic_enum::enum_values<FontType>()) {
      if (font.data() == font_bytes_.at(type).data()) {
        return magic_enum::enum_name(type);
      }
    }
    return "?";
  }

  std::string Describe(const TextStyle& style) const {
    std::string palette;
    for (const uint8_t entry : style.font_style.palette) {
      absl::StrAppendFormat(&palette, "%02x", entry);
    }
    // Only the style bits: before Task 7 the returned flags still carry the
    // font index.
    return absl::StrFormat(
        "font=%s x=%d y=%d fore=%d flags=%#x pal=%s",
        FontName(style.font_style.font.data()), style.font_style.x_spacing,
        style.font_style.y_spacing, style.forecolor,
        base::Bits(style.flag) & ~0xFU, palette);
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
                TextStyleFor(font.font | flags, &remap_, kBack);
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

// Captured from 1f60381e, before the split. If a later task changes it, the
// split changed what some text draws with: diff the printed table against
// the one this commit prints.
constexpr uint64_t kGoldenFingerprint = 0x0;

TEST_F(TextStyleTest, EveryFontAndFlagDrawsAsBeforeTheSplit) {
  const std::string table = Table();
  EXPECT_EQ(Fingerprint(table), kGoldenFingerprint) << table;
}

TEST_F(TextStyleTest, NoFontSelectsTheEightPointFont) {
  const TextStyle style = TextStyleFor(TPF_NOSHADOW, &remap_, kBack);
  EXPECT_EQ(FontName(style.font_style.font.data()), "k8Point");
}

TEST_F(TextStyleTest, ThreePointFontDropsTheShadowsItHasNoRoomFor) {
  const TextStyle style =
      TextStyleFor(TPF_3POINT | TPF_FULLSHADOW | TPF_CENTER, &remap_, kBack);
  EXPECT_FALSE(base::Any(style.flag & TPF_FULLSHADOW));
  EXPECT_TRUE(base::Any(style.flag & TPF_CENTER));
}

}  // namespace
}  // namespace ra
```

- [ ] **Step 5: Register it** in `src/ra/CMakeLists.txt`, after the `ra_options_test` block:

```cmake
add_gtest(
    NAME ra_text_style_test
    SOURCES ui/text_style_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_text_style_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_text_style_test)
apply_language_setting(ra_text_style_test)
```

- [ ] **Step 6: Capture the golden value.** Build and run:
      `cmake --build build --parallel 22 --target ra_text_style_test && build/src/ra/ra_text_style_test`.
      Expected: `EveryFontAndFlagDrawsAsBeforeTheSplit` FAILS printing the actual fingerprint; the
      other two PASS. Read the table it printed: the `LASTPOINT` rows must show `font=k8Point`, the
      `6PT_GRAD` rows `font=k6PointGradient`, the `3POINT ... FULLSHADOW` rows no `0x40` in `flags`.
      Paste the printed fingerprint into `kGoldenFingerprint` and rerun: all three PASS.

- [ ] **Step 7: Verify.** `git clang-format -f -- <touched files>`; strict build; `ctest` in both
      build dirs. The strict build re-analyzes most of RA (`defines.h` changed).

- [ ] **Step 8: Commit** `Pin Red Alert's text styles before splitting TextPrintType`.

### Task 2: Tiberian Dawn groundwork and golden test

**Files:**

- Modify: `src/td/core/defines.h`, `src/td/assets/assets.h`, `src/td/CMakeLists.txt`
- Create: `src/td/ui/text_style_test.cc`

**Interfaces:**

- Produces: `td::FontType` in `td/core/defines.h`; `TPF_NONE`; `Assets::set_font()`; the TD
  `kGoldenFingerprint`.

- [ ] **Step 1:** Steps 1-3 of Task 1 for `td/` (`TPF_LASTSHADOW` becomes `TPF_NONE`; `set_font()`
      writes `fonts_`, which `font()` reads).

- [ ] **Step 2: Write the golden test** `src/td/ui/text_style_test.cc`. It is the RA test with these
      differences, written out in full:

```cpp
// Pins what TextStyleFor() draws with -- font, spacing, glyph palette,
// colour and the flags after the font's fixups -- for every font and a
// spread of style flags and colours, so the TextPrintType split
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
#include "td/assets/assets.h"
#include "td/core/defines.h"
#include "td/ui/dialog.h"

namespace td {
namespace {

struct NamedFont {
  std::string_view name;
  TextPrintType font;  // Task 5 turns this into a FontType.
};

struct NamedFlags {
  std::string_view name;
  TextPrintType flags;
};

constexpr NamedFont kFonts[] = {
    {"LASTPOINT", TPF_LASTPOINT},       {"6POINT", TPF_6POINT},
    {"8POINT", TPF_8POINT},             {"3POINT", TPF_3POINT},
    {"LED", TPF_LED},                   {"VCR", TPF_VCR},
    {"6PT_GRAD", TPF_6PT_GRAD},         {"MAP", TPF_MAP},
    {"GREEN12", TPF_GREEN12},           {"GREEN12_GRAD", TPF_GREEN12_GRAD},
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

// TD colours are palette indices; its gradient tables are indexed by
// fore % 16, so three rows of them are exercised.
constexpr int kFores[] = {1, 5, 11};
constexpr int kBack = 0x0C;

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
    for (const FontType type : magic_enum::enum_values<FontType>()) {
      font_bytes_.at(type).assign(1, std::byte{0});
      assets_.set_font(type, font_bytes_.at(type));
    }
  }

  std::string_view FontName(std::span<const std::byte> font) const {
    for (const FontType type : magic_enum::enum_values<FontType>()) {
      if (font.data() == font_bytes_.at(type).data()) {
        return magic_enum::enum_name(type);
      }
    }
    return "?";
  }

  std::string Describe(const TextStyle& style) const {
    std::string palette;
    for (const uint8_t entry : style.font_style.palette) {
      absl::StrAppendFormat(&palette, "%02x", entry);
    }
    return absl::StrFormat(
        "font=%s x=%d y=%d fore=%d flags=%#x pal=%s",
        FontName(style.font_style.font.data()), style.font_style.x_spacing,
        style.font_style.y_spacing, style.forecolor,
        base::Bits(style.flag) & ~0xFU, palette);
  }

  std::string Table() {
    std::string table;
    for (const NamedFont& font : kFonts) {
      for (const int fore : kFores) {
        for (const NamedFlags& shadow : kShadows) {
          for (const NamedFlags& color : kColors) {
            for (const NamedFlags& justify : kJustify) {
              const TextPrintType flags =
                  shadow.flags | color.flags | justify.flags;
              const TextStyle style =
                  TextStyleFor(font.font | flags, fore, kBack);
              absl::StrAppend(&table, font.name, " fore", fore, " ",
                              shadow.name, " ", color.name, " ",
                              justify.name, " -> ", Describe(style), "\n");
            }
          }
        }
      }
    }
    return table;
  }

  base::EnumArray<FontType, std::vector<std::byte>> font_bytes_;
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
};

// Captured from 1f60381e, before the split. If a later task changes it, the
// split changed what some text draws with: diff the printed table against
// the one this commit prints.
constexpr uint64_t kGoldenFingerprint = 0x0;

TEST_F(TextStyleTest, EveryFontAndFlagDrawsAsBeforeTheSplit) {
  const std::string table = Table();
  EXPECT_EQ(Fingerprint(table), kGoldenFingerprint) << table;
}

TEST_F(TextStyleTest, NoFontSelectsTheEightPointFont) {
  const TextStyle style = TextStyleFor(TPF_NOSHADOW, 5, kBack);
  EXPECT_EQ(FontName(style.font_style.font.data()), "k8Point");
}

TEST_F(TextStyleTest, ThreePointFontDropsTheShadowsItHasNoRoomFor) {
  const TextStyle style =
      TextStyleFor(TPF_3POINT | TPF_FULLSHADOW | TPF_CENTER, 5, kBack);
  EXPECT_FALSE(base::Any(style.flag & TPF_FULLSHADOW));
  EXPECT_TRUE(base::Any(style.flag & TPF_CENTER));
}

}  // namespace
}  // namespace td
```

If `Assets` in TD needs more than the default constructor for `TextStyleFor()` (it should not:
`TextFontStyle()` reads only `font()`), install what the failure names, the way `td/ui/list_test.cc`
does.

- [ ] **Step 3: Register it** in `src/td/CMakeLists.txt` after the `td_archive_roundtrip_test`
      block:

```cmake
add_gtest(
    NAME td_text_style_test
    SOURCES ui/text_style_test.cc startup/startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_text_style_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_text_style_test)
```

- [ ] **Step 4:** Capture the golden value as in Task 1 Step 6 (`build/src/td/td_text_style_test`);
      check that the `LASTPOINT` rows show `font=k8Point` and `GREEN12_GRAD` rows
      `font=kGreen12Gradient`.

- [ ] **Step 5:** Verify as in Task 1 Step 7, and commit
      `Pin Tiberian Dawn's text styles before splitting TextPrintType`.

### Task 3: The call-site rewriter

**Files:**

- Create: `tools/split_text_print.py`, `tools/split_text_print_test.py`

**Interfaces:**

- Produces: `tools/split_text_print.py --game ra|td --callees NAME[,NAME...] FILE...`. It rewrites
  in place and prints `path:line: not rewritten: <argument>` for every argument of a named callee
  that mentions a `TPF_` constant or a preset but could not be split. Exit status 0.

- [ ] **Step 1: Write the failing tests** `tools/split_text_print_test.py`:

```python
#!/usr/bin/env python3
"""Tests for tools/split_text_print.py. Run: tools/split_text_print_test.py"""

import unittest

import split_text_print as stp


def rewrite(text, callees=("Fancy_Text_Print",), game="ra"):
    return stp.rewrite_text(text, game, set(callees), "f.cc")


class RewriteTest(unittest.TestCase):
    def test_splits_font_from_flags(self):
        out, notes = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_6PT_GRAD | TPF_NOSHADOW);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::k6PointGradient, TPF_NOSHADOW);")
        self.assertEqual(notes, [])

    def test_font_alone_gets_tpf_none(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_EFNT);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::kEditor, TPF_NONE);")

    def test_flags_alone_get_the_eight_point_font(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_CENTER|TPF_NOSHADOW);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::k8Point, TPF_CENTER | TPF_NOSHADOW);")

    def test_presets_split_into_their_new_names(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, kTpfText | TPF_CENTER);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, kTextFont, kTextFlags | TPF_CENTER);")

    def test_multiline_and_casts(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0,\n    static_cast<TextPrintType>(TPF_8POINT | TPF_DROPSHADOW), a);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0,\n    FontType::k8Point, TPF_DROPSHADOW, a);")

    def test_declarations_and_new(self):
        out, _ = rewrite("TextButtonClass ok(1, TXT_OK, TPF_EFNT | TPF_CENTER, 2, 3);\n"
                         "p = new TextButtonClass(1, t, TPF_6POINT, 2, 3);",
                         callees=("TextButtonClass",))
        self.assertEqual(out, "TextButtonClass ok(1, TXT_OK, FontType::kEditor, TPF_CENTER, 2, 3);\n"
                              "p = new TextButtonClass(1, t, FontType::k6Point, TPF_NONE, 2, 3);")

    def test_reports_what_it_cannot_split(self):
        src = ("Fancy_Text_Print(v, t, x, y, &c, 0, flags | TPF_CENTER);\n"
               "Fancy_Text_Print(v, t, x, y, &c, 0, kTpfText | TPF_8POINT);\n"
               "Fancy_Text_Print(v, t, x, y, &c, 0, on ? TPF_6POINT : TPF_8POINT);")
        out, notes = rewrite(src)
        self.assertEqual(out, src)
        self.assertEqual([n.split(":")[1] for n in notes], ["1", "2", "3"])

    def test_leaves_other_callees_and_strings_alone(self):
        src = 'Other(TPF_6POINT); Fancy_Text_Print(v, "TPF_6POINT, x", x, y, &c, 0, flags);'
        out, notes = rewrite(src)
        self.assertEqual(out, src)
        self.assertEqual(notes, [])

    def test_td_fonts(self):
        out, _ = rewrite("Fancy_Text_Print(t, x, y, 5, 0, TPF_GREEN12_GRAD | TPF_FULLSHADOW);", game="td")
        self.assertEqual(out, "Fancy_Text_Print(t, x, y, 5, 0, FontType::kGreen12Gradient, TPF_FULLSHADOW);")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run** `PYTHONDONTWRITEBYTECODE=1 python3 tools/split_text_print_test.py`. Expected:
      `ModuleNotFoundError: No module named 'split_text_print'`.

- [ ] **Step 3: Implement** `tools/split_text_print.py`:

```python
#!/usr/bin/env python3
"""Splits literal TextPrintType arguments into a FontType and style flags.

docs/TEXT_PRINT_SPLIT_PLAN.md moves the font out of TextPrintType. In each
call to, or construction of, a named callee, every argument made only of
TPF_ constants and presets joined by '|' (in parentheses or a
static_cast<TextPrintType>(...) or not) becomes "FontType::kX, <flags>".
Arguments that mention TPF_ but are not that simple are reported, not
touched; the compiler finds the rest.

Usage:
  tools/split_text_print.py --game ra --callees Fancy_Text_Print,... FILE...
"""

import argparse
import re
import sys

FONTS = {
    "ra": {
        "TPF_LASTPOINT": "k8Point", "TPF_6POINT": "k6Point", "TPF_8POINT": "k8Point",
        "TPF_3POINT": "k3Point", "TPF_LED": "kLed", "TPF_VCR": "kVcr",
        "TPF_6PT_GRAD": "k6PointGradient", "TPF_MAP": "kMap", "TPF_METAL12": "kMetal12",
        "TPF_EFNT": "kEditor", "TPF_TYPE": "kType", "TPF_SCORE": "kScore",
    },
    "td": {
        "TPF_LASTPOINT": "k8Point", "TPF_6POINT": "k6Point", "TPF_8POINT": "k8Point",
        "TPF_3POINT": "k3Point", "TPF_LED": "kLed", "TPF_VCR": "kVcr",
        "TPF_6PT_GRAD": "k6PointGradient", "TPF_MAP": "kMap",
        "TPF_GREEN12": "kGreen12", "TPF_GREEN12_GRAD": "kGreen12Gradient",
    },
}

# Combined presets and the font and flags constants that replace them.
PRESETS = {
    "ra": {
        "kTpfButton": ("kButtonFont", "kButtonFlags"),
        "kTpfEButton": ("kEButtonFont", "kEButtonFlags"),
        "kTpfText": ("kTextFont", "kTextFlags"),
    },
    "td": {},
}

STYLE = {"TPF_NONE", "TPF_NOSHADOW", "TPF_DROPSHADOW", "TPF_FULLSHADOW",
         "TPF_LIGHTSHADOW", "TPF_CENTER", "TPF_RIGHT", "TPF_MEDIUM_COLOR",
         "TPF_BRIGHT_COLOR", "TPF_USE_GRAD_PAL", "TPF_LASTSHADOW"}

CAST = re.compile(r"static_cast\s*<\s*TextPrintType\s*>")
SIMPLE = re.compile(r"^[\s\w|()]*$")


def skip_literal(text, i):
    """Returns the index after the string or char literal starting at i."""
    quote = text[i]
    i += 1
    while i < len(text) and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def skip_comment(text, i):
    if text.startswith("//", i):
        end = text.find("\n", i)
        return len(text) if end < 0 else end
    end = text.find("*/", i + 2)
    return len(text) if end < 0 else end + 2


def split_args(text, open_index):
    """Returns (close_index, [(start, end), ...]) of the top-level arguments
    of the bracket at open_index, or None if it never closes."""
    pairs = {"(": ")", "{": "}", "[": "]"}
    stack = [pairs[text[open_index]]]
    args, start, i = [], open_index + 1, open_index + 1
    while i < len(text):
        c = text[i]
        if c in "\"'":
            i = skip_literal(text, i)
            continue
        if text.startswith("//", i) or text.startswith("/*", i):
            i = skip_comment(text, i)
            continue
        if c in pairs:
            stack.append(pairs[c])
        elif c in ")}]":
            stack.pop()
            if not stack:
                args.append((start, i))
                return i, args
        elif c == "," and len(stack) == 1:
            args.append((start, i))
            start = i + 1
        i += 1
    return None


def split_arg(arg, game):
    """Returns "FontType::kX, flags" for a simple TPF argument, "" for an
    argument without TPF_ or presets, or None if it cannot be split."""
    fonts, presets = FONTS[game], PRESETS[game]
    words = set(re.findall(r"\w+", arg))
    if not any(w.startswith("TPF_") or w in presets for w in words):
        return ""
    body = CAST.sub("", arg)
    if not SIMPLE.match(body):
        return None
    tokens = re.findall(r"\w+", body)
    font, flags = None, []
    for token in tokens:
        if token in fonts:
            new = "FontType::" + fonts[token]
        elif token in presets:
            new = presets[token][0]
            flags.append(presets[token][1])
        elif token in STYLE:
            flags.append("TPF_NONE" if token == "TPF_LASTSHADOW" else token)
            continue
        else:
            return None
        if font is not None and font != new:
            return None
        font = new
    if font is None:
        font = "FontType::k8Point"
    flags = [f for f in flags if f != "TPF_NONE"] or ["TPF_NONE"]
    return f"{font}, {' | '.join(flags)}"


def rewrite_text(text, game, callees, path):
    names = "|".join(sorted(map(re.escape, callees), key=len, reverse=True))
    call = re.compile(r"\b(?:" + names + r")\b(?:\s*<[^<>;()]*>)?(?:\s*[*&]?\s*[A-Za-z_]\w*)?\s*([({])")
    out, notes, pos = [], [], 0
    for match in call.finditer(text):
        if match.start() < pos:
            continue
        parsed = split_args(text, match.start(1))
        if parsed is None:
            continue
        close, args = parsed
        out.append(text[pos:match.end(1)])
        cursor = match.end(1)
        for start, end in args:
            arg = text[start:end]
            stripped = arg.strip()
            new = split_arg(stripped, game)
            if new is None:
                line = text.count("\n", 0, start) + 1
                notes.append(f"{path}:{line}: not rewritten: {' '.join(stripped.split())}")
                new = ""
            if new:
                lead = arg[: len(arg) - len(arg.lstrip())]
                trail = arg[len(arg.rstrip()):]
                out.append(text[cursor:start] + lead + new + trail)
            else:
                out.append(text[cursor:end])
            cursor = end
        out.append(text[cursor:close])
        pos = close
    out.append(text[pos:])
    return "".join(out), notes


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game", choices=sorted(FONTS), required=True)
    parser.add_argument("--callees", required=True)
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()
    callees = set(args.callees.split(","))
    for path in args.files:
        with open(path, encoding="utf-8") as f:
            text = f.read()
        new, notes = rewrite_text(text, args.game, callees, path)
        for note in notes:
            print(note)
        if new != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(new)
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

Note: nested calls inside an argument (`Fancy_Text_Print(..., Foo(TPF_X))`) are not rescanned;
`pos = close` skips them. They show up as "not rewritten" and are rare.

- [ ] **Step 4: Run** the tests again. Expected: `OK` (9 tests). Fix the script, not the tests, on a
      failure.

- [ ] **Step 5: Dry run on the tree** to size the hand work, then discard it:
      `python3 tools/split_text_print.py --game ra --callees Fancy_Text_Print,Plain_Text_Print,Simple_Text_Print,Conquer_Clip_Text_Print,TextFontStyle,TextStyleFor $(git ls-files 'src/ra/*.cc' 'src/ra/*.h') | wc -l`
      then `git checkout -- src/`. Record the count in this document under Progress.

- [ ] **Step 6: Commit** `Add a rewriter for splitting TextPrintType arguments` (cpplint does not
      look at Python; nothing to build).

### Task 4: Red Alert printers take a font and flags

**Files:**

- Modify: `src/ra/ui/dialog.h`, `src/ra/ui/dialog.cc`, `src/ra/core/defines.h` (split presets),
  `src/ra/ui/ui_skin.cc`, `src/ra/ui/text_style_test.cc`, and every RA file the rewriter or the
  compiler touches.

**Interfaces:**

- Consumes: `FontType`, `TPF_NONE` (Task 1); `tools/split_text_print.py` (Task 3).
- Produces, in `ra/ui/dialog.h`:

```cpp
void Conquer_Clip_Text_Print(PixelView& view, const char* text, int x, int y,
                             RemapControlType* fore, int back = kTBlack,
                             FontType font = FontType::k8Point,
                             TextPrintType flags = TPF_DROPSHADOW,
                             int width = -1, std::span<const int> tabs = {});
void Fancy_Text_Print(PixelView& view, const char* text, int x, int y,
                      RemapControlType* fore, int back, FontType font,
                      TextPrintType flags,
                      absl::Span<const absl::FormatArg> args = {});
void Fancy_Text_Print(PixelView& view, int text, int x, int y,
                      RemapControlType* fore, int back, FontType font,
                      TextPrintType flags,
                      absl::Span<const absl::FormatArg> args = {});
// (the two variadic Fancy_Text_Print templates gain `FontType font,` before
// `TextPrintType flags` the same way, and forward it)
struct TextStyle {
  TextPrintType flag{};  // The style flags after the font's own fixups.
  int forecolor = 0;
  FontStyle font_style;
};
FontStyle TextFontStyle(FontType font, TextPrintType flags);
TextStyle TextStyleFor(FontType font, TextPrintType flags,
                       RemapControlType* fore = nullptr, int back = kTBlack);
void Simple_Text_Print(PixelView& view, const char* text, int x, int y,
                       RemapControlType* fore, int back, FontType font,
                       TextPrintType flags);
void Plain_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flags,
                      absl::Span<const absl::FormatArg> args = {});
void Plain_Text_Print(PixelView& view, const char* text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flags,
                      absl::Span<const absl::FormatArg> args = {});
// (and the two variadic Plain_Text_Print templates likewise)

// Transitional, deleted by Task 7: the font a combined TextPrintType selects
// (none selects the 8-point font) and its flags without the font bits.
FontType FontOf(TextPrintType combined);
TextPrintType StyleOf(TextPrintType combined);
```

- and in `ra/core/defines.h`, next to the old presets (which stay until Task 7 because widgets still
  take combined values):

```cpp
// Standard button, editor button and dialog text: font and style flags.
inline constexpr FontType kButtonFont = FontType::k6PointGradient;
inline constexpr TextPrintType kButtonFlags = TPF_CENTER | TPF_NOSHADOW;
inline constexpr FontType kEButtonFont = FontType::kEditor;
inline constexpr TextPrintType kEButtonFlags = TPF_CENTER | TPF_NOSHADOW;
inline constexpr FontType kTextFont = FontType::k6PointGradient;
inline constexpr TextPrintType kTextFlags = TPF_NOSHADOW;
```

- [ ] **Step 1: Update the test first.** In `ra/ui/text_style_test.cc` make `NamedFont::font` a
      `FontType` and set the table to
      `{"LASTPOINT", FontType::k8Point}, {"6POINT", FontType::k6Point}, {"8POINT", FontType::k8Point}, {"3POINT", FontType::k3Point}, {"LED", FontType::kLed}, {"VCR", FontType::kVcr}, {"6PT_GRAD", FontType::k6PointGradient}, {"MAP", FontType::kMap}, {"METAL12", FontType::kMetal12}, {"EFNT", FontType::kEditor}, {"TYPE", FontType::kType}, {"SCORE", FontType::kScore}`;
      call `TextStyleFor(font.font, flags, &remap_, kBack)`; the no-font test becomes
      `TextStyleFor(FontType::k8Point, TPF_NOSHADOW, ...)` and the 3-point test
      `TextStyleFor(FontType::k3Point, TPF_FULLSHADOW | TPF_CENTER, ...)`. Leave
      `kGoldenFingerprint` alone. Add:

```cpp
TEST(FontOfTest, MapsTheLegacyFontBits) {
  EXPECT_EQ(FontOf(TPF_LASTPOINT | TPF_CENTER), FontType::k8Point);
  EXPECT_EQ(FontOf(TPF_6PT_GRAD | TPF_NOSHADOW), FontType::k6PointGradient);
  EXPECT_EQ(FontOf(TPF_EFNT), FontType::kEditor);
  EXPECT_EQ(StyleOf(TPF_EFNT | TPF_CENTER | TPF_USE_GRAD_PAL),
            TPF_CENTER | TPF_USE_GRAD_PAL);
}
```

Building the test now fails to compile (no two-argument `TextStyleFor`); that is the red step.

- [ ] **Step 2: Change the declarations** in `dialog.h` to the signatures above and update their
      comments (the TextFontStyle comment: "Returns the font and spacing `font` and the shadow in
      `flags` select, with the identity palette: all that measuring text needs.").

- [ ] **Step 3: Change the definitions** in `dialog.cc`:
  - `SupportedTextFlags(FontType font, TextPrintType flags)` returns
    `font == FontType::k3Point ? flags & ~kShadowsTooBigFor3Point : flags`.
  - `TextFontStyle(FontType font, TextPrintType flags)` starts with
    `DCHECK_EQ(base::Bits(flags) & 0xFU, 0U) << "a font index in the style flags";`, then
    `switch (font)` over every `FontType` (no `default`), each case keeping today's `font =` and
    spacing lines; `FontType::k8Point` covers the old `TPF_8POINT` and `TPF_LASTPOINT`/default
    cases. `assets.font(font)` replaces the per-case `assets.font(FontType::kX)`.
  - `TextStyleFor(FontType font, TextPrintType flags, ...)` gets the same `DCHECK`;
    `point == TPF_VCR || ...` becomes
    `font == FontType::kVcr || font == FontType::k6PointGradient || font == FontType::kMetal12 || font == FontType::kEditor || font == FontType::kType`;
    `point == TPF_TYPE` and `point != TPF_TYPE` compare `font` with `FontType::kType`; it calls
    `TextFontStyle(font, flags)`.
  - `Simple_Text_Print`, `Fancy_Text_Print`, `Plain_Text_Print`, `Conquer_Clip_Text_Print` thread
    `font` through.
  - `FontOf`/`StyleOf`:

```cpp
FontType FontOf(const TextPrintType combined) {
  // Indexed by the low four bits; 0 (TPF_LASTPOINT) and the unused values
  // fall back to the 8-point font, as TextFontStyle() always did.
  static constexpr std::array<FontType, 16> kFontOfBits = {
      FontType::k8Point,  FontType::k6Point,         FontType::k8Point,
      FontType::k3Point,  FontType::kLed,            FontType::kVcr,
      FontType::k6PointGradient, FontType::kMap,     FontType::kMetal12,
      FontType::kEditor,  FontType::kType,           FontType::kScore,
      FontType::k8Point,  FontType::k8Point,         FontType::k8Point,
      FontType::k8Point};
  return kFontOfBits.at(base::Bits(combined) & 0xFU);
}

TextPrintType StyleOf(const TextPrintType combined) {
  return combined & ~static_cast<TextPrintType>(0xFU);
}
```

- [ ] **Step 4: Add the split presets** to `defines.h` (above).

- [ ] **Step 5: Rewrite the literal call sites:**
      `python3 tools/split_text_print.py --game ra --callees Fancy_Text_Print,Plain_Text_Print,Simple_Text_Print,Conquer_Clip_Text_Print,TextFontStyle,TextStyleFor $(git ls-files 'src/ra/*.cc' 'src/ra/*.h') > <scratch dir>/ra_notes.txt`
      (a file outside the repo). It also touches the declarations in `dialog.h` only if a default
      argument is a literal; check `git diff src/ra/ui/dialog.h` and restore the Step 2 text if it
      did.

- [ ] **Step 6: Fix the rest by hand, driven by the compiler.** Build `build/` with `-- -k 0` and
      work through the errors and `ra_notes.txt`:
  - A stored combined value passed to a printer (`TextFlags`, `PrintFlags`, `flags`, `style`): pass
    `FontOf(x), StyleOf(x)`. These are the widgets' values; Task 6 removes every `FontOf`.
  - A local combined value built from literals
    (`const TextPrintType style = TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW;` in
    `ra/map/radar.cc`): split it into `const FontType font = ...;` and
    `const TextPrintType style = ...;`.
  - `kMenuTextFlags` in `ra/screens/menus.cc`: add
    `constexpr FontType kMenuTextFont = FontType::k8Point;`, make `kMenuTextFlags = TPF_DROPSHADOW`,
    and pass both. Check that nothing else (a widget) receives `kMenuTextFlags`; if one does, give
    it `kMenuTextFont` too in Task 6 and keep a combined local until then.
  - `ra/net/session.cc:1286` and the editor `Draw_It` members compare the font bits of a combined
    value they received from a widget; leave them for Task 6.
  - `UiSkin` (`ra/ui/ui_skin.cc`) calls printers with its combined `PrintFlags`: `FontOf`/`StyleOf`.

- [ ] **Step 7: Run the tests.**
      `cmake --build build --parallel 22 && ctest --test-dir build -R 'ra_text_style|ra_drop'`.
      Expected: PASS, golden unchanged. Then the whole `ctest`.

- [ ] **Step 8: Verify** (format, strict build, both ctests) and commit
      `Pass Red Alert's printers the font apart from the flags`. The body gives the number of call
      sites the rewriter changed and the number of hand edits.

### Task 5: Tiberian Dawn printers take a font and flags

**Files:**

- Modify: `src/td/ui/dialog.h`, `src/td/ui/dialog.cc`, `src/td/ui/ui_skin.cc`,
  `src/td/ui/text_style_test.cc`, and every TD file the rewriter or compiler touches.

**Interfaces:**

- Produces, in `td/ui/dialog.h`: the RA signatures of Task 4 with TD's colour parameters (`int fore`
  where RA has `RemapControlType* fore`), i.e.
  `FontStyle TextFontStyle(FontType font, TextPrintType flags);`
  `TextStyle TextStyleFor(FontType font, TextPrintType flags, int fore = kTBlack, int back = kTBlack);`
  `void Simple_Text_Print(PixelView& view, const char* text, int x, int y, int fore, int back, FontType font, TextPrintType flags);`
  and `Fancy_Text_Print`, `Plain_Text_Print`, `Conquer_Clip_Text_Print` likewise (the last with
  `FontType font = FontType::k8Point, TextPrintType flags = TPF_DROPSHADOW`); plus the transitional
  `FontOf`/`StyleOf`, whose table for TD is
  `{k8Point, k6Point, k8Point, k3Point, kLed, kVcr, k6PointGradient, kMap, kGreen12, kGreen12Gradient, k8Point x6}`.

- [ ] **Step 1:** Update `td/ui/text_style_test.cc` as Task 4 Step 1 did for RA (TD font table:
      `LASTPOINT`→`k8Point`, `6POINT`→`k6Point`, `8POINT`→`k8Point`, `3POINT`→`k3Point`,
      `LED`→`kLed`, `VCR`→`kVcr`, `6PT_GRAD`→`k6PointGradient`, `MAP`→`kMap`, `GREEN12`→`kGreen12`,
      `GREEN12_GRAD`→`kGreen12Gradient`; calls `TextStyleFor(font.font, flags, fore, kBack)`), plus
      a `FontOfTest` with `FontOf(TPF_GREEN12_GRAD | TPF_CENTER) == FontType::kGreen12Gradient` and
      `FontOf(TPF_LASTPOINT) == FontType::k8Point`.
- [ ] **Step 2:** Change `td/ui/dialog.h` and `dialog.cc` as Task 4 Steps 2-3 did: the
      `(flag & static_cast<TextPrintType>(0xF)) == TPF_VCR` tests compare `font` with
      `FontType::kVcr`, `FontType::k6PointGradient`, `FontType::kGreen12Gradient`,
      `FontType::kGreen12`; the `DCHECK`s go in `TextFontStyle` and `TextStyleFor`.
- [ ] **Step 3:** Run the rewriter with `--game td` and the same callees over
      `$(git ls-files 'src/td/*.cc' 'src/td/*.h')`, then fix the rest by hand as in Task 4 Step 6.
- [ ] **Step 4:** Tests, verify, commit
      `Pass Tiberian Dawn's printers the font apart from the flags`.

### Task 6: Widgets and skins take a font and flags (both games)

**Files:**

- Modify: `src/game/ui/textbtn.h`, `edit.h`, `list.h`, `txtlabel.h`, `checkbox_test.cc`, and any
  other `game/ui` test skin; `src/ra/ui/ui_skin.{h,cc}`, `src/td/ui/ui_skin.{h,cc}`; the RA and TD
  widgets listed under Findings; the editor `Draw_It` members; every constructing call site.

**Interfaces:**

- Consumes: the two-parameter printers and `FontOf`/`StyleOf` (Tasks 4-5).
- Produces, in each `UiSkin`:

```cpp
  using Font = FontType;
  using PrintFlags = TextPrintType;  // Style flags only.
  static constexpr Font kButtonDefaultFont = FontType::k8Point;
  static constexpr PrintFlags kButtonDefaultFlags = TPF_NONE;
  static int Text_Width(Font font, PrintFlags flags, const char* text);
  static int Text_Height(Font font, PrintFlags flags);
  static void Draw_Button_Box(PixelView& view, int x, int y, int width,
                              int height, Font font, PrintFlags flags,
                              bool disabled, bool pressed);
  static void Print_Button_Text(PixelView& view, const char* text, int x,
                                int y, Font font, PrintFlags flags,
                                bool disabled, bool pressed, bool on);
  static FontStyle FontStyleFor(Font font, PrintFlags flags);  // was Font()
  static PrintFlags Edit_Flags(PrintFlags flags);              // unchanged
  static void Draw_Edit_Text(PixelView& view, const char* text, int x, int y,
                             int width, TextColor color, Font font,
                             PrintFlags flags, bool focused, int max_length);
  static void Draw_List_Entry(PixelView& view, const char* text, int x, int y,
                              int width, int line_height, Font font,
                              PrintFlags flags, bool selected,
                              std::span<const int> tabs);
  static void Print_Label(PixelView& view, const char* text, int x, int y,
                          TextColor color, Font font, PrintFlags style);
  static void Print_Clipped(PixelView& view, const char* text, int x, int y,
                            TextColor color, Font font, PrintFlags style,
                            int width);
```

- every widget constructor that took `PrintFlags flags`/`TextPrintType flags` takes
  `Skin::Font font, Skin::PrintFlags flags` (game widgets: `FontType font, TextPrintType flags`) in
  that position, and stores `Font`/`TextFont` beside its flags member (`TextButtonClass`:
  `Font TextFont; PrintFlags PrintFlags;`; `EditClass`/`ListClass`/game lists: `Font TextFont;`
  beside `TextFlags`; `TextLabelClass`: `Font TextFont;` beside `Style`).
  `Set_Style(Font, PrintFlags)` on `TextButtonClass`. The editor
  `Draw_It(..., FontType font, TextPrintType flags)`.

- [ ] **Step 1: Test skins first.** Give `CheckBoxTestSkin` in `game/ui/checkbox_test.cc` the new
      members (`using Font = int; static constexpr Font kButtonDefaultFont = 0;`, the extra `Font`
      parameters, `FontStyleFor` in place of `Font`), and add a test that an edit field keeps its
      font through `Edit_Flags()` (Review Focus 5), with `#include "game/ui/edit.h"`:

```cpp
// Records the font and flags an edit field measures its text with, and
// strips flag 1 in Edit_Flags() the way Red Alert's skin strips TPF_CENTER.
struct FontRecordingSkin : CheckBoxTestSkin {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static inline Font last_font = -1;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static inline PrintFlags last_flags = -1;
  static PrintFlags Edit_Flags(PrintFlags flags) { return flags & ~1; }
  static FontStyle FontStyleFor(Font font, PrintFlags flags) {
    last_font = font;
    last_flags = flags;
    return {};
  }
};

TEST(TextWidgetFontTest, EditFieldKeepsItsFontThroughEditFlags) {
  char text[8] = "abc";
  const EditClass<FontRecordingSkin> edit(1, text, sizeof(text), /*font=*/7,
                                          /*flags=*/3, 0, 0);
  EXPECT_EQ(FontRecordingSkin::last_font, 7);
  EXPECT_EQ(FontRecordingSkin::last_flags, 2);
}
```

It fails to compile until Step 2.

- [ ] **Step 2: game/ui templates.** Add the font members and parameters; every `Skin::` call passes
      `TextFont` before the flags; `Skin::Font(TextFlags)` becomes
      `Skin::FontStyleFor(TextFont, TextFlags)`.
- [ ] **Step 3: Both skins.** New signatures; their bodies pass `font, flags` straight to the
      printers, so the `FontOf`/`StyleOf` calls Tasks 4-5 put there go.
- [ ] **Step 4: Game widgets and editor `Draw_It`.** Same change; the font-bit comparisons
      (`(static_cast<uint32_t>(flags) & 0x0FU) == static_cast<uint32_t>(font)` and friends in
      `tevent.cc`, `trigtype.cc`, `taction.cc`, `trigger.cc`, `teamtype.cc`, `session.cc`) compare
      the `FontType` parameter instead.
- [ ] **Step 5: Rewrite the constructions** in each game:
      `python3 tools/split_text_print.py --game ra --callees TextButtonClass,EditClass,ListClass,TextLabelClass,DropListClass,TDropListClass,CheckListClass,ColorListClass,IconListClass,WOLEditClass,PassEditClass,BigCheckBoxClass,StaticButtonClass,TListClass,EgoClass,Add_Message,Add_Edit,Set_Style $(git ls-files 'src/ra/*.cc' 'src/ra/*.h')`,
      and with `--game td` and the TD class names
      (`TextButtonClass,EditClass,ListClass,TextLabelClass,CheckListClass,ColorListClass,Add_Message,Add_Edit,Set_Style`)
      over `src/td`. Then the compiler loop as in Task 4 Step 6; every remaining `FontOf`/`StyleOf`
      call is a widget value that now has its own font: remove it. When done,
      `git grep -n 'FontOf\|StyleOf' src/` lists only the definitions, their tests and comments.
- [ ] **Step 6: Presets.** RA call sites now use `kButtonFont, kButtonFlags` and friends; check with
      `git grep -nw 'kTpfButton\|kTpfEButton\|kTpfText' src/` that only the `defines.h` definitions
      remain, and delete them.
- [ ] **Step 7:** Tests (both golden tests unchanged), verify, commit
      `Pass the text widgets the font apart from the flags`. One commit: the `game::` templates are
      shared, so both skins change with them.

### Task 7: Delete Red Alert's font bits

**Files:**

- Modify: `src/ra/core/defines.h`, `src/ra/ui/dialog.{h,cc}`, `src/ra/ui/text_style_test.cc`, and
  whatever the compiler names.

- [ ] **Step 1:** Delete `TPF_LASTPOINT` through `TPF_SCORE` from `TextPrintType` and rename the
      enum's comment to "The style flags of Fancy_Text_Print(): shadow, justification and colour.
      The font is a separate FontType." Delete `FontOf`, `StyleOf`, their test and the two
      `DCHECK`s.
- [ ] **Step 2:** Build with `-- -k 0`. Remove the case labels of deleted enumerators from the
      switches in `dialog.cc` (each switch keeps its `default:`); fix any other survivor the
      compiler names. Each survivor is a font passed as a flag: give it to the `FontType` parameter.
- [ ] **Step 3:** In the test, `Describe()` prints `base::Bits(style.flag)` without the `& ~0xFU`
      mask and its comment goes. The golden value must not change.
- [ ] **Step 4:** Update `FontType`'s comment ("Fancy_Text_Print() and the text widgets take one
      beside their TextPrintType flags."). Verify, commit
      `Remove the font bits from Red Alert's TextPrintType`.

### Task 8: Delete Tiberian Dawn's font bits

- [ ] **Step 1-4:** Task 7 for `src/td/` (`TPF_LASTPOINT` through `TPF_GREEN12_GRAD`). Commit
      `Remove the font bits from Tiberian Dawn's TextPrintType`.

### Task 9: Close out

- [ ] **Step 1:**
      `git grep -nE 'TPF_(LASTPOINT|LASTSHADOW|6POINT|8POINT|3POINT|LED|VCR|6PT_GRAD|MAP|METAL12|EFNT|TYPE|SCORE|GREEN12)' src/`
      prints nothing; `git grep -n '0x000F\|0x0FU' src/ra src/td` shows no font masks.
- [ ] **Step 2: Display check** (human, real display): RA main menu, options dialog, a mission's
      sidebar and tooltips (`TPF_TYPE`), the map editor's trigger and team dialogs (`TPF_EFNT`), the
      score screen; TD main menu, map selection, the score screen. Text must look exactly as on
      `1f60381e`. Record the result below.
- [ ] **Step 3:** Write the Outcome section below (commits, sites changed, hand edits), and commit
      it with the plan's Progress updates.

## Verification

- Every task: the GCC build and `ctest --test-dir build`; the strict build with `-- -k 0` until
  ninja has no work; `ctest --test-dir cmake-build-strict-ra-clang`.
- The golden fingerprints of Tasks 1 and 2 never change after they are captured.
- Task 9's greps and display check.

## Progress

| Task | Status | Commit  |
| ---- | ------ | ------- |
| 1    | done   | a3ebfc7 |
| 2    | done   | 0ee07f6 |
| 3    | done   | (this)  |
| 4    |        |         |
| 5    |        |         |
| 6    |        |         |
| 7    |        |         |
| 8    |        |         |
| 9    |        |         |

Task 3 dry run of the printer callees: RA 36 files (232 lines) rewritten, 7 arguments reported; TD
29 files (226 lines), 7 reported. Golden fingerprints: RA `0xAC8A29F9D28546BC` (1,080 lines), TD
`0x780934A45B2F86E3` (2,700 lines).

## Outcome
