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

#ifndef CNC_RED_ALERT_TECH_RAMFILE_H_
#define CNC_RED_ALERT_TECH_RAMFILE_H_

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "absl/base/attributes.h"
#include "tech/file.h"

// A "file" that reads and writes a caller-supplied memory buffer, for code
// written against File that needs to work on data already in memory.
class RAMFileClass final : public File {
 public:
  // Wraps size bytes at buffer. A null buffer with a positive size allocates a
  // scratch buffer of that size, which is only useful for writing.
  RAMFileClass(void* buffer ABSL_ATTRIBUTE_LIFETIME_BOUND, int size);

  RAMFileClass(const RAMFileClass&) = delete;
  RAMFileClass& operator=(const RAMFileClass&) = delete;
  RAMFileClass(RAMFileClass&&) = delete;
  RAMFileClass& operator=(RAMFileClass&&) = delete;

  ~RAMFileClass() override;

  [[nodiscard]] std::string_view FileName() const override { return "UNKNOWN"; }
  void SetName(std::string_view /*filename*/) override {}
  bool Create() override;
  bool Delete() override;
  bool IsAvailable() override;
  [[nodiscard]] bool IsOpen() const override;
  bool Open(std::string_view filename,
            FileAccess access = FileAccess::kRead) override;
  bool Open(FileAccess access = FileAccess::kRead) override;
  int32_t Read(void* buffer, int32_t size) override;
  int32_t Seek(int32_t offset, int origin = SEEK_CUR) override;
  int32_t Size() override;
  int32_t Write(const void* buffer, int32_t size) override;
  void Close() override;
  void Error(int /*error*/, bool /*can_retry*/ = false,
             std::string_view /*filename*/ = {}) override {}

 private:
  // The memory the "file" lives in.
  char* buffer_;

  // Size of buffer_. The file occupying it may be smaller.
  int capacity_;

  // Number of bytes of file data in buffer_.
  int size_;

  // Current read/write position within buffer_.
  int position_ = 0;

  // Access mode of the current open.
  FileAccess access_ = FileAccess::kRead;

  bool is_open_ = false;

  // The constructor allocated buffer_, so the destructor must delete it.
  bool owns_buffer_ = false;
};

#endif  // CNC_RED_ALERT_TECH_RAMFILE_H_
