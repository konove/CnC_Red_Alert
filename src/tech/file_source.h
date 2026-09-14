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

#ifndef CNC_RED_ALERT_TECH_FILE_SOURCE_H_
#define CNC_RED_ALERT_TECH_FILE_SOURCE_H_

#include <cstddef>
#include <iterator>
#include <span>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "tech/byte_source.h"
#include "tech/file.h"

/*
**	This class is used to manage a file as a data source. Data requests will
*draw from the *	file until the file has been completely read.
*/
class FileSource : public ByteSource {
 public:
  explicit FileSource(File* file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(file), HasOpened(false) {}
  explicit FileSource(File& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(&file), HasOpened(false) {}
  ~FileSource() override;

  FileSource(const FileSource&) = delete;
  FileSource& operator=(const FileSource&) = delete;
  FileSource(FileSource&&) = delete;
  FileSource& operator=(FileSource&&) = delete;

  base::ssize Read(std::span<std::byte> buffer) override;

 private:
  File* file_;
  bool HasOpened;

  bool Valid_File() { return file_ != nullptr; }
};

#endif  // CNC_RED_ALERT_TECH_FILE_SOURCE_H_
