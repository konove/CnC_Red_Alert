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

// File: DiskFile implementation.
//
// Originally RAWFILE.CPP by Joe L. Bostic, August 8, 1994.

#include "tech/disk_file.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "absl/strings/ascii.h"
#include "base/types.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"
#include "tech/file.h"

std::optional<std::string> FindExistingFile(const std::string_view path) {
  // Opening is the existence test; it is what Open() will do next.
  std::string name(path);
  if (void* const handle = IO_Open_File(name.c_str(), FileAccess::kRead)) {
    IO_Close_File(handle);
    return name;
  }
  std::string lower_name = absl::AsciiStrToLower(name);
  if (void* const handle =
          IO_Open_File(lower_name.c_str(), FileAccess::kRead)) {
    IO_Close_File(handle);
    return lower_name;
  }
  return std::nullopt;
}

bool DiskFile::Create() {
  Close();
  return DiskStream::Open(filename_, FileAccess::kWrite) != nullptr;
}

bool DiskFile::Delete() {
  Close();
  if (!IsAvailable()) {
    return false;
  }
  return IO_Delete_File(filename_.c_str());
}

bool DiskFile::IsAvailable() {
  if (filename_.empty()) {
    return false;
  }
  if (IsOpen()) {
    return true;
  }
  std::optional<std::string> found = FindExistingFile(filename_);
  if (!found) {
    return false;
  }
  filename_ = *std::move(found);
  return true;
}

bool DiskFile::Open(const std::string_view filename, const FileAccess rights) {
  SetName(filename);
  return Open(rights);
}

bool DiskFile::Open(const FileAccess rights) {
  Close();
  if (filename_.empty()) {
    return false;
  }
  stream_ = DiskStream::Open(filename_, rights);
  return IsOpen();
}

base::ssize DiskFile::Read(const std::span<std::byte> buffer) {
  const bool opened_for_this_read = !IsOpen() && Open(FileAccess::kRead);
  if (!IsOpen()) {
    return 0;
  }
  const base::ssize bytes_read = stream_->Read(buffer);
  if (opened_for_this_read) {
    Close();
  }
  return bytes_read;
}

base::ssize DiskFile::Write(const std::span<const std::byte> buffer) {
  const bool opened_for_this_write = !IsOpen() && Open(FileAccess::kWrite);
  if (!IsOpen()) {
    return 0;
  }
  const base::ssize bytes_written = stream_->Write(buffer);
  if (opened_for_this_write) {
    Close();
  }
  return bytes_written;
}

base::ssize DiskFile::Seek(const base::ssize offset, const SeekOrigin origin) {
  return IsOpen() ? stream_->Seek(offset, origin) : 0;
}

base::ssize DiskFile::Size() {
  if (IsOpen()) {
    return stream_->Size();
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(filename_, FileAccess::kRead);
  return stream != nullptr ? stream->Size() : 0;
}
