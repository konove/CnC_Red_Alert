#include "engine/file/game_files_test_util.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>
#include <system_error>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "engine/file/search_paths.h"
#include "gtest/gtest.h"

namespace engine::file {

TestGameFiles::~TestGameFiles() {
  if (!directory_.empty()) {
    SearchPaths::Clear();
    std::error_code ignored;
    std::filesystem::remove_all(directory_, ignored);
  }
}

void TestGameFiles::Write(const std::string_view name,
                          const std::string_view contents) {
  std::ofstream file(Path(name), std::ios::binary);
  file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  CHECK(file.good()) << "could not write " << name;
}

std::filesystem::path TestGameFiles::Path(const std::string_view name) {
  return Directory() / name;
}

const std::filesystem::path& TestGameFiles::Directory() {
  if (directory_.empty()) {
    const testing::TestInfo* test =
        testing::UnitTest::GetInstance()->current_test_info();
    directory_ = std::filesystem::temp_directory_path() /
                 absl::StrCat("cnc_test_files_", test->test_suite_name(), "_",
                              test->name(), ".dir");
    std::filesystem::remove_all(directory_);
    std::filesystem::create_directories(directory_);
    SearchPaths::Add(directory_.string());
  }
  return directory_;
}

}  // namespace engine::file
