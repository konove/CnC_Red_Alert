#ifndef CNC_RED_ALERT_ENGINE_PLATFORM_FILE_SYSTEM_H_
#define CNC_RED_ALERT_ENGINE_PLATFORM_FILE_SYSTEM_H_

// File: directory listing and free space for the working directory, over
// std::filesystem.

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// One file that matched a FindFiles() pattern.
struct FoundFile {
  std::string name;
  int64_t modified;  // Seconds since the Unix epoch.
};

// Returns true if `name` matches `pattern`, where `*` matches any run of
// characters and `?` matches exactly one, case-insensitively (the game's
// files came from a case-insensitive filesystem).
bool MatchesPattern(std::string_view pattern, std::string_view name);

// Returns the regular files in `directory` whose name matches `pattern`
// (case-insensitively), sorted by name. Directories, and entries that cannot
// be stat'ed, are skipped.
std::vector<FoundFile> FindFiles(std::string_view pattern,
                                 const std::filesystem::path& directory = ".");

// Returns the bytes free on the filesystem holding the working directory, or
// 0 if that cannot be determined.
int64_t FreeDiskSpace();

#endif  // CNC_RED_ALERT_ENGINE_PLATFORM_FILE_SYSTEM_H_
