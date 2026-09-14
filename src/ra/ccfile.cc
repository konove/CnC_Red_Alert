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

// File: CCFileClass implementation and the integer-handle file API built on it.
//
// Originally CCFILE.CPP by Joe L. Bostic, started August 8, 1994.

#include "ra/ccfile.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <new>
#include <optional>
#include <span>
#include <string>

#include "base/numeric.h"
#include "ra/conquer.h"
#include "ra/externs.h"
#include "ra/jshell.h"
#include "ra/startup.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/cdfile.h"
#include "tech/mixfile.h"
#include "tech/rawfile.h"
#include "tech/wwfile.h"

// The name is copied by Set_Name, so filename need not outlive the object.
CCFileClass::CCFileClass(const char* filename) : Position(0) {
  CCFileClass::Set_Name(filename);
}

CCFileClass::CCFileClass() : Position(0) {}

void CCFileClass::Error(int /*error*/, bool /*canretry*/,
                        const char* /*filename*/) {
  // A missing CD is the only failure this can recover from, so ask for the
  // disc and give up if the player cancels.
  if (!Force_CD_Available(RequiredCD)) {
    Emergency_Exit(EXIT_FAILURE);
  }
}

int32_t CCFileClass::Write(const void* buffer, int32_t size) {
  // A resident file is a view into the mixfile cache, so writing is not
  // allowed. It must not fall through: Is_Open() reports the resident file as
  // open, so the base class would skip opening a handle and write through a
  // null one. Error() is no help here, since it only prompts for a CD and
  // returns whenever the disc is present.
  if (Is_Resident()) {
    return 0;
  }

  return CDFileClass::Write(buffer, size);
}

int32_t CCFileClass::Read(void* buffer, int32_t size) {
  bool opened = false;

  // A read on a closed file opens it for just this call.
  if ((!Is_Open()) && Open()) {
    opened = true;
  }

  // If the file is part of a cached mixfile, then a mere copy is all that is
  // required for the read, clipped to the bytes left after Position.
  if (Is_Resident()) {
    const int32_t maximum = static_cast<int32_t>(Data.Get_Size()) - Position;

    size = maximum < size ? maximum : size;
    if (size) {
      memmove(buffer, static_cast<char*>(Data) + Position, base::ToSize(size));
      Position += size;
    }
    if (opened) {
      Close();
    }
    return size;
  }

  // A file on disk, or one inside a mixfile on disk (the bias set up by Open
  // keeps the read inside the embedded file).
  const int32_t s = CDFileClass::Read(buffer, size);

  // If the file was opened by this routine, then close it at this time.
  if (opened) {
    Close();
  }

  return s;
}

int32_t CCFileClass::Seek(int32_t pos, int dir) {
  // When the file is resident, a mere adjustment of the virtual file position
  // is all that is required of a seek. An unrecognized dir is treated as
  // SEEK_CUR.
  if (Is_Resident()) {
    switch (dir) {
      case SEEK_END:
        Position = static_cast<int32_t>(Data.Get_Size());
        break;

      case SEEK_SET:
        Position = 0;
        break;

      case SEEK_CUR:
      default:
        break;
    }
    // Clamp rather than fail, so the position always stays inside the image
    // and Read's size arithmetic cannot go negative.
    Position += pos;
    Position = Position < 0 ? 0 : Position;
    Position = Position > static_cast<int32_t>(Data.Get_Size())
                   ? static_cast<int32_t>(Data.Get_Size())
                   : Position;
    return Position;
  }
  return CDFileClass::Seek(pos, dir);
}

int32_t CCFileClass::Size() {
  // If the file is resident, the size is already known.
  if (Is_Resident()) {
    return static_cast<int32_t>(Data.Get_Size());
  }

  // If the file is not available as a stand-alone file, then search for it in
  // the mixfiles to get its size without opening it (added August 1996). A file
  // open on a mixfile on disk does not take this path: its handle is open, so
  // the check succeeds and the biased CDFileClass::Size() below reports the
  // embedded length.
  if (!CDFileClass::Do_Is_Available(AvailabilityCheck::kQuick)) {
    if (auto loc = MFCD::Offset(File_Name())) {
      return loc->size;
    }
    return 0;
  }

  return CDFileClass::Size();
}

bool CCFileClass::Delete() {
  Close();

  // Only a loose file on disk can be deleted. Without this check the base class
  // would take the mixfile lookup in Do_Is_Available() as proof that the file
  // exists and then try to delete a disk file that is not there.
  if (!CDFileClass::Do_Is_Available(AvailabilityCheck::kQuick)) {
    return false;
  }
  return CDFileClass::Delete();
}

bool CCFileClass::Do_Is_Available(AvailabilityCheck mode) {
  // A file that is open is presumed available.
  if (Is_Open()) {
    return true;
  }

  // A file that is part of a mixfile is also presumed available. This is
  // checked before the disk because it is a lookup in memory, and it cannot
  // block waiting for media.
  if (MFCD::Offset(File_Name()).has_value()) {
    return true;
  }

  // Otherwise a manual check of the file system is required to determine if the
  // file is actually available.
  return CDFileClass::Do_Is_Available(mode);
}

bool CCFileClass::Is_Open() const {
  // A resident file has no file handle; holding a pointer into the mixfile
  // image is what makes it open. Close() clears that pointer.
  if (Is_Resident()) {
    return true;
  }

  // Otherwise, go to a lower level to determine if the file is open.
  return CDFileClass::Is_Open();
}

void CCFileClass::Close() {
  // Reconstructs Data in place as an empty buffer without destroying the old
  // one. That leaks nothing because Open only ever points Data at memory the
  // mixfile cache owns.
  new (&Data)::Buffer;
  Position = 0;
  CDFileClass::Close();
}

bool CCFileClass::Open(FileAccess rights) {
  // Always close the file if it was open.
  Close();

  // Perform a preliminary check to see if the specified file exists on the
  // disk. If it does, then open this file regardless of whether it also exists
  // in a mixfile. This is slower, but allows upgrade files to work. Writes
  // always go to disk, since mixfile contents are read-only.
  if (HasAccess(rights, FileAccess::kWrite) ||
      CDFileClass::Do_Is_Available(AvailabilityCheck::kQuick)) {
    return CDFileClass::Open(rights);
  }

  // Check to see if the file is part of a registered mixfile.
  auto loc = MFCD::Offset(File_Name());
  if (!loc) {
    // The file cannot be found in any mixfile, so it must reside as an
    // individual file on the disk. Or else it is just plain missing, and the
    // disk open reports it.
    return CDFileClass::Open(rights);
  }

  // An empty data span means the mixfile is registered but not cached, so it
  // is still on disk. Fake out the file system to read from the mixfile, but
  // think it is reading from a solitary file.
  if (loc->data.empty()) {
    // This is a legitimate open of the mixfile itself. All access through this
    // file object is adjusted for mixfile support, however. Also note that the
    // filename attached to this object is NOT the same as the file attached to
    // the file handle.
    const std::string dupfile = File_Name();
    Open(loc->mixfile->Filename().c_str(), FileAccess::kRead);
    // Put the embedded file's name back. Searching is off so Set_Name takes the
    // name verbatim instead of probing the search paths for it.
    Searching(false);
    Set_Name(dupfile.c_str());
    Searching(true);
    // The bias must be set after Set_Name, which clears it; Bias() adds start
    // to the existing bias rather than replacing it. offset is absolute within
    // the mixfile here, since the mixfile is not cached.
    Bias(loc->offset, loc->size);
    Seek(0, SEEK_SET);
  } else {
    // Cached mixfile: point at the file's bytes in the RAM image. The handle
    // stays closed.
    new (&Data)::Buffer(loc->data.data(), base::ToSigned(loc->data.size()));
    Position = 0;
  }

  return true;
}

// Integer-handle file API declared in sdllib/file.h, for code outside the game
// (audio streaming, PCX writing) that cannot use CCFileClass directly. A handle
// is an index into Handles, so at most this many files are open through it at
// once.
static CCFileClass Handles[10];

// Returns the open file behind handle, or nullptr if handle is WWERROR, out of
// range, or closed.
static CCFileClass* FindOpenHandle(int handle) {
  if (handle < 0 || handle >= std::ssize(Handles) ||
      !Handles[handle].Is_Open()) {
    return nullptr;
  }
  return &Handles[handle];
}

// Returns the handle, or WWERROR if the open failed or every handle is in use.
int __cdecl Open_File(const char* file_name, FileAccess mode) {
  for (int index = 0; index < std::ssize(Handles); index++) {
    if (!Handles[index].Is_Open()) {
      if (Handles[index].Open(file_name, mode)) {
        return index;
      }
      // The first free slot is as good as any other; a failed open would fail
      // in them all.
      break;
    }
  }
  return WWERROR;
}

void __cdecl Close_File(int handle) {
  if (CCFileClass* const file = FindOpenHandle(handle)) {
    file->Close();
  }
}

int32_t __cdecl Read_File(int handle, void* buf, int32_t bytes) {
  if (CCFileClass* const file = FindOpenHandle(handle)) {
    return file->Read(buf, bytes);
  }
  return 0;
}

int32_t __cdecl Write_File(int handle, const void* buf, int32_t bytes) {
  if (CCFileClass* const file = FindOpenHandle(handle)) {
    return file->Write(buf, bytes);
  }
  return 0;
}

bool __cdecl Find_File(const char* file_name) {
  CCFileClass file(file_name);
  return file.Is_Available();
}

void* __cdecl Load_Alloc_Data(const char* name, int /*unused*/) {
  CCFileClass file(name);

  return Load_Alloc_Data(file);
}

int32_t __cdecl File_Size(int handle) {
  if (CCFileClass* const file = FindOpenHandle(handle)) {
    return file->Size();
  }
  return 0;
}

int32_t __cdecl Seek_File(int handle, int32_t offset, int starting) {
  if (CCFileClass* const file = FindOpenHandle(handle)) {
    return file->Seek(offset, starting);
  }
  return 0;
}
