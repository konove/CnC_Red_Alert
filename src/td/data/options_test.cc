// Tests that the options CONQUER.INI stores read back as they were written,
// and that saving them leaves the rest of the file alone.

#include "td/data/options.h"

#include <cstddef>
#include <vector>

#include "base/installed.h"
#include "engine/file/ini_file.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"
#include "td/assets/palettes.h"
#include "td/data/ini_io.h"

namespace td {

namespace {

using engine::file::IniFile;

// Fills `reloaded` from the text Save() writes for `ini`, which is what the
// next run of the game reads.
void SaveAndReload(const IniFile& ini, IniFile& reloaded) {
  std::vector<std::byte> saved;
  VectorSink sink(saved);
  ASSERT_TRUE(ini.Save(sink));
  SpanSource source(saved);
  ASSERT_TRUE(reloaded.Load(source));
}

// Reading applies the settings to the game palette and the palette speed, so
// each test installs palettes of its own for the length of the test.
class OptionsSettingsTest : public testing::Test {
 private:
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
};

// Gives every setting a value that is not its default and differs from the
// others.
OptionsClass MakeUnusualOptions() {
  OptionsClass options;
  options.GameSpeed = 1;
  options.ScrollRate = 6;
  options.Brightness = 0x91;
  options.Volume = 0x33;
  options.ScoreVolume = 0x44;
  options.Contrast = 0x95;
  options.Color = 0x96;
  options.Tint = 0x97;
  options.AutoScroll = false;
  options.IsScoreRepeat = true;
  options.IsScoreShuffle = true;
  options.IsDeathAnnounce = true;
  options.IsFreeScroll = true;
  return options;
}

TEST_F(OptionsSettingsTest, SavingKeepsSectionsAndEntriesTheGameDoesNotKnow) {
  IniFile ini;
  LoadIniText(
      "; written by hand\r\n"
      "[Mine]\r\n"
      "Foo=Bar\r\n"
      "[Options]\r\n"
      "GameSpeed=3\r\n"
      "Unknown=Kept\r\n",
      ini);

  MakeUnusualOptions().WriteSettings(ini);
  IniFile reloaded;
  SaveAndReload(ini, reloaded);

  EXPECT_EQ(reloaded.GetString("Mine", "Foo", ""), "Bar");
  EXPECT_EQ(reloaded.GetString("Options", "Unknown", ""), "Kept");
  EXPECT_EQ(reloaded.GetInt("Options", "GameSpeed", -1), 1);
}

TEST_F(OptionsSettingsTest, EveryOptionReadsBackAsItWasWritten) {
  const OptionsClass written = MakeUnusualOptions();
  IniFile ini;
  written.WriteSettings(ini);

  OptionsClass read;
  read.ReadSettings(ini);

  EXPECT_EQ(read.GameSpeed, written.GameSpeed);
  EXPECT_EQ(read.ScrollRate, written.ScrollRate);
  EXPECT_EQ(read.Brightness, written.Brightness);
  EXPECT_EQ(read.Volume, written.Volume);
  EXPECT_EQ(read.ScoreVolume, written.ScoreVolume);
  EXPECT_EQ(read.Contrast, written.Contrast);
  EXPECT_EQ(read.Color, written.Color);
  EXPECT_EQ(read.Tint, written.Tint);
  EXPECT_EQ(read.AutoScroll, written.AutoScroll);
  EXPECT_EQ(read.IsScoreRepeat, written.IsScoreRepeat);
  EXPECT_EQ(read.IsScoreShuffle, written.IsScoreShuffle);
  EXPECT_EQ(read.IsDeathAnnounce, written.IsDeathAnnounce);
  EXPECT_EQ(read.IsFreeScroll, written.IsFreeScroll);
}

TEST_F(OptionsSettingsTest, AMissingEmptyOrUnparsableValueGivesTheDefault) {
  IniFile ini;
  LoadIniText(
      "[Options]\r\n"
      "GameSpeed=\r\n"
      "ScrollRate=fast\r\n"
      "Volume=$40\r\n",
      ini);

  OptionsClass options = MakeUnusualOptions();
  options.ReadSettings(ini);

  EXPECT_EQ(options.GameSpeed, 4U);
  EXPECT_EQ(options.ScrollRate, 4);
  EXPECT_EQ(options.Volume, 0x40);
  // Not in the file at all.
  EXPECT_EQ(options.Brightness, 0x80);
  EXPECT_EQ(options.ScoreVolume, 0x40);
  EXPECT_TRUE(options.AutoScroll);
  EXPECT_FALSE(options.IsFreeScroll);
}

}  // namespace

}  // namespace td
