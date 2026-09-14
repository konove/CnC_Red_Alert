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

#include <string_view>

#include "tech/disk_file.h"
#include "tech/file.h"

// A DiskFile whose name is looked up through SearchPaths when it is set for
// reading, so that data can live on the hard drive or on the CD. Writes use
// the name as given.
//
// Example:
//   SearchPaths::Add("C:\\GameData;?:\\Assets");
//   CDFileClass file("textures\\player.bmp");
//   file.Open(FileAccess::kRead);  // Searches C:\GameData, then the CD drive
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

  // Turns the SearchPaths lookup in SetName() and Open() on or off for this
  // file object. It is on by default.
  void SetSearchEnabled(bool enabled) { search_disabled_ = !enabled; }

 private:
  // Is the SearchPaths lookup disabled for this file object?
  bool search_disabled_ = false;
};

#endif  // CNC_RED_ALERT_TECH_CDFILE_H_
