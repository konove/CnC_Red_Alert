/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CNC_RED_ALERT_TECH_CDFILE_H_
#define CNC_RED_ALERT_TECH_CDFILE_H_

#include <string>
#include <string_view>
#include <vector>

#include "tech/disk_file.h"
#include "tech/file.h"

// File I/O class with multi-directory search support.
//
// CDFileClass extends DiskFile to search for files across multiple
// directories and drives. This is designed for CD-ROM games where data may
// exist on both the hard drive and CD-ROM.
//
// File lookup behavior:
//   - Read operations: Searches current directory first, then iterates through
//     registered search paths in order until the file is found.
//   - Write operations: Only uses the current directory (no path searching).
//
// Search paths are registered via AddSearchPaths() using semicolon-delimited
// strings (e.g., "C:\Game;D:\"). Paths support wildcard "?:" notation which
// resolves to the current CD-ROM drive letter.
//
// Example usage:
//   CDFileClass::AddSearchPaths("C:\GameData;?:\Assets");
//   CDFileClass file("textures\player.bmp");
//   file.Open(FileAccess::kRead);  // Searches C:\GameData, then CD drive
class CDFileClass : public DiskFile {
 public:
  explicit CDFileClass(std::string_view filename);
  CDFileClass() = default;

  CDFileClass(const CDFileClass&) = delete;
  CDFileClass& operator=(const CDFileClass&) = delete;
  CDFileClass(CDFileClass&&) = delete;
  CDFileClass& operator=(CDFileClass&&) = delete;

  ~CDFileClass() override = default;

  // Binds filename to the file object. Unless search is disabled, the current
  // directory and then each search path is checked for the file, and the first
  // location where it exists is stored as the name. A name found nowhere is
  // stored as given.
  void SetName(std::string_view filename) override;
  bool Open(std::string_view filename,
            FileAccess rights = FileAccess::kRead) override;
  bool Open(FileAccess rights = FileAccess::kRead) override;

  // Turns the search-path lookup in SetName() and Open() on or off for this
  // file object. It is on by default.
  void SetSearchEnabled(bool enabled) { search_disabled_ = !enabled; }

  // Returns true if any search path is registered.
  static bool HasSearchPaths() { return !search_paths_.empty(); }

  static void AddSearchPath(const std::string& path);

  // Appends new paths to the persistent search list and immediately scans them.
  //
  // This function adds the provided paths to the internal storage (raw_path_)
  // and processes them to register valid search paths. It optimizes by only
  // scanning the newly added paths, not the entire history.
  //
  // new_paths: A semicolon-delimited string of paths (e.g.,
  // "C:\Data;D:\Assets").
  //
  // Returns:
  //   0 if at least one valid path was found and added.
  //   1 if no valid paths were found.
  static int AddSearchPaths(std::string_view new_paths);

  static void ClearSearchPaths();

  // Clears and re-scans all currently stored search paths.
  //
  // This is used when the system configuration changes (e.g., a new CD is
  // inserted). It wipes the active search list and re-evaluates the entire
  // raw_path_ history, allowing wildcard drives ("?:") to resolve to new drive
  // letters.
  static void RefreshSearchPaths();

  // Makes drive the current CD drive, remembering the previous one as the last
  // CD drive.
  static void SetCdDrive(int drive);
  static int current_cd_drive() { return current_cd_drive_; }
  static int last_cd_drive() { return last_cd_drive_; }

 private:
  // Helper function to tokenize and process a list of paths.
  //
  // Handles path normalization (ensuring trailing separators) and resolves
  // wildcard drive specifications "?:" to the active CD-ROM drive.
  //
  // paths: The view of paths to process.
  //
  // Returns:
  //   0 if at least one valid path was added.
  //   1 otherwise.
  static int ProcessPathTokens(std::string_view paths);

  static std::vector<std::string> search_paths_;

  // Persistent storage for all added search paths.
  static std::string raw_path_;

  // The drive letter of the current CD drive
  static int current_cd_drive_;

  // The drive letter of the last used CD drive
  static int last_cd_drive_;

  // Is multi-drive searching disabled for this file object?
  bool search_disabled_ = false;
};

#endif  // CNC_RED_ALERT_TECH_CDFILE_H_
