#include "port/win32/win32_system.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "port/win32/win32_registry.h"

#ifndef _WIN32

namespace {

// A file that exists for the length of one test.
class TempFile {
 public:
  explicit TempFile(std::string_view contents) {
    static int counter = 0;
    path_ = std::filesystem::temp_directory_path() /
            ("win32_system_test_" + std::to_string(++counter));
    std::ofstream out(path_, std::ios::binary);
    out << contents;
  }

  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;

  ~TempFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  const char* c_str() const { return path_.c_str(); }
  const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

TEST(Win32SystemTest, FindFirstFileReportsSizeAndName) {
  const TempFile file("0123456789");
  WIN32_FIND_DATA data{};

  HANDLE handle = FindFirstFile(file.c_str(), &data);

  ASSERT_NE(handle, INVALID_HANDLE_VALUE);
  EXPECT_EQ(data.nFileSizeLow, 10U);
  EXPECT_EQ(data.nFileSizeHigh, 0U);
  EXPECT_STREQ(data.cFileName, file.path().filename().c_str());
  EXPECT_EQ(FindClose(handle), TRUE);
}

TEST(Win32SystemTest, FindFirstFileFailsWhenNothingIsThere) {
  WIN32_FIND_DATA data{};
  EXPECT_EQ(FindFirstFile("no_such_file_at_all.tmp", &data),
            INVALID_HANDLE_VALUE);
  EXPECT_EQ(FindFirstFile(nullptr, &data), INVALID_HANDLE_VALUE);
}

TEST(Win32SystemTest, WildcardsAreReportedAsNotFound) {
  // Documented limitation: a pattern must never match the wrong file.
  const TempFile file("x");
  WIN32_FIND_DATA data{};
  const std::string pattern =
      (file.path().parent_path() / "win32_system_test_*").string();

  EXPECT_EQ(FindFirstFile(pattern.c_str(), &data), INVALID_HANDLE_VALUE);
}

TEST(Win32SystemTest, DeleteFileRemovesTheFile) {
  const TempFile file("x");
  ASSERT_TRUE(std::filesystem::exists(file.path()));

  EXPECT_EQ(DeleteFile(file.c_str()), TRUE);
  EXPECT_FALSE(std::filesystem::exists(file.path()));

  // Deleting what is not there fails rather than pretending.
  EXPECT_EQ(DeleteFile(file.c_str()), FALSE);
  EXPECT_EQ(DeleteFile(nullptr), FALSE);
}

TEST(Win32SystemTest, DesktopOperationsFail) {
  EXPECT_EQ(ShellExecute(nullptr, "open", "http://example.com", nullptr, ".",
                         SW_SHOW),
            nullptr);
  EXPECT_EQ(LoadLibrary("wolapi.dll"), nullptr);
  EXPECT_EQ(GetProcAddress(nullptr, "DllRegisterServer"), nullptr);
  EXPECT_EQ(ShowWindow(nullptr, SW_RESTORE), FALSE);
  EXPECT_EQ(GetLastError(), 0U);
}

TEST(Win32SystemTest, CreateProcessFailsAndClearsItsOutParameter) {
  STARTUPINFO startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  process.hProcess = &process;  // Deliberately not null to start with.

  EXPECT_EQ(CreateProcess(nullptr, nullptr, nullptr, nullptr, FALSE, 0, nullptr,
                          nullptr, &startup, &process),
            FALSE);
  EXPECT_EQ(process.hProcess, nullptr);
}

TEST(Win32SystemTest, TheWorkingDirectoryCanBeReadAndChanged) {
  char before[MAX_PATH] = {};
  const DWORD length = GetCurrentDirectory(sizeof(before), before);
  ASSERT_GT(length, 0U);
  EXPECT_EQ(std::strlen(before), length) << "the length excludes the null";
  EXPECT_EQ(std::filesystem::path(before), std::filesystem::current_path());

  const std::filesystem::path temp = std::filesystem::temp_directory_path();
  ASSERT_EQ(SetCurrentDirectory(temp.c_str()), TRUE);
  EXPECT_EQ(std::filesystem::current_path(), std::filesystem::canonical(temp));

  ASSERT_EQ(SetCurrentDirectory(before), TRUE);
  EXPECT_EQ(std::filesystem::path(before), std::filesystem::current_path());
}

TEST(Win32SystemTest, TheWorkingDirectoryReportsFailureRatherThanTruncating) {
  char tiny[2] = {'x', 'x'};
  EXPECT_EQ(GetCurrentDirectory(sizeof(tiny), tiny), 0U);
  EXPECT_EQ(tiny[0], 'x') << "a failed read must not touch the buffer";

  EXPECT_EQ(GetCurrentDirectory(0, tiny), 0U);
  EXPECT_EQ(GetCurrentDirectory(sizeof(tiny), nullptr), 0U);

  EXPECT_EQ(SetCurrentDirectory("no_such_directory_at_all"), FALSE);
  EXPECT_EQ(SetCurrentDirectory(nullptr), FALSE);
}

TEST(Win32SystemTest, CreateDirectoryMakesOneLevelOnly) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "win32_create_dir_test";
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  EXPECT_EQ(CreateDirectory(root.c_str(), nullptr), TRUE);
  EXPECT_TRUE(std::filesystem::is_directory(root));

  EXPECT_EQ(CreateDirectory(root.c_str(), nullptr), FALSE)
      << "an existing directory is a failure, as on Windows";

  const std::filesystem::path deep = root / "a" / "b";
  EXPECT_EQ(CreateDirectory(deep.c_str(), nullptr), FALSE)
      << "this is not mkdir -p";

  EXPECT_EQ(CreateDirectory(nullptr, nullptr), FALSE);
  std::filesystem::remove_all(root, ignored);
}

TEST(Win32RegistryTest, ReadsFailAndLeaveOutputsAlone) {
  int marker = 0;
  HKEY key = &marker;  // Deliberately not null to start with.
  EXPECT_EQ(
      RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Westwood", 0, KEY_READ, &key),
      ERROR_FILE_NOT_FOUND);

  BYTE value[sizeof(DWORD)] = {0xDE, 0xAD, 0xBE, 0xEF};
  DWORD size = sizeof(value);
  EXPECT_EQ(
      RegQueryValueEx(nullptr, "InstallPath", nullptr, nullptr, value, &size),
      ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(value[0], 0xDE) << "a failed query must not touch the buffer";
}

TEST(Win32RegistryTest, WritesAreDroppedButReportSuccess) {
  const BYTE value[sizeof(DWORD)] = {1, 0, 0, 0};
  EXPECT_EQ(RegSetValueEx(nullptr, "WOLAPI Find Enabled", 0, REG_DWORD, value,
                          sizeof(value)),
            ERROR_SUCCESS);
  EXPECT_EQ(RegCloseKey(nullptr), ERROR_SUCCESS);
}

}  // namespace

#endif  // _WIN32
