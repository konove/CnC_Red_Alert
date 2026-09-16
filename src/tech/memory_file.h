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

/* $Header: /CounterStrike/RAMFILE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RAMFILE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : June 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_MEMORY_FILE_H_
#define CNC_RED_ALERT_TECH_MEMORY_FILE_H_

#include <cstddef>
#include <cstdio>
#include <span>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"
#include "tech/file.h"

// A File that reads and writes a caller-supplied memory buffer, for code
// written against File that needs to work on data already in memory.
// Originally RAMFILE.H (class RAMFileClass).
class MemoryFile final : public File {
 public:
  // Wraps caller-owned storage, which must outlive this file.
  explicit MemoryFile(
      std::span<std::byte> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
  // Allocates scratch storage of size bytes.
  explicit MemoryFile(base::ssize size);

  MemoryFile(const MemoryFile&) = delete;
  MemoryFile& operator=(const MemoryFile&) = delete;
  MemoryFile(MemoryFile&&) = delete;
  MemoryFile& operator=(MemoryFile&&) = delete;

  ~MemoryFile() override;

  [[nodiscard]] std::string_view FileName() const override { return "UNKNOWN"; }
  void SetName(std::string_view /*filename*/) override {}
  bool Create() override;
  bool Delete() override;
  bool IsAvailable() override;
  [[nodiscard]] bool IsOpen() const override;
  bool Open(std::string_view filename,
            FileAccess access = FileAccess::kRead) override;
  bool Open(FileAccess access = FileAccess::kRead) override;
  using File::Read;
  using File::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> buffer) override;
  [[nodiscard]] bool ok() const override { return true; }
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override;
  void Close() override;

 private:
  // The memory the "file" lives in.
  std::vector<std::byte> owned_buffer_;
  std::span<std::byte> buffer_;

  // Size of buffer_. The file occupying it may be smaller.
  base::ssize capacity_;

  // Number of bytes of file data in buffer_.
  base::ssize size_;

  // Current read/write position within buffer_.
  base::ssize position_ = 0;

  // Access mode of the current open.
  FileAccess access_ = FileAccess::kRead;

  bool is_open_ = false;

};

#endif  // CNC_RED_ALERT_TECH_MEMORY_FILE_H_
