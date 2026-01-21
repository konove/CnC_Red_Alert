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

#ifndef XSTRAW_H
#define XSTRAW_H

#include "tech/buff.h"
#include "tech/straw.h"
#include "tech/wwfile.h"

/*
**	This class is used to manage a buffer as a data source. Data requests
*will draw from the *	buffer supplied until the buffer is exhausted.
*/
class BufferStraw : public Straw {
 public:
  // Creates a non-owning view into the buffer.
  BufferStraw(const Buffer& buffer)
      : BufferPtr(buffer.Get_Buffer(), buffer.Get_Size()), Index(0) {}
  BufferStraw(void const* buffer, int length)
      : BufferPtr((void*)buffer, length), Index(0) {}
  ~BufferStraw() override = default;

  BufferStraw(const BufferStraw&) = delete;
  BufferStraw& operator=(const BufferStraw&) = delete;
  BufferStraw(BufferStraw&&) = delete;
  BufferStraw& operator=(BufferStraw&&) = delete;

  int Get(void* source, int slen) override;

 private:
  Buffer BufferPtr;
  int Index;

  bool Is_Valid() { return (BufferPtr.Is_Valid()); }
};

/*
**	This class is used to manage a file as a data source. Data requests will
*draw from the *	file until the file has been completely read.
*/
class FileStraw : public Straw {
 public:
  FileStraw(FileClass* file) : File(file), HasOpened(false) {}
  FileStraw(FileClass& file) : File(&file), HasOpened(false) {}
  ~FileStraw() override;

  FileStraw(const FileStraw&) = delete;
  FileStraw& operator=(const FileStraw&) = delete;
  FileStraw(FileStraw&&) = delete;
  FileStraw& operator=(FileStraw&&) = delete;

  int Get(void* source, int slen) override;

 private:
  FileClass* File;
  bool HasOpened;

  bool Valid_File() { return (File != nullptr); }
};

#endif
