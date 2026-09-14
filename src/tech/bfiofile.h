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

/* $Header: /CounterStrike/BFIOFILE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : BFIOFILE.H *
 *                                                                                             *
 *                   Programmer : David R. Dettmer *
 *                                                                                             *
 *                   Start Date : November 10, 1995 *
 *                                                                                             *
 *                  Last Update : November 10, 1995  [DRD] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_BFIOFILE_H_
#define CNC_RED_ALERT_TECH_BFIOFILE_H_

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "tech/rawfile.h"
#include "tech/wwfile.h"

/*
**	This derivation of the raw file class handles buffering the input/output
*in order to *	achieve greater speed. The buffering is not active by default.
*It must be activated *	by setting the appropriate buffer through the Cache()
*function.
*/
class BufferIOFileClass : public RawFileClass {
 public:
  // Smallest buffer Cache() will use.
  static constexpr int32_t kMinimumBufferSize = 1024;

  explicit BufferIOFileClass(std::string_view filename);
  BufferIOFileClass() = default;

  BufferIOFileClass(const BufferIOFileClass&) = delete;
  BufferIOFileClass& operator=(const BufferIOFileClass&) = delete;
  BufferIOFileClass(BufferIOFileClass&&) = delete;
  BufferIOFileClass& operator=(BufferIOFileClass&&) = delete;

  ~BufferIOFileClass() override;

  bool Cache(int32_t size = 0, void* buffer = nullptr);
  void Free();
  bool Commit();
  void SetName(std::string_view filename) override;
  [[nodiscard]] bool IsOpen() const override;
  bool Open(std::string_view filename,
            FileAccess rights = FileAccess::kRead) override;
  bool Open(FileAccess rights = FileAccess::kRead) override;
  int32_t Read(void* buffer, int32_t size) override;
  int32_t Seek(int32_t offset, int origin = SEEK_CUR) override;
  int32_t Size() override;
  int32_t Write(const void* buffer, int32_t size) override;
  void Close() override;

 protected:
  bool DoIsAvailable(AvailabilityCheck mode) override;

 private:
  // Cache() allocated buffer_, so Free() must delete it.
  bool owns_buffer_ : 1 = false;

  // The file was opened while buffering was active.
  bool is_open_ : 1 = false;

  // The file on disk is open too, because the buffer cannot hold everything
  // the access rights require.
  bool is_disk_open_ : 1 = false;

  // buffer_ holds the file bytes starting at buffer_file_position_.
  bool is_buffer_loaded_ : 1 = false;

  // buffer_ has changes that Commit() has not written yet.
  bool has_unwritten_changes_ : 1 = false;

  // Reads, writes and seeks go through buffer_ instead of straight to disk.
  bool use_buffer_ : 1 = false;

  // Access rights of the buffered open.
  FileAccess buffer_rights_ = FileAccess::kRead;

  void* buffer_ = nullptr;
  int32_t buffer_size_ = 0;

  // Read/write position within buffer_.
  int32_t buffer_position_ = 0;

  // File offset of the first byte in buffer_.
  int32_t buffer_file_position_ = 0;

  // Changed range of buffer_, [change_begin_, change_end_), or -1 for both
  // when nothing has changed.
  int32_t change_begin_ = -1;
  int32_t change_end_ = -1;

  int32_t file_size_ = 0;
  int32_t file_position_ = 0;

  // Offset of the file within the underlying raw file, nonzero when the file
  // is biased inside a larger one.
  int32_t true_file_start_ = 0;
};

#endif  // CNC_RED_ALERT_TECH_BFIOFILE_H_
