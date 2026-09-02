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

/* $Header:   F:\projects\c&c\vcs\code\rawfile.cpv   2.18   16 Oct 1995 16:50:54
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library *
 *                                                                                             *
 *                    File Name : RAWFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 8, 1994 *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * RawFileClass::Close -- Perform a closure of the file. *
 *   RawFileClass::Create -- Creates an empty file. * RawFileClass::Delete --
 *Deletes the file object from the disk.                            *
 *   RawFileClass::Error -- Handles displaying a file error message. *
 *   RawFileClass::Is_Available -- Checks to see if the specified file is
 *available to open.   * RawFileClass::Open -- Assigns name and opens file in
 *one operation.                       * RawFileClass::Open -- Opens the file
 *object with the rights specified.                    *
 *   RawFileClass::RawFileClass -- Simple constructor for a file object. *
 *   RawFileClass::Read -- Reads the specified number of bytes into a memory
 *buffer.           * RawFileClass::Seek -- Reposition the file pointer as
 *indicated.                           * RawFileClass::Set_Name -- Manually sets
 *the name for a file object.                       * RawFileClass::Size --
 *Determines size of file (in bytes).                                 *
 *   RawFileClass::Write -- Writes the specified data to the buffer specified. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "td/function.h"

#include "td/rawfile.h"

/***********************************************************************************************
 * RawFileClass::Error -- Handles displaying a file error message. *
 *                                                                                             *
 *    Display an error message as indicated. If it is allowed to retry, then
 *pressing a key    * will return from this function. Otherwise, it will exit
 *the program with "exit()".       *
 *                                                                                             *
 * INPUT:   error    -- The error number (same as the DOSERR.H error numbers). *
 *                                                                                             *
 *          canretry -- Can this routine exit normally so that retrying can
 *occur? If this is  * false, then the program WILL exit in this routine. *
 *                                                                                             *
 *          filename -- Optional filename to report with this error. If no
 *filename is         * supplied, then no filename is listed in the error
 *message.             *
 *                                                                                             *
 * OUTPUT:  none, but this routine might not return at all if the "canretry"
 *parameter is      * false or the player pressed ESC. *
 *                                                                                             *
 * WARNINGS:   This routine may not return at all. It handles being in text mode
 *as well as    * if in a graphic mode. *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
void RawFileClass::Error(int error, int canretry, const char* filename) {
}

/***********************************************************************************************
 * RawFileClass::RawFileClass -- Simple constructor for a file object. *
 *                                                                                             *
 *    This constructor is called when a file object is created with a supplied
 *filename, but   * not opened at the same time. In this case, an assumption is
 *made that the supplied       * filename is a constant string. A duplicate of
 *the filename string is not created since   * it would be wasteful in that
 *case.                                                       *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
RawFileClass::RawFileClass(const char* filename)
    :
      Handle(nullptr),
      Filename(filename),
      Allocated(false) {
}

RawFileClass::~RawFileClass() {
  if (Allocated) {
    delete[] Filename;
  }
  Allocated = false;
  Filename = nullptr;
}

/***********************************************************************************************
 * RawFileClass::Set_Name -- Manually sets the name for a file object. *
 *                                                                                             *
 *    This routine will set the name for the file object to the name specified.
 *This name is   * duplicated in free store. This allows the supplied name to be
 *a temporarily constructed  * text string. Setting the name in this fashion
 *doesn't affect the closed or opened state  * of the file. *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated copy of this filename. This
 *pointer is     * guaranteed to remain valid for the duration of this file
 *object or until the name  * is changed -- whichever is sooner. *
 *                                                                                             *
 * WARNINGS:   Because of the allocation this routine must perform, memory could
 *become        * fragmented. *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
const char* RawFileClass::Set_Name(const char* filename) {
  if (Allocated) {
    delete[] Filename;
    Filename = nullptr;
    Allocated = false;
  }

  if (!filename) {
    return nullptr;
  }

  size_t len = strlen(filename) + 1;
  char* name_copy = new (std::nothrow) char[len];
  if (!name_copy) {
    Error(ENOMEM, false, filename);
    return nullptr;
  }
  memcpy(name_copy, filename, len);
  Filename = name_copy;
  Allocated = true;
  return Filename;
}

/***********************************************************************************************
 * RawFileClass::Open -- Assigns name and opens file in one operation. *
 *                                                                                             *
 *    This routine will assign the specified filename to the file object and
 *open it at the    * same time. If the file object was already open, then it
 *will be closed first. If the     * file object was previously assigned a
 *filename, then it will be replaced with the new    * name. Typically, this
 *routine is used when an anonymous file object has been crated and  * now it
 *needs to be assigned a name and opened. *
 *                                                                                             *
 * INPUT:   filename -- The filename to assign to this file object. *
 *                                                                                             *
 *          rights   -- The open file access rights to use. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened? The return value of this is moot, since
 *the open file   * is designed to never return unless it succeeded. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
int RawFileClass::Open(const char* filename, FileAccess rights) {
  Set_Name(filename);
  return Open(rights);
}

/***********************************************************************************************
 * RawFileClass::Open -- Opens the file object with the rights specified. *
 *                                                                                             *
 *    This routine is used to open the specified file object with the access
 *rights indicated. * This only works if the file has already been assigned a
 *filename. It is guaranteed, by   * the error handler, that this routine will
 *always return with success.                    *
 *                                                                                             *
 * INPUT:   rights   -- The file access rights to use when opening this file.
 *This is a        * combination of READ and/or WRITE bit flags. *
 *                                                                                             *
 * OUTPUT:  bool; Was the file opened successfully? This will always return true
 *by reason of  * the error handler. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
int RawFileClass::Open(FileAccess rights) {
  Close();

  /*
  **	Verify that there is a filename associated with this file object. If
  *not, then this is a *	big error condition.
  */
  if (!Filename) {
    Error(ENOENT, false);
  }

  /*
  **	Record the access rights used for this open call. These rights will be
  *used if the *	file object is duplicated.
  */
  Rights = rights;

  /*
  **	Repetatively try to open the file. Abort if a fatal error condition
  *occurs.
  */
  for (;;) {
    /*
    **	Try to open the file according to the access rights specified.
    */
    Handle = IO_Open_File(Filename, rights);
    if (!Handle) {
      return false;
    }
    break;
  }
  return true;
}

/***********************************************************************************************
 * RawFileClass::Do_Is_Available -- Checks to see if the specified file is
 *available to open.     *
 *                                                                                             *
 *    This routine will examine the disk system to see if the specified file can
 *be opened     * or not. Use this routine before opening a file in order to
 *make sure that is available   * or to perform other necessary actions. *
 *                                                                                             *
 * INPUT:   mode -- kQuick for fast check that may fail silently, kBlocking for
 *full error      * recovery which may block waiting for media. *
 *                                                                                             *
 * OUTPUT:  bool; Is the file available to be opened? *
 *                                                                                             *
 * WARNINGS:   Depending on the mode passed in, this routine may never
 *return.            *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
int RawFileClass::Do_Is_Available(AvailabilityCheck mode) {
  void* file;
  int open_failed;

  /*
  **	If the file is already open, then is must have already passed the
  *availability check. *	Return true in this case.
  */
  if (Is_Open()) {
    return true;
  }

  /*
  **	If this is a blocking check, then go through the normal open channels,
  *since those *	channels ensure that the file must exist.
  */
  if (mode == AvailabilityCheck::kBlocking) {
    RawFileClass::Open(FileAccess::kRead);
    RawFileClass::Close();
    return true;
  }

  /*
  **	Perform a raw open of the file. If this open fails for ANY REASON,
  *including a missing *	CD-ROM, this routine will return a failure
  *condition. In all but the missing file *	condition, go through the normal
  *error recover channels.
  */
  for (;;) {
    file = IO_Open_File(Filename, FileAccess::kRead);
    if (!file) {
      // retry with lowercase name for case-sensitive fs
      size_t len = strlen(Filename) + 1;
      char* lower_name = new char[len];
      memcpy(lower_name, Filename, len);
      strlwr(lower_name);
      file = IO_Open_File(lower_name, FileAccess::kRead);

      if (file) {
        // if successful, replace the filename with the working one
        if (Allocated) {
          delete[] Filename;
        }

        static_cast<const char*&>(Filename) = lower_name;
        Allocated = true;
      } else {
        delete[] lower_name;
      }
    }

    if (!file) {
      return false;
    }
    break;
  }

  /*
  **	Since the file could be opened, then close it and return that the file
  *exists.
  */
  IO_Close_File(file);
  return true;
}

/***********************************************************************************************
 * RawFileClass::Close -- Perform a closure of the file. *
 *                                                                                             *
 *    Close the file object. In the rare case of an error, handle it as
 *appropriate.           *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Some rare error conditions may cause this routine to abort the
 *program.         *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
void RawFileClass::Close() {
  /*
  **	If the file is open, then close it. If the file is already closed, then
  *just return. This *	isn't considered an error condition.
  */
  if (Is_Open()) {
    for (;;) {
      /*
      **	Close the file. If there was an error in the close operation --
      *abort.
      */
      IO_Close_File(Handle);
      break;
    }

    /*
    **	At this point the file must have been closed. Mark the file as empty and
    *return.
    */
    Handle = nullptr;
  }
}

/***********************************************************************************************
 * RawFileClass::Read -- Reads the specified number of bytes into a memory
 *buffer.             *
 *                                                                                             *
 *    This routine will read the specified number of bytes and place the data
 *into the buffer  * indicated. It is legal to call this routine with a request
 *for more bytes than are in    * the file. This condition can result in fewer
 *bytes being read than requested. Determine  * this by examining the return
 *value.                                                      *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the buffer to read data into. If NULL is
 *passed, no read    * is performed. *
 *                                                                                             *
 *          size     -- The number of bytes to read. If NULL is passed, then no
 *read is        * performed. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes read into the buffer. If this
 *number is less      * than requested, it indicates that the file has been
 *exhausted.                     *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
long RawFileClass::Read(void* buffer, long size) {
  long bytesread =
      0;  // Running count of the number of bytes read into the buffer.
  int opened = false;  // Was the file opened by this routine?
  int readresult;

  /*
  **	If the file isn't opened, open it. This serves as a convenience
  **	for the programmer.
  */
  if (!Is_Open()) {
    /*
    **	The error check here is moot. Open will never return unless it
    *succeeded.
    */
    if (!Open(FileAccess::kRead)) {
      return 0;
    }
    opened = true;
  }

  size_t actual = 0;
  IO_Read_File(Handle, buffer, size, actual);
  bytesread = actual;
  /*
  **	Close the file if it was opened by this routine and return
  **	the actual number of bytes read into the buffer.
  */
  if (opened) {
    Close();
  }
  return bytesread;
}

/***********************************************************************************************
 * RawFileClass::Write -- Writes the specified data to the buffer specified. *
 *                                                                                             *
 *    This routine will write the data specified to the file. *
 *                                                                                             *
 * INPUT:   buffer   -- The buffer that holds the data to write. *
 *                                                                                             *
 *          size     -- The number of bytes to write to the file. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes written to the file. This routine
 *catches the     * case of a disk full condition, so this routine will always
 *return with the number  * matching the size request. *
 *                                                                                             *
 * WARNINGS:   A fatal file condition could cause this routine to never return.
 **
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
long RawFileClass::Write(const void* buffer, long size) {
  long bytesread = 0;
  int opened = false;  // Was the file manually opened?
  int writeresult;

  /*
  **	Check to open status of the file. If the file is open, then merely write
  *to *	it. Otherwise, open the file for writing and then close the file when
  *the *	output is finished.
  */
  if (!Is_Open()) {
    if (!Open(FileAccess::kWrite)) {
      return 0;
    }
    opened = true;
  }

  size_t actual = 0;
  IO_Write_File(Handle, buffer, size, actual);
  bytesread = actual;
  /*
  **	If this routine had to open the file, then close it before returning.
  */
  if (opened) {
    Close();
  }

  /*
  **	Return with the number of bytes written. This will always be the number
  *of bytes *	requested, since the case of the disk being full is caught by
  *this routine.
  */
  return bytesread;
}

/***********************************************************************************************
 * RawFileClass::Seek -- Reposition the file pointer as indicated. *
 *                                                                                             *
 *    Use this routine to move the filepointer to the position indicated. It can
 *move either   * relative to current position or absolute from the beginning or
 *ending of the file. This  * routine will only return if it successfully
 *performed the seek.                          *
 *                                                                                             *
 * INPUT:   pos   -- The position to seek to. This is interpreted as relative to
 *the position  * indicated by the "dir" parameter. *
 *                                                                                             *
 *          dir   -- The relative position to relate the seek to. This can be
 *either SEEK_SET  * for the beginning of the file, SEEK_CUR for the current
 *position, or      * SEEK_END for the end of the file. *
 *                                                                                             *
 * OUTPUT:  This routine returns the position that the seek ended up at. *
 *                                                                                             *
 * WARNINGS:   If there was a file error, then this routine might never return.
 **
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
long RawFileClass::Seek(long pos, int dir) {
  /*
  **	If the file isn't opened, then this is a fatal error condition.
  */
  if (!Is_Open()) {
    Error(EBADF, false, Filename);
  }

  pos = IO_Seek_File(Handle, pos, dir);
  /*
  **	Return with the new position of the file. This will range between zero
  *and the number of *	bytes the file contains.
  */
  return pos;
}

/***********************************************************************************************
 * RawFileClass::Size -- Determines size of file (in bytes). *
 *                                                                                             *
 *    Use this routine to determine the size of the file. The file must exist or
 *this is an    * error condition. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes in the file. *
 *                                                                                             *
 * WARNINGS:   This routine handles error conditions and will not return unless
 *the file       * exists and can successfully be queried for file length. *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
long RawFileClass::Size() {
  long size = 0;

  /*
  **	If the file is open, then proceed normally.
  */
  if (Is_Open()) {
    return IO_Get_File_Size(Handle);
  } else {
    /*
    **	If the file wasn't open, then open the file and call this routine again.
    *Count on *	the fact that the open function must succeed.
    */
    if (Open()) {
      size = Size();

      /*
      **	Since we needed to open the file we must remember to close the
      *file when the *	size has been determined.
      */
      Close();
    }
  }
  return size;
}

/***********************************************************************************************
 * RawFileClass::Create -- Creates an empty file. *
 *                                                                                             *
 *    This routine will create an empty file from the file object. The file
 *object's filename  * must already have been assigned before this routine will
 *function.                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the file successfully created? This routine will always
 *return true.     *
 *                                                                                             *
 * WARNINGS:   A fatal error condition could occur with this routine. Especially
 *if the disk   * is full or a read-only media was selected. *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
int RawFileClass::Create() {
  Close();
  if (Open(FileAccess::kWrite)) {
    Close();
    return true;
  }
  return false;
}

/***********************************************************************************************
 * RawFileClass::Delete -- Deletes the file object from the disk. *
 *                                                                                             *
 *    This routine will delete the file object from the disk. If the file object
 *doesn't       * exist, then this routine will return as if it had succeeded
 *(since the effect is the     * same). *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the file deleted? If the file was already missing, the
 *this value will   * be false. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/18/1994 JLB : Created. *
 *=============================================================================================*/
int RawFileClass::Delete() {
  /*
  **	If the file was open, then it must be closed first.
  */
  Close();

  /*
  **	If there is no filename associated with this object, then this indicates
  *a fatal error *	condition. Report this and abort.
  */
  if (!Filename) {
    Error(ENOENT, false);
  }

  if (!IO_Delete_File(Filename)) {
    return false;
  }
  /*
  **	DOS reports that the file was successfully deleted. Return with this
  *fact.
  */
  return true;
}
