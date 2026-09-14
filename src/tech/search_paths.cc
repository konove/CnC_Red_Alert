// File: SearchPaths implementation. Originally the static part of CDFILE.CPP
// by Joe L. Bostic, October 18, 1994.

#include "tech/search_paths.h"

#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "tech/disk_file.h"

// Supplied by the game: returns the index of the CD in cd_drive, waiting up to
// timeout ticks, or -1 if none is recognized.
extern int Get_CD_Index(int cd_drive, int timeout);

std::vector<std::string> SearchPaths::directories_;
std::string SearchPaths::history_;
int SearchPaths::current_cd_drive_ = 0;
int SearchPaths::last_cd_drive_ = 0;

int SearchPaths::Add(const std::string_view paths) {
  if (paths.empty()) {
    return 0;
  }
  if (!history_.empty()) {
    history_ += ';';
  }
  history_ += paths;
  return Scan(paths);
}

int SearchPaths::Scan(const std::string_view paths) {
  bool added = false;

  for (const auto token : paths | std::views::split(';')) {
    std::string path(token.begin(), token.end());
    if (path.empty()) {
      continue;
    }

    // Resolve() concatenates the file name directly, so every directory
    // must end in a separator. A bare drive ("D:") already does what it
    // should.
    if (path.back() != std::filesystem::path::preferred_separator &&
        path.back() != ':') {
      path += std::filesystem::path::preferred_separator;
    }

    // "?:" is a placeholder for the CD drive and is added only when that
    // drive holds a recognized CD; Get_CD_Index waits up to two seconds.
    if (path.starts_with("?:")) {
      if (current_cd_drive_ && Get_CD_Index(current_cd_drive_, 120) >= 0) {
        path[0] = static_cast<char>(current_cd_drive_ + 'A');
        directories_.push_back(path);
        added = true;
      }
      continue;
    }

    directories_.push_back(path);
    added = true;
  }

  return added ? 0 : 1;
}

void SearchPaths::Clear() { directories_.clear(); }

void SearchPaths::Refresh() {
  Clear();
  Scan(history_);
}

bool SearchPaths::HasAny() { return !directories_.empty(); }

void SearchPaths::SetCdDrive(const int drive) {
  last_cd_drive_ = current_cd_drive_;
  current_cd_drive_ = drive;
}

int SearchPaths::current_cd_drive() { return current_cd_drive_; }

int SearchPaths::last_cd_drive() { return last_cd_drive_; }

std::optional<std::string> SearchPaths::Resolve(const std::string_view name) {
  // A search directory plus an empty name would name the directory itself.
  if (name.empty()) {
    return std::nullopt;
  }

  // The current directory wins, so that patch files override CD data.
  if (auto found = FindExistingFile(name)) {
    return found;
  }

  for (const std::string& directory : directories_) {
    if (auto found = FindExistingFile(directory + std::string(name))) {
      return found;
    }
  }
  return std::nullopt;
}
