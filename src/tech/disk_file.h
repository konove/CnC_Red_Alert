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

#ifndef CNC_RED_ALERT_TECH_DISK_FILE_H_
#define CNC_RED_ALERT_TECH_DISK_FILE_H_

// File: opening a single file on disk as a ByteStream, and the
// case-insensitive existence check the game's lookups share.
//
// Originally RAWFILE.H (class RawFileClass) by Joe L. Bostic, August 8, 1994.

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "tech/byte_stream.h"
#include "tech/disk_stream.h"
#include "tech/file_access.h"

// Returns path if a file exists there, otherwise the lowercased path if a file
// exists there (game data is named in upper case, while Unix installs often
// carry it in lower case), otherwise nullopt.
std::optional<std::string> FindExistingFile(std::string_view path);

// Opens path, preferring an existing file under the lowercased name when the
// name as given does not exist (see FindExistingFile), for every access mode:
// a write replaces the file the game would read. Returns nullptr on failure.
std::unique_ptr<DiskStream> OpenDiskFile(std::string_view path,
                                         FileAccess access = FileAccess::kRead);

#endif  // CNC_RED_ALERT_TECH_DISK_FILE_H_
