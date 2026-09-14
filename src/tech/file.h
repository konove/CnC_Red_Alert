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

#define YEAR(dt) ((((dt) & 0xFE000000) >> (9 + 16)) + 1980)
#define MONTH(dt) (((dt) & 0x01E00000) >> (5 + 16))
#define DAY(dt) (((dt) & 0x001F0000) >> (0 + 16))
#define HOUR(dt) (((dt) & 0x0000F800) >> 11)
#define MINUTE(dt) (((dt) & 0x000007E0) >> 5)
#define SECOND(dt) (((dt) & 0x0000001F) << 1)

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "sdllib/file_access.h"

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
  virtual int32_t Read(void* buffer, int32_t size) = 0;
  virtual int32_t Seek(int32_t offset, int origin = SEEK_CUR) = 0;
  virtual int32_t Size() = 0;
  virtual int32_t Write(const void* buffer, int32_t size) = 0;
  virtual void Close() = 0;
  virtual void Error(int error, bool can_retry = false,
                     std::string_view filename = {}) = 0;
};

#endif  // CNC_RED_ALERT_TECH_FILE_H_
