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

// File: opening a single file on disk as a ByteStream.
//
// Originally RAWFILE.CPP by Joe L. Bostic, August 8, 1994.

#include "tech/disk_file.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"

std::optional<std::string> FindExistingFile(const std::string_view path) {
  // Opening is the existence test; it is what Open() will do next.
  std::string name(path);
  if (DiskStream::Open(name, FileAccess::kRead) != nullptr) {
    return name;
  }
  std::string lower_name = absl::AsciiStrToLower(name);
  if (DiskStream::Open(lower_name, FileAccess::kRead) != nullptr) {
    return lower_name;
  }
  return std::nullopt;
}

std::unique_ptr<DiskStream> OpenDiskFile(const std::string_view path,
                                         const FileAccess access) {
  if (const std::optional<std::string> existing = FindExistingFile(path)) {
    return DiskStream::Open(*existing, access);
  }
  return access == FileAccess::kRead ? nullptr : DiskStream::Open(path, access);
}
