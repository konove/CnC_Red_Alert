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

// File: MixAwareFile implementation and the integer-handle file API built on
// it.
//
// Originally CCFILE.CPP by Joe L. Bostic, started August 8, 1994.

#include "ra/mix_aware_file.h"

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
#include "ra/startup.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/cdfile.h"
#include "tech/mixfile.h"
#include "tech/rawfile.h"
#include "tech/wwfile.h"

// The name is copied by SetName, so filename need not outlive the object.
MixAwareFile::MixAwareFile(const char* filename) {
  MixAwareFile::SetName(filename);
}

MixAwareFile::MixAwareFile() = default;

void MixAwareFile::Error(int /*error*/, bool /*can_retry*/,
                         const char* /*filename*/) {
  // A missing CD is the only failure this can recover from, so ask for the
  // disc and give up if the player cancels.
  if (!Force_CD_Available(RequiredCD)) {
    Emergency_Exit(EXIT_FAILURE);
  }
}

int32_t MixAwareFile::Write(const void* buffer, int32_t size) {
  // A resident file is a view into the mixfile cache, so writing is not
  // allowed. It must not fall through: IsOpen() reports the resident file as
  // open, so the base class would skip opening a handle and write through a
  // null one. Error() is no help here, since it only prompts for a CD and
  // returns whenever the disc is present.
  if (IsResident()) {
    return 0;
  }

  return CDFileClass::Write(buffer, size);
}

int32_t MixAwareFile::Read(void* buffer, int32_t size) {
  // A read on a closed file opens it for just this call.
  const bool opened_for_this_read = !IsOpen() && Open();

  // If the file is part of a cached mixfile, then a mere copy is all that is
  // required for the read, clipped to the bytes left after the position.
  if (IsResident()) {
    const int32_t bytes_left =
        static_cast<int32_t>(resident_data_.Get_Size()) - resident_position_;

    size = bytes_left < size ? bytes_left : size;
    if (size) {
      memmove(buffer, static_cast<char*>(resident_data_) + resident_position_,
              base::ToSize(size));
      resident_position_ += size;
    }
    if (opened_for_this_read) {
      Close();
    }
    return size;
  }

  // A file on disk, or one inside a mixfile on disk (the bias set up by Open
  // keeps the read inside the embedded file).
  const int32_t bytes_read = CDFileClass::Read(buffer, size);

  // If the file was opened by this routine, then close it at this time.
  if (opened_for_this_read) {
    Close();
  }

  return bytes_read;
}

int32_t MixAwareFile::Seek(int32_t offset, int origin) {
  // When the file is resident, a mere adjustment of the virtual file position
  // is all that is required of a seek. An unrecognized origin is treated as
  // SEEK_CUR.
  if (IsResident()) {
    const auto image_size = static_cast<int32_t>(resident_data_.Get_Size());
    switch (origin) {
      case SEEK_END:
        resident_position_ = image_size;
        break;

      case SEEK_SET:
        resident_position_ = 0;
        break;

      case SEEK_CUR:
      default:
        break;
    }
    // Clamp rather than fail, so the position always stays inside the image
    // and Read's size arithmetic cannot go negative.
    resident_position_ += offset;
    resident_position_ = resident_position_ < 0 ? 0 : resident_position_;
    resident_position_ =
        resident_position_ > image_size ? image_size : resident_position_;
    return resident_position_;
  }
  return CDFileClass::Seek(offset, origin);
}

int32_t MixAwareFile::Size() {
  // If the file is resident, the size is already known.
  if (IsResident()) {
    return static_cast<int32_t>(resident_data_.Get_Size());
  }

  // If the file is not available as a stand-alone file, then search for it in
  // the mixfiles to get its size without opening it (added August 1996). A file
  // open on a mixfile on disk does not take this path: its handle is open, so
  // the check succeeds and the biased CDFileClass::Size() below reports the
  // embedded length.
  if (!CDFileClass::DoIsAvailable(AvailabilityCheck::kQuick)) {
    if (const auto location = MFCD::Offset(FileName())) {
      return location->size;
    }
    return 0;
  }

  return CDFileClass::Size();
}

bool MixAwareFile::Delete() {
  Close();

  // Only a loose file on disk can be deleted. Without this check the base class
  // would take the mixfile lookup in DoIsAvailable() as proof that the file
  // exists and then try to delete a disk file that is not there.
  if (!CDFileClass::DoIsAvailable(AvailabilityCheck::kQuick)) {
    return false;
  }
  return CDFileClass::Delete();
}

bool MixAwareFile::DoIsAvailable(AvailabilityCheck mode) {
  // A file that is open is presumed available.
  if (IsOpen()) {
    return true;
  }

  // A file that is part of a mixfile is also presumed available. This is
  // checked before the disk because it is a lookup in memory, and it cannot
  // block waiting for media.
  if (MFCD::Offset(FileName()).has_value()) {
    return true;
  }

  // Otherwise a manual check of the file system is required to determine if the
  // file is actually available.
  return CDFileClass::DoIsAvailable(mode);
}

bool MixAwareFile::IsOpen() const {
  // A resident file has no file handle; holding a pointer into the mixfile
  // image is what makes it open. Close() clears that pointer.
  if (IsResident()) {
    return true;
  }

  // Otherwise, go to a lower level to determine if the file is open.
  return CDFileClass::IsOpen();
}

void MixAwareFile::Close() {
  // Reconstructs resident_data_ in place as an empty buffer without destroying
  // the old one. That leaks nothing because Open only ever points it at memory
  // the mixfile cache owns.
  new (&resident_data_)::Buffer;
  resident_position_ = 0;
  CDFileClass::Close();
}

bool MixAwareFile::Open(FileAccess rights) {
  // Always close the file if it was open.
  Close();

  // Perform a preliminary check to see if the specified file exists on the
  // disk. If it does, then open this file regardless of whether it also exists
  // in a mixfile. This is slower, but allows upgrade files to work. Writes
  // always go to disk, since mixfile contents are read-only.
  if (HasAccess(rights, FileAccess::kWrite) ||
      CDFileClass::DoIsAvailable(AvailabilityCheck::kQuick)) {
    return CDFileClass::Open(rights);
  }

  // Check to see if the file is part of a registered mixfile.
  const auto location = MFCD::Offset(FileName());
  if (!location) {
    // The file cannot be found in any mixfile, so it must reside as an
    // individual file on the disk. Or else it is just plain missing, and the
    // disk open reports it.
    return CDFileClass::Open(rights);
  }

  // An empty data span means the mixfile is registered but not cached, so it
  // is still on disk. Fake out the file system to read from the mixfile, but
  // think it is reading from a solitary file.
  if (location->data.empty()) {
    // This is a legitimate open of the mixfile itself. All access through this
    // file object is adjusted for mixfile support, however. Also note that the
    // filename attached to this object is NOT the same as the file attached to
    // the file handle.
    const std::string embedded_name = FileName();
    Open(location->mixfile->Filename().c_str(), FileAccess::kRead);
    // Put the embedded file's name back. Search is disabled so SetName
    // takes the name verbatim instead of probing the search paths for it.
    SetSearchEnabled(false);
    SetName(embedded_name.c_str());
    SetSearchEnabled(true);
    // The bias must be set after SetName, which clears it; Bias() adds start
    // to the existing bias rather than replacing it. offset is absolute within
    // the mixfile here, since the mixfile is not cached.
    Bias(location->offset, location->size);
    Seek(0, SEEK_SET);
  } else {
    // Cached mixfile: point at the file's bytes in the RAM image. The handle
    // stays closed.
    new (&resident_data_)::Buffer(location->data.data(),
                                  base::ToSigned(location->data.size()));
    resident_position_ = 0;
  }

  return true;
}

namespace {

// Files behind the integer-handle file API declared in sdllib/file.h, for code
// outside the game (audio streaming, PCX writing) that cannot use MixAwareFile
// directly. A handle is an index into this table, so at most this many files
// are open through the API at once.
MixAwareFile handle_table[10];

// Returns the open file behind handle, or nullptr if handle is WWERROR, out of
// range, or closed.
MixAwareFile* OpenFileForHandle(int handle) {
  if (handle < 0 || handle >= std::ssize(handle_table) ||
      !handle_table[handle].IsOpen()) {
    return nullptr;
  }
  return &handle_table[handle];
}

}  // namespace

int __cdecl OpenFileHandle(const char* file_name, FileAccess mode) {
  for (int handle = 0; handle < std::ssize(handle_table); handle++) {
    if (!handle_table[handle].IsOpen()) {
      if (handle_table[handle].Open(file_name, mode)) {
        return handle;
      }
      // The first free slot is as good as any other; a failed open would fail
      // in them all.
      break;
    }
  }
  return WWERROR;
}

void __cdecl CloseFileHandle(int handle) {
  if (MixAwareFile* const file = OpenFileForHandle(handle)) {
    file->Close();
  }
}

int32_t __cdecl ReadFileHandle(int handle, void* buffer, int32_t size) {
  if (MixAwareFile* const file = OpenFileForHandle(handle)) {
    return file->Read(buffer, size);
  }
  return 0;
}

int32_t __cdecl WriteFileHandle(int handle, const void* buffer, int32_t size) {
  if (MixAwareFile* const file = OpenFileForHandle(handle)) {
    return file->Write(buffer, size);
  }
  return 0;
}

bool __cdecl FileExists(const char* file_name) {
  MixAwareFile file(file_name);
  return file.IsAvailable();
}

int32_t __cdecl FileHandleSize(int handle) {
  if (MixAwareFile* const file = OpenFileForHandle(handle)) {
    return file->Size();
  }
  return 0;
}

int32_t __cdecl SeekFileHandle(int handle, int32_t offset, int origin) {
  if (MixAwareFile* const file = OpenFileForHandle(handle)) {
    return file->Seek(offset, origin);
  }
  return 0;
}
