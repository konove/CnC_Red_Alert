// The fixture shared by the tests that run against a real Game with no game
// data.

#ifndef CNC_RED_ALERT_TD_STARTUP_GAME_TEST_UTIL_H_
#define CNC_RED_ALERT_TD_STARTUP_GAME_TEST_UTIL_H_

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "engine/file/game_files_test_util.h"
#include "engine/window/display.h"
#include "gtest/gtest.h"
#include "td/startup/game.h"

namespace td {

// A Game with an empty map and no objects. A test fixture derives from it and
// sizes the heaps and builds the houses it needs in its own constructor.
//
// Example:
//   class OptionsTest : public GameFixture {
//    protected:
//     OptionsTest() { WriteGameFile("CONQUER.INI", "[Options]\n"); }
//   };
class GameFixture : public ::testing::Test {
 protected:
  GameFixture();

  // Empties the display layers, which are static and outlive the Game.
  void TearDown() override;

  // Makes Text_String(text_id) return `text`, for one of the strings the
  // text table holds (0 up to kTextCount). The first call installs a table
  // in which every other string is empty; before it there is no table and
  // Text_String() is null for them.
  void SetText(int text_id, std::string_view text);

  // Puts a file called `name` holding `contents` where the game finds its
  // data, so that OpenGameFile(name) reads it. The files live in a directory
  // of the test's own, removed when the test ends.
  void WriteGameFile(const std::string_view name,
                     const std::string_view contents) {
    game_files_.Write(name, contents);
  }

  // Returns where a file called `name` has to be for OpenGameFile(name) to
  // read it, for a test that writes the file itself. The directory exists;
  // the file need not.
  std::filesystem::path GameFilePath(const std::string_view name) {
    return game_files_.Path(name);
  }

 private:
  // More than the text table has entries; the debug strings start here.
  static constexpr int kTextCount = 1000;

  // Without a window: the display is a surface a test may or may not use.
  Game game_{engine::window::DisplayKind::kHeadless};
  // The strings SetText() has set, and the table made from them that Assets
  // borrows. Both are empty until the first SetText().
  std::vector<std::string> texts_;
  std::vector<std::byte> text_table_;
  engine::file::TestGameFiles game_files_;
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_STARTUP_GAME_TEST_UTIL_H_
