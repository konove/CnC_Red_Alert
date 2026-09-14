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

/* $Header: /CounterStrike/XPIPE.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : XPIPE.H *
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

#ifndef CNC_RED_ALERT_TECH_FILE_SINK_H_
#define CNC_RED_ALERT_TECH_FILE_SINK_H_

#include <cstddef>
#include <span>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "tech/byte_sink.h"
#include "tech/file.h"

/*
**	This is a store-to-file pipe terminator. Use it as the final link in a
*pipe process that *	needs to store the data to a file. This can only serve
*as the last link in the chain *	of pipe segments.
*/
class FileSink : public ByteSink {
 public:
  explicit FileSink(File* file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(file), HasOpened(false) {}
  explicit FileSink(File& file ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : file_(&file), HasOpened(false) {}
  ~FileSink() override;

  FileSink(const FileSink&) = delete;
  FileSink& operator=(const FileSink&) = delete;
  FileSink(FileSink&&) = delete;
  FileSink& operator=(FileSink&&) = delete;

  bool Write(std::span<const std::byte> bytes) override;
  bool Finish() override;

 private:
  File* file_;
  bool HasOpened;

  bool Valid_File() { return file_ != nullptr; }
};

#endif  // CNC_RED_ALERT_TECH_FILE_SINK_H_
