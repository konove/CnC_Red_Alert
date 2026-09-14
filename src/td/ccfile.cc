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

/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : March 20, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CCFileClass::CCFileClass -- Default constructor for file object.
 ** CCFileClass::CCFileClass -- Filename based constructor for C&C file. *
 *   CCFileClass::Close -- Closes the file. * CCFileClass::IsAvailable --
 *Checks for existence of file on disk or in mixfile.          *
 *   CCFileClass::IsOpen -- Determines if the file is open. * CCFileClass::Open
 *-- Opens a file from either the mixfile system or the rawfile system.   *
 *   CCFileClass::Read -- Reads data from the file. * CCFileClass::Seek -- Moves
 *the current file pointer in the file.                          *
 *   CCFileClass::Size -- Determines the size of the file. * CCFileClass::Write
 *-- Writes data to the file (non mixfile files only).                   *
 *   CCFileClass::Error -- Handles displaying a file error message. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

// #include	<direct.h>
// #include	<fcntl.h>
// #include	<io.h>
// #include	<dos.h>
#include <iterator>

#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/memflag.h"
#include "sdllib/wwstd.h"
#include "td/ccfile.h"
#include "td/conquer.h"
#include "td/externs.h"
#include "td/jshell.h"
#include "tech/cdfile.h"
#include "tech/file.h"
#include "tech/mixfile.h"
// #include	<share.h>
// #include	"ccfile.h"

/***********************************************************************************************
 * CCFileClass::CCFileClass -- Filename based constructor for C&C file. *
 *                                                                                             *
 *    Use this constructor for a file when the filename is known at construction
 *time.         *
 *                                                                                             *
 * INPUT:   filename -- Pointer to the filename to use for this file object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The filename pointer is presumed to be inviolate throughout the
 *duration of     * the file object. If this is not guaranteed, then use the
 *default constructor    * and then set the name manually. *
 *                                                                                             *
 * HISTORY: * 03/20/1995 JLB : Created. *
 *=============================================================================================*/
CCFileClass::CCFileClass(const std::string_view filename)
    : FromDisk(false), Pointer(nullptr), Start(0), Position(0), Length(0) {
  SetName(filename);
}

/***********************************************************************************************
 * CCFileClass::CCFileClass -- Default constructor for file object. *
 *                                                                                             *
 *    This is the default constructor for a C&C file object. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/20/1995 JLB : Created. *
 *=============================================================================================*/
CCFileClass::CCFileClass()
    : FromDisk(false), Pointer(nullptr), Start(0), Position(0), Length(0) {}

/***********************************************************************************************
 * CCFileClass::Write -- Writes data to the file (non mixfile files only). *
 *                                                                                             *
 *    This routine will write data to the file, but NOT to a file that is part
 *of a mixfile.   *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer that holds the data to be written.
 **
 *                                                                                             *
 *          size     -- The number of bytes to write. *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually written. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
base::ssize CCFileClass::Write(const void* buffer, base::ssize size) {
  // A file inside a mixfile is read-only. This must not fall through: for a
  // resident file the base class would write through a null handle.
  if (Pointer || FromDisk) {
    return 0;
  }

  return CDFileClass::Write(buffer, size);
}

/***********************************************************************************************
 * CCFileClass::Read -- Reads data from the file. *
 *                                                                                             *
 *    This routine determines if the file is part of the mixfile system. If it
 *is, then        * the file is copied from RAM if it is located there.
 *Otherwise it is read from disk       * according to the correct position of
 *the file within the parent mixfile.                 *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to place the read data. *
 *                                                                                             *
 *          size     -- The number of bytes to read. *
 *                                                                                             *
 * OUTPUT:  Returns the actual number of bytes read (this could be less than
 *requested).       *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
base::ssize CCFileClass::Read(void* buffer, base::ssize size) {
  bool opened = false;

  if ((!IsOpen()) && Open()) {
    opened = true;
  }

  /*
  **	If the file is part of a loaded mixfile, then a mere copy is
  **	all that is required for the read.
  */
  if (Pointer) {
    size = std::min(size, Length - Position);
    if (size) {
      Mem_Copy(Add_Long_To_Pointer(Pointer, Position), buffer,
               base::ToSize(size));
      Position += size;
    }
    if (opened) {
      Close();
    }
    return size;
  }

  /*
  **	If the file is part of a mixfile, but the mixfile is located
  **	on disk, then a special read operation is necessary.
  */
  if (FromDisk) {
    size = std::min(size, Length - Position);
    if (size > 0) {
      CDFileClass::Seek(Start + Position, SeekOrigin::kBegin);
      size = CDFileClass::Read(buffer, size);
      Position += size;
    }
    if (opened) {
      Close();
    }
    return size;
  }

  const base::ssize s = CDFileClass::Read(buffer, size);
  if (opened) {
    Close();
  }
  return s;
}

/***********************************************************************************************
 * CCFileClass::Seek -- Moves the current file pointer in the file. *
 *                                                                                             *
 *    This routine will change the current file pointer to the position
 *specified. It follows  * the same rules the a normal Seek() does, but if the
 *file is part of the mixfile system,  * then only the position value needs to
 *be updated.                                        *
 *                                                                                             *
 * INPUT:   pos      -- The position to move the file to relative to the
 *position indicated    * by the "dir" parameter. *
 *                                                                                             *
 *          dir      -- The direction to affect the position change against.
 *This can be       * either SEEK_CUR, SEEK_END, or SEEK_SET. *
 *                                                                                             *
 * OUTPUT:  Returns with the position of the new location. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
base::ssize CCFileClass::Seek(base::ssize offset, SeekOrigin origin) {
  if (Pointer || FromDisk) {
    switch (origin) {
      case SeekOrigin::kEnd:
        Position = Length;
        break;

      case SeekOrigin::kBegin:
        Position = 0;
        break;

      case SeekOrigin::kCurrent:
      default:
        break;
    }
    Position = std::clamp<base::ssize>(Position + offset, 0, Length);
    return Position;
  }
  return CDFileClass::Seek(offset, origin);
}

/***********************************************************************************************
 * CCFileClass::Size -- Determines the size of the file. *
 *                                                                                             *
 *    If the file is part of the mixfile system, then the size of the file is
 *already          * determined and available. Otherwise, go to the low level
 *system to find the file         * size. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the size of the file in bytes. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
base::ssize CCFileClass::Size() {
  if (Pointer || FromDisk) {
    return Length;
  }

  return CDFileClass::Size();
}

/***********************************************************************************************
 * CCFileClass::IsAvailable -- Checks for existence of file on disk or in
 *mixfile.            *
 *                                                                                             *
 *    This routine will examine the mixfile system looking for the file. If the
 *file could     * not be found there, then the disk is examined directly. *
 *                                                                                             *
 * INPUT:   mode -- Ignored for mixfile lookups, passed through for disk checks.
 * *
 *                                                                                             *
 * OUTPUT:  bool; Is the file available for opening? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
bool CCFileClass::IsAvailable() {
  if (MFCD::Offset(FileName()).has_value()) {
    return true;
  }
  return CDFileClass::IsAvailable();
}

/***********************************************************************************************
 * CCFileClass::IsOpen -- Determines if the file is open. *
 *                                                                                             *
 *    A mixfile is open if there is a pointer to the mixfile data. In absence of
 *this,         * the the file is open if the file handle is valid. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
bool CCFileClass::IsOpen() const {
  /*
  **	If the file is part of a cached file, then return that it is opened. A
  *closed file *	doesn't have a valid pointer.
  */
  if (Pointer) {
    return true;
  }
  return CDFileClass::IsOpen();
}

/***********************************************************************************************
 * CCFileClass::Close -- Closes the file. *
 *                                                                                             *
 *    If this is a mixfile file, then only the pointers need to be adjusted. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
void CCFileClass::Close() {
  FromDisk = false;
  Pointer = nullptr;
  Position = 0;  // Starts at beginning offset.
  Start = 0;
  Length = 0;
  CDFileClass::Close();
}

/***********************************************************************************************
 * CCFileClass::Open -- Opens a file from either the mixfile system or the
 *rawfile system.     *
 *                                                                                             *
 *    This routine will open the specified file. It examines the mixfile system
 *to find a      * match. If one is found then the file is "opened" in a special
 *cached way. Otherwise      * it is opened as a standard DOS file. *
 *                                                                                             *
 * INPUT:   rights   -- The access rights desired. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1994 JLB : Created. *
 *=============================================================================================*/
bool CCFileClass::Open(FileAccess rights) {
  /*
  **	Always close the file if it was open.
  */
  Close();

  /*
  **	Perform a preliminary check to see if the specified file
  **	exists on the disk. If it does, then open this file regardless
  **	of whether it also exists in RAM. This is slower, but allows
  **	upgrade files to work.
  */
  if (HasAccess(rights, FileAccess::kWrite) || CDFileClass::IsAvailable()) {
    return CDFileClass::Open(rights);
  }

  /*
  **	Check to see if file is part of a mixfile and that mixfile is currently
  *loaded *	into RAM.
  */
  auto loc = MFCD::Offset(FileName());
  if (loc) {
    /*
    **	If the mixfile is located on disk, then fake out the file system to read
    *from *	the mixfile, but think it is reading from a solitary file.
    */
    if (loc->data.empty()) {
      // Not cached - read from disk
      const int32_t start = loc->offset;
      const int32_t length = loc->size;

      /*
      **	This is a legitimate open to the file. All access to the file
      *through this *	file object will be appropriately adjusted for mixfile
      *support however. Also *	note that the filename attached to this object
      *is NOT the same as the file *	attached to the file handle.
      */
      const std::string dupfile(FileName());
      Open(loc->mixfile->Filename(), FileAccess::kRead);
      SetSearchEnabled(false);  // Disable multi-drive search.
      SetName(dupfile);
      SetSearchEnabled(true);
      Start = start;
      Length = length;
      FromDisk = true;
    } else {
      // Cached in RAM
      Pointer = static_cast<const void*>(loc->data.data());
      Length = loc->size;
    }
  } else {
    /*
    **	The file cannot be found in any mixfile, so it must reside as
    ** an individual file on the disk. Or else it is just plain missing.
    */
    return CDFileClass::Open(rights);
  }
  return true;
}

/***********************************************************************************
** Backward compatibility section.
*/
// extern "C" {

static CCFileClass Handles[10];

int __cdecl OpenFileHandle(const std::string_view file_name, FileAccess mode) {
  for (int index = 0; index < std::ssize(Handles); index++) {
    if (!Handles[index].IsOpen()) {
      Handles[index].SetName(file_name);
      if (Handles[index].Open(mode)) {
        //			if (Handles[index].Open(file_name, mode)) {
        return index;
      }
      break;
    }
  }
  return kInvalidHandle;
}

void __cdecl CloseFileHandle(int handle) {
  if (handle != kInvalidHandle && Handles[handle].IsOpen()) {
    Handles[handle].Close();
  }
}

int32_t __cdecl ReadFileHandle(int handle, void* buffer, int32_t size) {
  if (handle != kInvalidHandle && Handles[handle].IsOpen()) {
    return static_cast<int32_t>(Handles[handle].Read(buffer, size));
  }
  return 0;
}

int32_t __cdecl WriteFileHandle(int handle, const void* buffer, int32_t size) {
  if (handle != kInvalidHandle && Handles[handle].IsOpen()) {
    return static_cast<int32_t>(Handles[handle].Write(buffer, size));
  }
  return 0;
}

bool __cdecl FileExists(const std::string_view file_name) {
  CCFileClass file(file_name);
  return file.IsAvailable();
}

void* __cdecl Load_Alloc_Data(const char* name, int /*unused*/) {
  CCFileClass file(name);

  return Load_Alloc_Data(file);
}

int32_t __cdecl FileHandleSize(int handle) {
  if (handle != kInvalidHandle && Handles[handle].IsOpen()) {
    return static_cast<int32_t>(Handles[handle].Size());
  }
  return 0;
}

int32_t __cdecl SeekFileHandle(int handle, int32_t offset, int origin) {
  if (handle != kInvalidHandle && Handles[handle].IsOpen()) {
    return static_cast<int32_t>(
        Handles[handle].Seek(offset, SeekOriginFromStdio(origin)));
  }
  return 0;
}

void WWDOS_Shutdown() {
  for (auto& Handle : Handles) {
    Handle.SetName({});
  }
}

