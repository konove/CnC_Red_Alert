// Tests for Tiberian Dawn's options: that saving them keeps what else
// CONQUER.INI holds, and the visual controls dialog, played by a script on a
// headless screen.

#include <filesystem>
#include <string>

#include "engine/file/ini_file.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "td/data/ini_io.h"
#include "td/screens/goptions.h"
#include "td/screens/visudlg.h"
#include "td/startup/game_test_util.h"
#include "td/startup/ui_test_util.h"

namespace td {
namespace {

// What Set_Brightness(200) stores and saves: the dialog's 0..255 is squeezed
// into the upper three quarters of the range, 64 + 200 / 2.
constexpr int kStoredBrightness = 164;

// The game writes CONQUER.INI beside itself, in the current directory, so
// the test moves into a directory of its own for as long as it runs.
class OptionsSettingsTest : public GameFixture {
 protected:
  void SetUp() override {
    start_directory_ = std::filesystem::current_path();
    std::filesystem::current_path(GameFilePath("").parent_path());
  }

  void TearDown() override {
    std::filesystem::current_path(start_directory_);
    GameFixture::TearDown();
  }

  // Reads CONQUER.INI as it is on disk now into `ini`.
  static void LoadSavedSettings(engine::file::IniFile& ini) {
    EXPECT_TRUE(LoadIniFile("CONQUER.INI", ini));
  }

 private:
  std::filesystem::path start_directory_;
};

TEST_F(OptionsSettingsTest, SavingWritesTheSettings) {
  TheOptions().Set_Brightness(200);

  TheOptions().Save_Settings();

  engine::file::IniFile saved;
  LoadSavedSettings(saved);
  EXPECT_EQ(saved.GetInt("Options", "Brightness", -1), kStoredBrightness);
}

// The file also belongs to the setup program and to the player, who may have
// put sections and entries there the game knows nothing about.
TEST_F(OptionsSettingsTest, SavingKeepsWhatTheGameDoesNotWrite) {
  WriteGameFile("CONQUER.INI",
                "[Options]\n"
                "Brightness=10\n"
                "HandMade=kept\n"
                "\n"
                "[Sound]\n"
                "Card=7\n");
  TheOptions().Set_Brightness(200);

  TheOptions().Save_Settings();

  engine::file::IniFile saved;
  LoadSavedSettings(saved);
  EXPECT_EQ(saved.GetInt("Options", "Brightness", -1), kStoredBrightness);
  EXPECT_EQ(saved.GetString("Options", "HandMade", ""), "kept");
  EXPECT_EQ(saved.GetInt("Sound", "Card", -1), 7);
}

TEST_F(OptionsSettingsTest, LoadingReadsBackWhatSavingWrote) {
  TheOptions().Set_Brightness(200);
  TheOptions().Set_Contrast(90);
  TheOptions().Save_Settings();
  TheOptions().Set_Brightness(1);
  TheOptions().Set_Contrast(1);

  TheOptions().Load_Settings();

  EXPECT_EQ(TheOptions().Get_Brightness(), 200);
  EXPECT_EQ(TheOptions().Get_Contrast(), 90);
}

using VisualControlsTest = UiFixture;

TEST_F(VisualControlsTest, EscClosesIt) {
  player().At(3).Key(engine::window::KN_ESC);

  VisualControlsClass::Process();

  EXPECT_TRUE(player().finished());
}

// The dialog opens on the brightness slider, which the arrow keys move.
TEST_F(VisualControlsTest, LeftAndRightMoveTheBrightness) {
  TheOptions().Set_Brightness(128);
  int after_left = -1;
  player()
      .At(3)
      .Key(engine::window::KN_LEFT)
      .At(8)
      .Do([&after_left] { after_left = TheOptions().Get_Brightness(); })
      .Key(engine::window::KN_RIGHT)
      .At(12)
      .Key(engine::window::KN_RIGHT)
      .At(17)
      .Key(engine::window::KN_ESC);

  VisualControlsClass::Process();

  EXPECT_LT(after_left, 128);
  EXPECT_GT(TheOptions().Get_Brightness(), 128);
}

TEST_F(VisualControlsTest, DownMovesToTheNextSlider) {
  TheOptions().Set_Brightness(128);
  TheOptions().Set_Color(128);
  player()
      .At(3)
      .Key(engine::window::KN_DOWN)
      .At(8)
      .Key(engine::window::KN_LEFT)
      .At(13)
      .Key(engine::window::KN_ESC);

  VisualControlsClass::Process();

  EXPECT_EQ(TheOptions().Get_Brightness(), 128);
  EXPECT_LT(TheOptions().Get_Color(), 128);
}

TEST_F(VisualControlsTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(3).Key(engine::window::KN_LEFT);

  EXPECT_THROW(VisualControlsClass::Process(), engine::window::ScriptOverrun);
}

}  // namespace
}  // namespace td
