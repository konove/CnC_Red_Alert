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

#include "absl/base/attributes.h"
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
  explicit BufferIOFileClass(const char* filename);
  BufferIOFileClass();
  ~BufferIOFileClass() override;

  BufferIOFileClass(const BufferIOFileClass&) = delete;
  BufferIOFileClass& operator=(const BufferIOFileClass&) = delete;
  BufferIOFileClass(BufferIOFileClass&&) = delete;
  BufferIOFileClass& operator=(BufferIOFileClass&&) = delete;

  bool Cache(int32_t size = 0, void* ptr = nullptr);
  void Free();
  bool Commit();
  const char* Set_Name(const char* filename)
      ABSL_ATTRIBUTE_LIFETIME_BOUND override;
  [[nodiscard]] int Is_Open() const override;
  int Open(const char* filename,
           FileAccess rights = FileAccess::kRead) override;
  int Open(FileAccess rights = FileAccess::kRead) override;
  int32_t Read(void* buffer, int32_t size) override;
  int32_t Seek(int32_t pos, int dir = SEEK_CUR) override;
  int32_t Size() override;
  int32_t Write(const void* buffer, int32_t size) override;
  void Close() override;

  enum { MINIMUM_BUFFER_SIZE = 1024 };

 protected:
  int Do_Is_Available(AvailabilityCheck mode) override;

 private:
  unsigned IsAllocated : 1;
  unsigned IsOpen : 1;
  unsigned IsDiskOpen : 1;
  unsigned IsCached : 1;
  unsigned IsChanged : 1;
  unsigned UseBuffer : 1;

  FileAccess BufferRights;

  void* Buffer;

  int32_t BufferSize;
  int32_t BufferPos;
  int32_t BufferFilePos;
  int32_t BufferChangeBeg;
  int32_t BufferChangeEnd;
  int32_t FileSize;
  int32_t FilePos;
  int32_t TrueFileStart;
};

#endif  // CNC_RED_ALERT_TECH_BFIOFILE_H_
