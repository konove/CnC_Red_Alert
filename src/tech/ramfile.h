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

#include "absl/base/attributes.h"
#include "tech/wwfile.h"

class RAMFileClass final : public FileClass {
 public:
  RAMFileClass(void* buffer ABSL_ATTRIBUTE_LIFETIME_BOUND, int len);
  ~RAMFileClass() override;

  RAMFileClass(const RAMFileClass&) = delete;
  RAMFileClass& operator=(const RAMFileClass&) = delete;
  RAMFileClass(RAMFileClass&&) = delete;
  RAMFileClass& operator=(RAMFileClass&&) = delete;

  [[nodiscard]] const char* File_Name() const override { return "UNKNOWN"; }
  const char* Set_Name(const char* /*filename*/) override {
    return File_Name();
  }
  int Create() override;
  int Delete() override;
  [[nodiscard]] int Is_Open() const override;
  int Open(const char* filename,
           FileAccess access = FileAccess::kRead) override;
  int Open(FileAccess access = FileAccess::kRead) override;
  int32_t Read(void* buffer, int32_t size) override;
  int32_t Seek(int32_t pos, int dir = SEEK_CUR) override;
  int32_t Size() override;
  int32_t Write(const void* buffer, int32_t size) override;
  void Close() override;
  void Error(int /*error*/, int /*canretry*/ = false,
             const char* /*filename*/ = nullptr) override {}

 protected:
  int Do_Is_Available(AvailabilityCheck mode) override;

 private:
  /*
  **	Pointer to the buffer that the "file" will reside in.
  */
  char* Buffer;

  /*
  **	The maximum size of the buffer. The file occupying the buffer
  **	may be smaller than this size.
  */
  int MaxLength;

  /*
  **	The number of bytes in the sub-file occupying the buffer.
  */
  int Length;

  /*
  **	The current file position offset within the buffer.
  */
  int Offset = 0;

  /*
  **	The file was opened with this access mode.
  */
  FileAccess Access = FileAccess::kRead;

  /*
  **	Is the file currently open?
  */
  bool IsOpen = false;

  /*
  **	Was the file buffer allocated during construction of this object?
  */
  bool IsAllocated = false;
};

#endif  // CNC_RED_ALERT_TECH_RAMFILE_H_
