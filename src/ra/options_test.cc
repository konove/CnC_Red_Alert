// Tests for reading and writing the [WinHotkeys] bindings, which REDALERT.INI
// stores as Windows virtual-key codes, and for the palette sliders.

#include "ra/options.h"

#include "base/fixed.h"
#include "engine/gfx/hsv_color.h"
#include "engine/gfx/palette_color.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/ini.h"
#include "ra/palette.h"

namespace {

// Values from the [WinHotkeys] section of a Steam install's REDALERT.INI.
TEST(OptionsHotkeysTest, LoadsWindowsKeyCodes) {
  INIClass ini;
  ASSERT_TRUE(ini.Put_Int("WinHotkeys", "KeyForceMove1", 18));  // VK_MENU
  ASSERT_TRUE(ini.Put_Int("WinHotkeys", "KeyStop", 83));        // 'S'
  ASSERT_TRUE(ini.Put_Int("WinHotkeys", "KeyTeam1", 49));       // '1'
  ASSERT_TRUE(ini.Put_Int("WinHotkeys", "KeyHome2", 103));      // VK_NUMPAD7
  ASSERT_TRUE(ini.Put_Int("WinHotkeys", "KeyRepairOn", 0));

  OptionsClass options;
  options.KeyRepairOn = engine::window::KN_T;
  options.LoadHotkeys(ini);

  EXPECT_EQ(options.KeyForceMove1, engine::window::KN_LALT);
  EXPECT_EQ(options.KeyStop, engine::window::KN_S);
  EXPECT_EQ(options.KeyTeam1, engine::window::KN_1);
  EXPECT_EQ(options.KeyHome2, engine::window::KN_E_HOME);
  // 0 is how the file says "unbound".
  EXPECT_EQ(options.KeyRepairOn, engine::window::KN_NONE);
}

TEST(OptionsHotkeysTest, MissingEntriesKeepTheDefaults) {
  const INIClass ini;
  OptionsClass options;
  options.LoadHotkeys(ini);

  EXPECT_EQ(options.KeyStop, engine::window::KN_S);
  EXPECT_EQ(options.KeyBookmark1, engine::window::KN_F9);
}

TEST(OptionsHotkeysTest, SavesWindowsKeyCodes) {
  INIClass ini;
  const OptionsClass options;
  options.SaveHotkeys(ini);

  EXPECT_EQ(ini.Get_Int("WinHotkeys", "KeyForceMove1"), 18);
  EXPECT_EQ(ini.Get_Int("WinHotkeys", "KeyStop"), 83);
  EXPECT_EQ(ini.Get_Int("WinHotkeys", "KeyTeam10"), 48);
  EXPECT_EQ(ini.Get_Int("WinHotkeys", "KeyHome2"), 103);
  EXPECT_EQ(ini.Get_Int("WinHotkeys", "KeyBookmark1"), 120);  // VK_F9
}

// Writing the bindings and reading them back changes none of them.
TEST(OptionsHotkeysTest, RoundTripsEveryDefault) {
  INIClass ini;
  const OptionsClass defaults;
  defaults.SaveHotkeys(ini);

  OptionsClass loaded;
  loaded.KeyStop = engine::window::KN_NONE;
  loaded.KeyTeam1 = engine::window::KN_NONE;
  loaded.LoadHotkeys(ini);

  EXPECT_EQ(loaded.KeyStop, defaults.KeyStop);
  EXPECT_EQ(loaded.KeyTeam1, defaults.KeyTeam1);
  EXPECT_EQ(loaded.KeyForceMove2, defaults.KeyForceMove2);
  EXPECT_EQ(loaded.KeySidebarDown, defaults.KeySidebarDown);
}

// A palette whose entries spread over the 6-bit color cube.
PaletteClass SpreadPalette() {
  PaletteClass palette;
  for (int index = 0; index < PaletteClass::COLOR_COUNT; ++index) {
    palette.at(index) =
        engine::gfx::PaletteColor(engine::gfx::ExpandGun(index % 64),
                                  engine::gfx::ExpandGun((index * 7) % 64),
                                  engine::gfx::ExpandGun((index * 13) % 64));
  }
  return palette;
}

TEST(OptionsAdjustPaletteTest, NeutralSettingsKeepTheArt) {
  const PaletteClass original = SpreadPalette();
  // The HSV round trip on its own moves some of these entries, so a copy is
  // the only way to keep them all.
  int moved = 0;
  for (const engine::gfx::PaletteColor& color : original.colors()) {
    if (color.ToHsv().ToPaletteColor().SquaredDistance(color) != 0) {
      ++moved;
    }
  }
  ASSERT_GT(moved, 0);

  PaletteClass adjusted;
  OptionsClass::Adjust_Palette(original, adjusted, fixed::_1_2, fixed::_1_2,
                               fixed::_1_2, fixed::_1_2);

  for (int index = 0; index < PaletteClass::COLOR_COUNT; ++index) {
    EXPECT_EQ(adjusted.at(index).SquaredDistance(original.at(index)), 0)
        << "index " << index;
  }
}

TEST(OptionsAdjustPaletteTest, NoColorTurnsEveryEntryGrey) {
  const PaletteClass original = SpreadPalette();
  PaletteClass adjusted;
  OptionsClass::Adjust_Palette(original, adjusted, fixed::_1_2, fixed(0),
                               fixed::_1_2, fixed::_1_2);

  for (int index = 0; index < PaletteClass::COLOR_COUNT; ++index) {
    if (index == kMouseColor) {
      continue;
    }
    const engine::gfx::PaletteColor& color = adjusted.at(index);
    EXPECT_EQ(color.red(), color.green()) << "index " << index;
    EXPECT_EQ(color.green(), color.blue()) << "index " << index;
  }
}

}  // namespace
