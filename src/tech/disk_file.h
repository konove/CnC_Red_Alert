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

// File: DiskFile, a File that is a single file on disk, and the
// case-insensitive existence check the game's lookups share.
//
// Originally RAWFILE.H (class RawFileClass) by Joe L. Bostic, August 8, 1994.

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"
#include "tech/file.h"

// Returns path if a file exists there, otherwise the lowercased path if a file
// exists there (game data is named in upper case, while Unix installs often
// carry it in lower case), otherwise nullopt.
std::optional<std::string> FindExistingFile(std::string_view path);

// A File over one file on disk, held as a DiskStream while it is open. Read
// and Write on a closed file open it for the call and close it afterwards.
class DiskFile : public File {
 public:
  explicit DiskFile(std::string_view filename) : filename_(filename) {}
  DiskFile() = default;

  DiskFile(const DiskFile&) = delete;
  DiskFile& operator=(const DiskFile&) = delete;
  DiskFile(DiskFile&&) = delete;
  DiskFile& operator=(DiskFile&&) = delete;

  ~DiskFile() override = default;

  [[nodiscard]] std::string_view FileName() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    return filename_;
  }

  // Binds filename to the object. An open file stays open under the old name.
  void SetName(std::string_view filename) override { filename_ = filename; }

  // Creates the file empty, truncating an existing one, and leaves it closed.
  bool Create() override;

  // Deletes the file from disk. Returns false, deleting nothing, if it does
  // not exist.
  bool Delete() override;

  // Returns true if the file is open or exists. When it exists only under the
  // lowercased name, the object takes that name so a later Open() finds it.
  bool IsAvailable() override;

  [[nodiscard]] bool IsOpen() const override { return stream_ != nullptr; }

  bool Open(std::string_view filename,
            FileAccess rights = FileAccess::kRead) override;

  // Opens the file with the given access, closing it first if it was open,
  // and returns whether it could be opened.
  bool Open(FileAccess rights = FileAccess::kRead) override;

  using File::Read;
  using File::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> buffer) override;
  [[nodiscard]] bool ok() const override { return !failed_; }

  // Returns the new position, or 0 if the file is not open. A seek to before
  // the start of the file leaves the position where it was, as stdio does.
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;

  // Returns the size in bytes, opening and closing the file to measure it if
  // it is not open. Returns 0 for a file that does not exist.
  base::ssize Size() override;

  void Close() override { stream_.reset(); }

 private:
  // Name of the file on disk; empty if none has been assigned.
  std::string filename_;

  // The open file, or nullptr while it is closed.
  std::unique_ptr<DiskStream> stream_;

  // Set when a read or write fails; cleared by Open().
  bool failed_ = false;
};

#endif  // CNC_RED_ALERT_TECH_DISK_FILE_H_
