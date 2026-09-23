#include "sdllib/file_system.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"

bool MatchesPattern(std::string_view pattern, std::string_view name) {
  // Greedy matching with one backtrack point: on a mismatch, the last '*'
  // swallows one more character of name and matching resumes after it.
  std::string_view star_pattern;
  std::string_view star_name;
  bool have_star = false;
  const auto same = [](const char a, const char b) {
    return absl::ascii_tolower(static_cast<unsigned char>(a)) ==
           absl::ascii_tolower(static_cast<unsigned char>(b));
  };
  while (!name.empty()) {
    if (!pattern.empty() && pattern.front() == '*') {
      pattern.remove_prefix(1);
      star_pattern = pattern;
      star_name = name;
      have_star = true;
    } else if (!pattern.empty() && (pattern.front() == '?' ||
                                    same(pattern.front(), name.front()))) {
      pattern.remove_prefix(1);
      name.remove_prefix(1);
    } else if (have_star) {
      star_name.remove_prefix(1);
      pattern = star_pattern;
      name = star_name;
    } else {
      return false;
    }
  }
  return pattern.find_first_not_of('*') == std::string_view::npos;
}

std::vector<FoundFile> FindFiles(const std::string_view pattern,
                                 const std::filesystem::path& directory) {
  std::vector<FoundFile> found;
  std::error_code error;
  std::filesystem::directory_iterator entries(directory, error);
  // The iterator's own increment throws on error; step it by hand instead.
  for (; !error && entries != std::filesystem::directory_iterator();
       entries.increment(error)) {
    const std::filesystem::directory_entry& entry = *entries;
    std::string name = entry.path().filename().string();
    if (!MatchesPattern(pattern, name)) {
      continue;
    }
    // A directory, or a name that cannot be stat'ed (a broken symlink, no
    // permission), is skipped.
    std::error_code stat_error;
    if (!entry.is_regular_file(stat_error)) {
      continue;
    }
    const std::filesystem::file_time_type written =
        entry.last_write_time(stat_error);
    if (stat_error) {
      continue;
    }
    const auto system_time =
        std::chrono::clock_cast<std::chrono::system_clock>(written);
    found.push_back(
        {.name = std::move(name),
         .modified = std::chrono::duration_cast<std::chrono::seconds>(
                         system_time.time_since_epoch())
                         .count()});
  }
  std::ranges::sort(found, {}, &FoundFile::name);
  return found;
}

int64_t FreeDiskSpace() {
  std::error_code error;
  const std::filesystem::space_info space = std::filesystem::space(".", error);
  return error ? 0 : static_cast<int64_t>(space.available);
}
