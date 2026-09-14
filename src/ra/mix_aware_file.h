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

#ifndef CNC_RED_ALERT_RA_MIX_AWARE_FILE_H_
#define CNC_RED_ALERT_RA_MIX_AWARE_FILE_H_

// File: MixAwareFile, the game's file object that can read files packed inside
// mixfiles as if they were loose files on disk. mix_aware_file.cc also defines
// the integer-handle file API declared in sdllib/file.h (OpenFileHandle,
// ReadFileHandle, ...) on top of it, for the audio and image code outside the
// game.
//
// Originally CCFILE.H (class CCFileClass) by Joe L. Bostic, started October 17,
// 1994.

#include <cstdint>
#include <cstdio>

#include "tech/buff.h"
#include "tech/cdfile.h"
#include "tech/wwfile.h"

// A file object that knows about mixfiles (packed archives). Opening a name
// that is packed inside a registered mixfile works whether that mixfile is
// cached in RAM or still on disk; the caller sees an ordinary file either way.
// It is functionally similar to pakfiles, but much faster and less RAM
// intensive.
//
// A loose file on disk with the same name wins over the mixfile copy, which is
// how patch files override packed data.
//
// Example:
//   MixAwareFile file("RULES.INI");
//   if (file.Is_Available()) {
//     file.Open();
//     const int32_t size = file.Size();
//     file.Read(buffer, size);
//   }
class MixAwareFile : public CDFileClass {
 public:
  // Constructs a file object bound to filename. The name is resolved against
  // the CD search paths immediately.
  explicit MixAwareFile(const char* filename);
  MixAwareFile();

  MixAwareFile(const MixAwareFile&) = delete;
  MixAwareFile& operator=(const MixAwareFile&) = delete;
  MixAwareFile(MixAwareFile&&) = delete;
  MixAwareFile& operator=(MixAwareFile&&) = delete;

  ~MixAwareFile() override = default;

  // Returns true if the file is open on the RAM image of a cached mixfile, in
  // which case reads and seeks never touch the file handle.
  [[nodiscard]] bool IsResident() const {
    return resident_data_.Get_Buffer() != nullptr;
  }

  // Returns true if the file is open, either on a cached mixfile image or
  // through a valid file handle.
  [[nodiscard]] bool IsOpen() const override;

  // Assigns filename to the file object and opens it; see Open(FileAccess).
  bool Open(const char* filename,
            FileAccess rights = FileAccess::kRead) override {
    Set_Name(filename);
    return Open(rights);
  }

  // Opens the file, closing it first if it was open. A write, or a name found
  // as a loose file on disk, opens the disk file. Otherwise, if the name is in
  // a registered mixfile, the file is opened on the cached RAM image or on the
  // byte range inside the mixfile on disk. A name found nowhere falls through
  // to a normal disk open. Returns whether the file was opened.
  bool Open(FileAccess rights = FileAccess::kRead) override;

  // Reads up to size bytes into buffer and returns the number actually read,
  // which is less than size at end of file. A file that is not open is opened
  // for the read and closed again afterwards.
  int32_t Read(void* buffer, int32_t size) override;

  // Moves the file position by offset relative to origin (SEEK_SET, SEEK_CUR or
  // SEEK_END) and returns the new position. For a resident file the position is
  // clamped to [0, Size()].
  int32_t Seek(int32_t offset, int origin = SEEK_CUR) override;

  // Returns the size of the file in bytes. For a file packed in a mixfile this
  // is the size of the embedded file, not the mixfile, even when the file is
  // not open. Returns 0 for a file that is not found anywhere.
  int32_t Size() override;

  // Writes size bytes from buffer and returns the number written. Files packed
  // in a cached mixfile are read-only; writing one writes nothing and returns
  // 0.
  int32_t Write(const void* buffer, int32_t size) override;

  // Closes the file and resets the position to the start.
  void Close() override;

  // Closes the file and deletes it from disk. Returns false, deleting nothing,
  // if there is no loose file by this name; a file packed in a mixfile cannot
  // be deleted.
  bool Delete() override;

  // Handles a file error. All three arguments are ignored: the only recovery
  // attempted is making sure the scenario's required CD (RequiredCD) is in a
  // drive, prompting the player for it. If the player cancels, the game exits
  // and this never returns; otherwise it returns, even when can_retry is false.
  void Error(int error, bool can_retry = false,
             const char* filename = nullptr) override;

 protected:
  // Returns true if the file is open, is packed in a registered mixfile, or is
  // found on disk. mode is passed on to the disk check.
  bool DoIsAvailable(AvailabilityCheck mode) override;

 private:
  // A view of the file's bytes inside the RAM image of a cached mixfile, or an
  // empty buffer if the file is not resident. The buffer never owns the memory;
  // it belongs to the mixfile cache. While it is set, the inherited file handle
  // is invalid and all access is routed through this image, whose size stands
  // in for the file length.
  ::Buffer resident_data_;

  // Current read position within resident_data_, from zero to the size of the
  // file in bytes. Tracked here because a resident file has no handle to hold
  // one.
  int32_t resident_position_ = 0;
};

#endif  // CNC_RED_ALERT_RA_MIX_AWARE_FILE_H_
