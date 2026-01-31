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

#ifndef XPIPE_H
#define XPIPE_H

#include "tech/buff.h"
#include "tech/pipe.h"
#include "tech/wwfile.h"

/*
**	This is a simple store-into-buffer pipe terminator. Use it as the final
*link in a pipe process *	that needs to store the data into a memory
*buffer. This can only serve as the final *	link in the chain of pipe
*segments.
*/
class BufferPipe : public Pipe {
 public:
  // Creates a non-owning view into the buffer.
  BufferPipe(const Buffer& buffer)
      : BufferPtr(buffer.Get_Buffer(), buffer.Get_Size()), Index(0) {}
  BufferPipe(void* buffer, int length) : BufferPtr(buffer, length), Index(0) {}
  ~BufferPipe() override = default;

  BufferPipe(const BufferPipe&) = delete;
  BufferPipe& operator=(const BufferPipe&) = delete;
  BufferPipe(BufferPipe&&) = delete;
  BufferPipe& operator=(BufferPipe&&) = delete;

  int Put(const void* source, int slen) override;

 private:
  Buffer BufferPtr;
  int Index;

  bool Is_Valid() { return BufferPtr.Is_Valid(); }
};

/*
**	This is a store-to-file pipe terminator. Use it as the final link in a
*pipe process that *	needs to store the data to a file. This can only serve
*as the last link in the chain *	of pipe segments.
*/
class FilePipe : public Pipe {
 public:
  FilePipe(FileClass* file) : File(file), HasOpened(false) {}
  FilePipe(FileClass& file) : File(&file), HasOpened(false) {}
  ~FilePipe() override;

  FilePipe(const FilePipe&) = delete;
  FilePipe& operator=(const FilePipe&) = delete;
  FilePipe(FilePipe&&) = delete;
  FilePipe& operator=(FilePipe&&) = delete;

  int Put(const void* source, int slen) override;
  int End() override;

 private:
  FileClass* File;
  bool HasOpened;

  bool Valid_File() { return File != nullptr; }
};

#endif
