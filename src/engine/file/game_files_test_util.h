// File: TestGameFiles, loose game files for a test to put where the game
// looks for its data.

#ifndef CNC_RED_ALERT_ENGINE_FILE_GAME_FILES_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_FILE_GAME_FILES_TEST_UTIL_H_

#include <filesystem>
#include <string_view>

#include "absl/base/attributes.h"

namespace engine::file {

// A directory of the running test's own on the game's search path, so that
// OpenGameFile() finds what the test writes there. The directory is made on
// first use, named after the test, and removed, with the search path cleared,
// when this is destroyed. Both games' fixtures own one.
//
// Example:
//   TestGameFiles files;
//   files.Write("credits.txt", "Producer\r");
//   EXPECT_NE(OpenGameFile("credits.txt"), nullptr);
class TestGameFiles {
 public:
  TestGameFiles() = default;
  ~TestGameFiles();

  TestGameFiles(const TestGameFiles&) = delete;
  TestGameFiles& operator=(const TestGameFiles&) = delete;
  TestGameFiles(TestGameFiles&&) = delete;
  TestGameFiles& operator=(TestGameFiles&&) = delete;

  // Writes a file called `name` holding `contents`, replacing one already
  // there.
  void Write(std::string_view name, std::string_view contents);

  // Returns where a file called `name` has to be for the game to find it,
  // for a test that writes the file itself. The directory exists; the file
  // need not.
  std::filesystem::path Path(std::string_view name);

  // The directory itself, made if it was not there yet.
  const std::filesystem::path& Directory() ABSL_ATTRIBUTE_LIFETIME_BOUND;

 private:
  // Empty until the first use.
  std::filesystem::path directory_;
};

}  // namespace engine::file

#endif  // CNC_RED_ALERT_ENGINE_FILE_GAME_FILES_TEST_UTIL_H_
