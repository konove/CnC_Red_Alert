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

/* $Header: /CounterStrike/WWFILE.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : WWFILE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : August 8, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_FILE_H_
#define CNC_RED_ALERT_TECH_FILE_H_

#include <cstdio>
#include <string_view>

#include "base/types.h"
#include "sdllib/file_access.h"

// Where a Seek() offset is measured from.
enum class SeekOrigin { kBegin, kCurrent, kEnd };

// Maps a stdio SEEK_* constant, which the C-style file APIs still pass, to
// SeekOrigin. Anything unrecognized counts as SEEK_CUR, as the file classes
// have always treated it.
constexpr SeekOrigin SeekOriginFromStdio(int origin) {
  switch (origin) {
    case SEEK_SET:
      return SeekOrigin::kBegin;
    case SEEK_END:
      return SeekOrigin::kEnd;
    default:
      return SeekOrigin::kCurrent;
  }
}

// The stdio SEEK_* constant for origin.
constexpr int StdioOrigin(SeekOrigin origin) {
  switch (origin) {
    case SeekOrigin::kBegin:
      return SEEK_SET;
    case SeekOrigin::kEnd:
      return SEEK_END;
    case SeekOrigin::kCurrent:
    default:
      return SEEK_CUR;
  }
}

// File: the interface every file object in the game implements. Concrete
// files live on disk (DiskFile), in memory (MemoryFile) or inside the game's
// mixfile archives (MixAwareFile); code that reads or writes takes a File&.
//
// Originally WWFILE.H (class FileClass) by Joe L. Bostic, August 8, 1994.
class File {
 public:
  File() = default;
  virtual ~File() = default;

  File(const File&) = delete;
  File& operator=(const File&) = delete;
  File(File&&) = delete;
  File& operator=(File&&) = delete;

  // Returns the name bound to the file object, which is empty if none has
  // been assigned. The view is invalidated by the next SetName() or Open()
  // with a name.
  [[nodiscard]] virtual std::string_view FileName() const = 0;

  // Binds filename to the file object without opening it. Derived classes may
  // store a resolved form of the name (such as one with a search path
  // prepended); FileName() returns what was stored. The name is copied.
  virtual void SetName(std::string_view filename) = 0;

  virtual bool Create() = 0;
  virtual bool Delete() = 0;

  // Returns true if the file is available to be opened. Never blocks waiting
  // for media.
  virtual bool IsAvailable() = 0;

  [[nodiscard]] virtual bool IsOpen() const = 0;
  virtual bool Open(std::string_view filename,
                    FileAccess rights = FileAccess::kRead) = 0;
  virtual bool Open(FileAccess rights = FileAccess::kRead) = 0;

  // Reads up to size bytes into buffer and returns the number read, which is
  // less than size only at the end of the file.
  virtual base::ssize Read(void* buffer, base::ssize size) = 0;

  // Writes size bytes from buffer and returns the number written.
  virtual base::ssize Write(const void* buffer, base::ssize size) = 0;

  // Moves the file position by offset from origin and returns the new
  // position, measured from the start of the file.
  virtual base::ssize Seek(base::ssize offset,
                           SeekOrigin origin = SeekOrigin::kCurrent) = 0;

  // Returns the size of the file in bytes.
  virtual base::ssize Size() = 0;

  virtual void Close() = 0;
};

#endif  // CNC_RED_ALERT_TECH_FILE_H_
