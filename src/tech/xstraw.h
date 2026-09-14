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

/* $Header: /CounterStrike/XSTRAW.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : XSTRAW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/04/96 *
 *                                                                                             *
 *                  Last Update : July 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_XSTRAW_H_
#define CNC_RED_ALERT_TECH_XSTRAW_H_

#include <cstddef>
#include <iterator>
#include <span>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "tech/file.h"
#include "tech/straw.h"

/*
**	This class is used to manage a buffer as a data source. Data requests
*will draw from the *	buffer supplied until the buffer is exhausted.
*/
class BufferStraw : public Straw {
 public:
  // Reads from buffer, which must outlive the straw.
  explicit BufferStraw(
      std::span<const std::byte> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : buffer_(buffer) {}

  base::ssize Get(std::span<std::byte> buffer) override;

  // Returns the number of bytes not yet handed out.
  [[nodiscard]] base::ssize bytes_remaining() const {
    return std::ssize(buffer_) - index_;
  }

 private:
  std::span<const std::byte> buffer_;
  base::ssize index_ = 0;  // Bytes handed out so far.
};

/*
**	This class is used to manage a file as a data source. Data requests will
*draw from the *	file until the file has been completely read.
*/
class FileStraw : public Straw {
 public:
  explicit FileStraw(File* file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(file), HasOpened(false) {}
  explicit FileStraw(File& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(&file), HasOpened(false) {}
  ~FileStraw() override;

  FileStraw(const FileStraw&) = delete;
  FileStraw& operator=(const FileStraw&) = delete;
  FileStraw(FileStraw&&) = delete;
  FileStraw& operator=(FileStraw&&) = delete;

  base::ssize Get(std::span<std::byte> buffer) override;

 private:
  File* file_;
  bool HasOpened;

  bool Valid_File() { return file_ != nullptr; }
};

#endif  // CNC_RED_ALERT_TECH_XSTRAW_H_
