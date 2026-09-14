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

/* $Header: /CounterStrike/RAMFILE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RAMFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * MemoryFile::Close -- This will 'close' the ram file. *
 *   MemoryFile::Create -- Effectively clears the buffer of data. *
 *   MemoryFile::Delete -- Effectively clears the buffer of data. *
 *   MemoryFile::IsAvailable -- Determines if the "file" is available. *
 *   MemoryFile::IsOpen -- Is the file open? * MemoryFile::Open -- Opens a
 *RAM based file for read or write.                           *
 *   MemoryFile::Open -- Opens the RAM based file. *
 *   MemoryFile::MemoryFile -- Construct a RAM buffer based "file" object. *
 *   MemoryFile::Read -- Read data from the file. * MemoryFile::Seek --
 *Controls the ram file virtual read position.                        *
 *   MemoryFile::Size -- Returns with the size of the ram file. *
 *   MemoryFile::Write -- Copies data to the ram file. *
 *   MemoryFile::~MemoryFile -- Destructor for the RAM file class. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/memory_file.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "base/numeric.h"
#include "sdllib/file_access.h"

/***********************************************************************************************
 * MemoryFile::MemoryFile -- Construct a RAM buffer based "file" object. *
 *                                                                                             *
 *    This routine will construct a "file" object that actually is just a front
 *end processor  * for a buffer. Access to the buffer will appear as if it was
 *accessing a file. This       * is different from the caching ability of the
 *buffered file class in that this file       * class has no real file
 *counterpart. Typical use of this is for algorithms that were      * originally
 *designed for file processing, but are now desired to work with a buffer. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to use for this file. The buffer
 *will already    * contain data if the file is opened for READ. It will be
 *considered     * a scratch buffer if opened for WRITE. If the buffer pointer
 *is NULL    * but the length parameter is not, then a buffer will be allocated
 ** of the specified length. This case is only useful for opening the      *
 *                      file for WRITE. *
 *                                                                                             *
 *          length   -- The length of the buffer submitted to this routine. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
MemoryFile::MemoryFile(void* buffer, int size)
    : buffer_(static_cast<char*>(buffer)), capacity_(size), size_(size) {
  if (buffer == nullptr && size > 0) {
    buffer_ = new char[base::ToSize(size)];
    owns_buffer_ = true;
  }
}

/***********************************************************************************************
 * MemoryFile::~MemoryFile -- Destructor for the RAM file class. *
 *                                                                                             *
 *    The destructor will deallocate any buffer that it allocated. Otherwise it
 *does nothing.  *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
MemoryFile::~MemoryFile() {
  Close();
  if (owns_buffer_) {
    delete[] buffer_;
    buffer_ = nullptr;
    owns_buffer_ = false;
  }
}

/***********************************************************************************************
 * MemoryFile::Create -- Effectively clears the buffer of data. *
 *                                                                                             *
 *    This routine "clears" the buffer of data. It only makes the buffer appear
 *empty by       * resetting the internal length to zero. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Was the file reset in this fashion? *
 *                                                                                             *
 * WARNINGS:   If the file was open, then resetting by this routine is not
 *allowed.            *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::Create() {
  if (!IsOpen()) {
    size_ = 0;
    return true;
  }
  return false;
}

/***********************************************************************************************
 * MemoryFile::Delete -- Effectively clears the buffer of data. *
 *                                                                                             *
 *    This routine "clears" the buffer of data. It only makes the buffer appear
 *empty by       * resetting the internal length to zero. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Was the file reset in this fashion? *
 *                                                                                             *
 * WARNINGS:   If the file was open, then resetting by this routine is not
 *allowed.            *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::Delete() {
  if (!IsOpen()) {
    size_ = 0;
    return true;
  }
  return false;
}

/***********************************************************************************************
 * MemoryFile::IsAvailable -- Determines if the "file" is available. *
 *                                                                                             *
 *    RAM files are always available. *
 *                                                                                             *
 * INPUT:   mode -- Ignored for RAM files. *
 *                                                                                             *
 * OUTPUT:  true *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::IsAvailable() { return true; }

/***********************************************************************************************
 * MemoryFile::IsOpen -- Is the file open? *
 *                                                                                             *
 *    This answers the question whether the file is open or not. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::IsOpen() const { return is_open_; }

/***********************************************************************************************
 * MemoryFile::Open -- Opens a RAM based file for read or write. *
 *                                                                                             *
 *    This routine will open the ram file. The name is meaningless so that
 *parameter is        * ignored. If the access mode is for write, then the
 *pseudo-file can be written until the  * buffer is full. If the file is opened
 *for read, then the buffer is presumed to be full   * of the data to be read. *
 *                                                                                             *
 * INPUT:   name  -- ignored. *
 *                                                                                             *
 *          access-- The access method to use for the data buffer -- either READ
 *or WRITE.     *
 *                                                                                             *
 * OUTPUT:  bool; Was the open successful? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::Open(std::string_view /*filename*/, FileAccess access) {
  return Open(access);
}

/***********************************************************************************************
 * MemoryFile::Open -- Opens the RAM based file. *
 *                                                                                             *
 *    This will open the ram based file for read or write. If the file is opened
 *for write,    * the the 'file' can be written up to the limit of the buffer's
 *size. If the file is       * opened for read, then the buffer is presumed to
 *hold the data to be read.                *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool MemoryFile::Open(FileAccess access) {
  if (buffer_ == nullptr || IsOpen()) {
    return false;
  }

  position_ = 0;
  access_ = access;
  is_open_ = true;

  switch (access) {
    default:
    case FileAccess::kRead:
      break;

    case FileAccess::kWrite:
      size_ = 0;
      break;

    case FileAccess::kReadWrite:
      break;
  }

  return IsOpen();
}

/***********************************************************************************************
 * MemoryFile::Read -- Read data from the file. *
 *                                                                                             *
 *    Use this routine just like a normal file read. It will copy the bytes from
 *the ram       * buffer to the destination specified. When the ram buffer is
 *exhausted, less bytes than   * requested will be read. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to store the data to. *
 *                                                                                             *
 *          size     -- The number of bytes to 'read' into the specified buffer.
 **
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes copied to the destination buffer.
 *If the number   * of bytes returned is less than requested, then this
 *indicates that the source      * buffer is exhausted. *
 *                                                                                             *
 * WARNINGS:   The read function only applies to ram 'files' opened for read
 *access.           *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
int32_t MemoryFile::Read(void* buffer, int32_t size) {
  if (buffer_ == nullptr || buffer == nullptr || size == 0) {
    return 0;
  }

  bool opened_here = false;
  if (!IsOpen()) {
    Open(FileAccess::kRead);
    opened_here = true;
  } else {
    if (!HasAccess(access_, FileAccess::kRead)) {
      return 0;
    }
  }

  const int bytes_to_copy = size < size_ - position_ ? size : size_ - position_;
  memmove(buffer, &buffer_[position_], base::ToSize(bytes_to_copy));
  position_ += bytes_to_copy;

  if (opened_here) {
    Close();
  }

  return bytes_to_copy;
}

/***********************************************************************************************
 * MemoryFile::Seek -- Controls the ram file virtual read position. *
 *                                                                                             *
 *    This routine will move the read/write position of the ram file to the
 *location specified * by the offset and direction parameters. It functions
 *similarly to the regular file       * seek method. *
 *                                                                                             *
 * INPUT:   offset   -- The signed offset from the home position specified by
 * the "origin"           * parameter. *
 *                                                                                             *
 *          origin   -- The home position to base the position offset on. This
 * will either be     * the start of the file, the end of the file, or the
 * current read/write     * position. *
 *                                                                                             *
 * OUTPUT:  Returns with the new file position. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
int32_t MemoryFile::Seek(int32_t offset, int origin) {
  if (buffer_ == nullptr || !IsOpen()) {
    return position_;
  }

  int max_position = size_;
  if (HasAccess(access_, FileAccess::kWrite)) {
    max_position = capacity_;
  }

  switch (origin) {
    case SEEK_CUR:
      position_ = position_ + offset;
      break;

    case SEEK_SET:
      position_ = offset;
      break;

    case SEEK_END:
      position_ = max_position + offset;
      break;
    default:
      break;
  }

  position_ = std::clamp(position_, 0, max_position);
  size_ = std::max(position_, size_);

  return position_;
}

/***********************************************************************************************
 * MemoryFile::Size -- Returns with the size of the ram file. *
 *                                                                                             *
 *    This will return the size of the 'real' data in the ram file. The real
 *data is either    * the entire buffer, if opened for READ, or just the written
 *data if opened for WRITE.     *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes that the ram file system considers
 *to be valid    * data of the 'file'. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
int32_t MemoryFile::Size() { return size_; }

/***********************************************************************************************
 * MemoryFile::Write -- Copies data to the ram file. *
 *                                                                                             *
 *    This function similarly to the regular write operation supported for
 *files. It copies    * the data specified to the current write position in the
 *ram file.                        *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the data to be written. *
 *                                                                                             *
 *          size     -- The number of bytes to write to the file. *
 *                                                                                             *
 * OUTPUT:  Returns with the actual number of bytes written. This will be less
 *than requested  * if the buffer is exhausted of space prematurely. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
int32_t MemoryFile::Write(const void* buffer, int32_t size) {
  if (buffer_ == nullptr || buffer == nullptr || size == 0) {
    return 0;
  }

  bool opened_here = false;
  if (!IsOpen()) {
    Open(FileAccess::kWrite);
    opened_here = true;
  } else {
    if (!HasAccess(access_, FileAccess::kWrite)) {
      return 0;
    }
  }

  const int space_left = capacity_ - position_;
  const int bytes_to_write = size < space_left ? size : space_left;
  memmove(&buffer_[position_], buffer, base::ToSize(bytes_to_write));
  position_ += bytes_to_write;

  size_ = std::max(position_, size_);

  if (opened_here) {
    Close();
  }

  return bytes_to_write;
}

/***********************************************************************************************
 * MemoryFile::Close -- This will 'close' the ram file. *
 *                                                                                             *
 *    Closing a ram file actually does nothing but record that it is now closed.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
void MemoryFile::Close() { is_open_ = false; }
