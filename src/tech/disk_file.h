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

/* $Header: /CounterStrike/RAWFILE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : RAWFILE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * DiskFile::FileName -- Returns with the filename associate
 *with the file object.      * DiskFile::DiskFile -- Default constructor
 *for a file object.                      * DiskFile::~DiskFile --
 *Default deconstructor for a file object.                   *
 *   DiskFile::IsOpen -- Checks to see if the file is open or not. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_DISK_FILE_H_
#define CNC_RED_ALERT_TECH_DISK_FILE_H_

#include <climits>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "tech/file.h"

#ifndef WWERROR
#define WWERROR (-1)
#endif

// A File that reads and writes a single file on disk through the low-level
// IO_* routines. Originally RAWFILE.H (class RawFileClass). Derived classes add
// buffering, search paths and mixfile support.
//
// A file can be biased (see Bias()) so that a byte range inside a larger file,
// such as an entry in a mixfile, behaves as a whole file of its own.
//
// Override Error() when more sophisticated error handling is required; the
// version here ignores every error.
class DiskFile : public File {
 public:
  explicit DiskFile(std::string_view filename);
  DiskFile() = default;

  DiskFile(const DiskFile&) = delete;
  DiskFile& operator=(const DiskFile&) = delete;
  DiskFile(DiskFile&&) = delete;
  DiskFile& operator=(DiskFile&&) = delete;

  ~DiskFile() override;

  [[nodiscard]] std::string_view FileName() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND override {
    return filename_;
  }
  void SetName(std::string_view filename) override;
  bool Create() override;
  bool Delete() override;
  bool IsAvailable() override;
  [[nodiscard]] bool IsOpen() const override;
  bool Open(std::string_view filename,
            FileAccess rights = FileAccess::kRead) override;
  bool Open(FileAccess rights = FileAccess::kRead) override;
  int32_t Read(void* buffer, int32_t size) override;
  int32_t Seek(int32_t offset, int origin = SEEK_CUR) override;
  int32_t Size() override;
  int32_t Write(const void* buffer, int32_t size) override;
  void Close() override;
  void Error(int error, bool can_retry = false,
             std::string_view filename = {}) override;

  // Makes the byte range starting at start, length bytes long, appear as the
  // whole file. start is added to the current bias; start == 0 removes the
  // bias. length == -1 extends the range to the end of the file.
  void Bias(int start, int length = -1);

  // Returns the offset in the underlying file at which the biased range
  // begins, or 0 for an unbiased file.
  [[nodiscard]] int bias_start() const { return bias_start_; }

 protected:
  // Seeks in the underlying file, ignoring any bias.
  int32_t RawSeek(int32_t offset, int origin = SEEK_CUR);

 private:
  // Access rights passed to the most recent Open().
  FileAccess rights_ = FileAccess::kRead;

  // Offset of the biased range in the underlying file; see Bias().
  int bias_start_ = 0;

  // Length of the biased range, or -1 if the file is not biased.
  int bias_length_ = -1;

  // Low-level IO handle, or nullptr when the file is closed.
  void* handle_ = nullptr;

  // Name of the file on disk; empty if none has been assigned.
  std::string filename_;
};

/***********************************************************************************************
 * DiskFile::~DiskFile -- Default deconstructor for a file object. *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle
 *or filename    * associated with it. In order to use a file object created in
 *this fashion it must be     * assigned a name and then opened. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline DiskFile::~DiskFile() {
  // Derived overrides commit their own state in their own destructors;
  // by the time this runs the object is a plain DiskFile.
  DiskFile::Close();
  // filename_ (std::string) automatically cleans up via RAII
}

/***********************************************************************************************
 * DiskFile::IsOpen -- Checks to see if the file is open or not. *
 *                                                                                             *
 *    Use this routine to determine if the file is open. It returns true if it
 *is.             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open? *
 *                                                                                             *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
inline bool DiskFile::IsOpen() const { return handle_ != nullptr; }

#endif  // CNC_RED_ALERT_TECH_DISK_FILE_H_
