// The fixture shared by the tests that run against a real Game with no game
// data.

#ifndef CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_
#define CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "engine/file/game_files_test_util.h"
#include "gtest/gtest.h"
#include "ra/startup/game.h"

namespace ra {

// A Game with the house and vehicle types, room for two houses and four
// vehicles, and an empty map. A test fixture derives from it and builds the
// other types, heaps and houses it needs in its own constructor.
//
// Example:
//   class CarryoverTest : public GameFixture {
//    protected:
//     CarryoverTest() { base::NewPooled<HouseClass>(HOUSE_GREECE); }
//   };
class GameFixture : public ::testing::Test {
 protected:
  GameFixture();

  // Empties the display layers, which are static and outlive the Game.
  void TearDown() override;

  // Makes TextString(text_id) return `text`, for one of the game's own
  // strings (0 up to kTextCount). The first call installs a text table in
  // which every other string is empty; before it there is no table and every
  // TextString() is null.
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
  // One past the highest TXT_* number (ra/data/text_ids.h).
  static constexpr int kTextCount = 555;

  Game game_;
  // The strings SetText() has set, and the table made from them that Assets
  // borrows. Both are empty until the first SetText().
  std::vector<std::string> texts_;
  std::vector<std::byte> text_table_;
  engine::file::TestGameFiles game_files_;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_STARTUP_GAME_TEST_UTIL_H_
