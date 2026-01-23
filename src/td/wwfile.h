/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\wwfile.h_v   2.15   16 Oct 1995 16:46:06
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_WWFILE_H_
#define CNC_RED_ALERT_TD_WWFILE_H_

#include <cstdio>

#ifndef READ
#define READ _READ
#endif
#ifndef WRITE
#define WRITE _WRITE
#endif

// Specifies how thoroughly to check file availability.
enum class AvailabilityCheck {
  kQuick,     // Fast check, may return false for temporarily unavailable files
  kBlocking,  // Full check with error recovery, may block waiting for media
};

class FileClass {
 public:
  virtual ~FileClass() {};
  virtual char const* File_Name() const = 0;
  virtual char const* Set_Name(char const* filename) = 0;
  virtual int Create() = 0;
  virtual int Delete() = 0;

  // Returns true if the file is available to be opened.
  int Is_Available() { return Do_Is_Available(AvailabilityCheck::kQuick); }

  // Returns true if the file is available. Uses full error recovery which may
  // block waiting for media (e.g., prompting for CD-ROM).
  int Is_Available_Strict() {
    return Do_Is_Available(AvailabilityCheck::kBlocking);
  }

  virtual int Is_Open() const = 0;
  virtual int Open(char const* filename, int rights = READ) = 0;
  virtual int Open(int rights = READ) = 0;
  virtual long Read(void* buffer, long size) = 0;
  virtual long Seek(long pos, int dir = SEEK_CUR) = 0;
  virtual long Size() = 0;
  virtual long Write(void const* buffer, long size) = 0;
  virtual void Close() = 0;

  operator char const*() { return File_Name(); };

 protected:
  virtual int Do_Is_Available(AvailabilityCheck mode) = 0;
};

#endif  // CNC_RED_ALERT_TD_WWFILE_H_
