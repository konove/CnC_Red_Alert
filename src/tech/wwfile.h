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

#ifndef CNC_RED_ALERT_TECH_WWFILE_H_
#define CNC_RED_ALERT_TECH_WWFILE_H_

#define YEAR(dt) ((((dt) & 0xFE000000) >> (9 + 16)) + 1980)
#define MONTH(dt) (((dt) & 0x01E00000) >> (5 + 16))
#define DAY(dt) (((dt) & 0x001F0000) >> (0 + 16))
#define HOUR(dt) (((dt) & 0x0000F800) >> 11)
#define MINUTE(dt) (((dt) & 0x000007E0) >> 5)
#define SECOND(dt) (((dt) & 0x0000001F) << 1)

#include <cstdint>
#include <cstdio>

#include "sdllib/file_access.h"

// Specifies how thoroughly to check file availability.
enum class AvailabilityCheck {
  kQuick,     // Fast check, may return false for temporarily unavailable files
  kBlocking,  // Full check with error recovery, may block waiting for media
};

class FileClass {
 public:
  FileClass() = default;
  virtual ~FileClass() = default;

  FileClass(const FileClass&) = delete;
  FileClass& operator=(const FileClass&) = delete;
  FileClass(FileClass&&) = delete;
  FileClass& operator=(FileClass&&) = delete;

  [[nodiscard]] virtual const char* FileName() const = 0;
  virtual const char* SetName(const char* filename) = 0;
  virtual bool Create() = 0;
  virtual bool Delete() = 0;

  // Returns true if the file is available to be opened.
  bool IsAvailable() { return DoIsAvailable(AvailabilityCheck::kQuick); }

  // Returns true if the file is available. Uses full error recovery which may
  // block waiting for media (e.g., prompting for CD-ROM).
  bool IsAvailableStrict() {
    return DoIsAvailable(AvailabilityCheck::kBlocking);
  }

  [[nodiscard]] virtual bool IsOpen() const = 0;
  virtual bool Open(const char* filename,
                    FileAccess rights = FileAccess::kRead) = 0;
  virtual bool Open(FileAccess rights = FileAccess::kRead) = 0;
  virtual int32_t Read(void* buffer, int32_t size) = 0;
  virtual int32_t Seek(int32_t offset, int origin = SEEK_CUR) = 0;
  virtual int32_t Size() = 0;
  virtual int32_t Write(const void* buffer, int32_t size) = 0;
  virtual void Close() = 0;
  virtual void Error(int error, bool can_retry = false,
                     const char* filename = nullptr) = 0;

  // legacy C interfaces take the object where a pointer or name is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator const char*() const { return FileName(); }

 protected:
  virtual bool DoIsAvailable(AvailabilityCheck mode) = 0;
};

#endif  // CNC_RED_ALERT_TECH_WWFILE_H_
