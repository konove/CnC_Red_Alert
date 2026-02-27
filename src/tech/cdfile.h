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

#include "tech/bfiofile.h"
#include "tech/wwfile.h"

// File I/O class with multi-directory search support.
//
// CDFileClass extends BufferIOFileClass to search for files across multiple
// directories and drives. This is designed for CD-ROM games where data may
// exist on both the hard drive and CD-ROM.
//
// File lookup behavior:
//   - Read operations: Searches current directory first, then iterates through
//     registered search paths in order until the file is found.
//   - Write operations: Only uses the current directory (no path searching).
//
// Search paths are registered via Add_Search_Drives() using semicolon-delimited
// strings (e.g., "C:\Game;D:\"). Paths support wildcard "?:" notation which
// resolves to the current CD-ROM drive letter.
//
// Example usage:
//   CDFileClass::Add_Search_Drives("C:\GameData;?:\Assets");
//   CDFileClass file("textures\player.bmp");
//   file.Open(FileAccess::kRead);  // Searches C:\GameData, then CD drive
class CDFileClass : public BufferIOFileClass {
 public:
  explicit CDFileClass(const char* filename);
  CDFileClass();
  ~CDFileClass() override = default;

  CDFileClass(const CDFileClass&) = delete;
  CDFileClass& operator=(const CDFileClass&) = delete;
  CDFileClass(CDFileClass&&) = delete;
  CDFileClass& operator=(CDFileClass&&) = delete;

  const char* Set_Name(const char* filename) override;
  int Open(const char* filename,
           FileAccess rights = FileAccess::kRead) override;
  int Open(FileAccess rights = FileAccess::kRead) override;

  void Searching(const bool on) { is_disabled_ = !on; }

  static bool Is_There_Search_Drives() { return !search_paths_.empty(); }

  static void Add_Search_Drive(const std::string& path);

  // Appends new paths to the persistent search list and immediately scans them.
  //
  // This function adds the provided paths to the internal storage (RawPath)
  // and processes them to register valid search drives. It optimizes by only
  // scanning the newly added paths, not the entire history.
  //
  // new_paths: A semicolon-delimited string of paths (e.g.,
  // "C:\Data;D:\Assets").
  //
  // Returns:
  //   0 if at least one valid path was found and added.
  //   1 if no valid drives were found.
  static int Add_Search_Drives(std::string_view new_paths);

  static void Clear_Search_Drives();

  // Clears and re-scans all currently stored search paths.
  //
  // This is used when the system configuration changes (e.g., a new CD is
  // inserted). It wipes the active search list in the file system and
  // re-evaluates the entire RawPath history, allowing wildcard drives ("?:")
  // to resolve to new drive letters.
  static void Refresh_Search_Drives();
  static void Set_CD_Drive(int drive);
  static int Get_CD_Drive() { return current_cd_drive_; }
  static int Get_Last_CD_Drive() { return last_cd_drive_; }

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
  static int Process_Path_Tokens(std::string_view paths);

  // Is multi-drive searching disabled for this file object?
  bool is_disabled_ : true;

  static std::vector<std::string> search_paths_;

  // Persistent storage for all added search paths.
  static std::string raw_path_;

  // The drive letter of the current CD drive
  static int current_cd_drive_;

  // The drive letter of the last used CD drive
  static int last_cd_drive_;
};

int harderr_handler(unsigned, unsigned, unsigned*);

int Get_CD_Drive();

#endif  // CNC_RED_ALERT_TECH_CDFILE_H_
