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

/* $Header: /CounterStrike/WWFILE.H 1     3/03/97 10:26a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_TECH_FILE_H_
#define CNC_RED_ALERT_TECH_FILE_H_

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"

// File: the interface every file object in the game implements. Concrete
// files live on disk (DiskFile), in memory (MemoryFile) or inside the game's
// mixfile archives (GameFile); code that reads or writes takes a File&.
//
// Originally WWFILE.H (class FileClass) by Joe L. Bostic, August 8, 1994.
class File {
 public:
  File() = default;
  virtual ~File() = default;

  File(const File&) = delete;
  File& operator=(const File&) = delete;
  File(File&&) = delete;
  File& operator=(File&&) = delete;

  // Returns the name bound to the file object, which is empty if none has
  // been assigned. The view is invalidated by the next SetName() or Open()
  // with a name.
  [[nodiscard]] virtual std::string_view FileName() const = 0;

  // Binds filename to the file object without opening it. Derived classes may
  // store a resolved form of the name (such as one with a search path
  // prepended); FileName() returns what was stored. The name is copied.
  virtual void SetName(std::string_view filename) = 0;

  virtual bool Create() = 0;
  virtual bool Delete() = 0;

  // Returns true if the file is available to be opened. Never blocks waiting
  // for media.
  virtual bool IsAvailable() = 0;

  [[nodiscard]] virtual bool IsOpen() const = 0;
  virtual bool Open(std::string_view filename,
                    FileAccess rights = FileAccess::kRead) = 0;
  virtual bool Open(FileAccess rights = FileAccess::kRead) = 0;

  // Reads up to buffer.size() bytes into buffer and returns the number read,
  // which is less than the buffer size only at the end of the file.
  virtual base::ssize Read(std::span<std::byte> buffer) = 0;

  // Writes buffer to the file and returns the number of bytes written.
  virtual base::ssize Write(std::span<const std::byte> buffer) = 0;

  // Reads one trivially copyable value. Returns false on a short read, in
  // which case value is partially written.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool ReadObject(T& value) {
    return Read(std::as_writable_bytes(std::span(&value, 1))) ==
           base::ToSigned(sizeof(T));
  }

  // Writes one trivially copyable value. Returns false on a short write.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool WriteObject(const T& value) {
    return Write(std::as_bytes(std::span(&value, 1))) ==
           base::ToSigned(sizeof(T));
  }

  // Reads up to count bytes; the result is shorter at the end of the file.
  std::vector<std::byte> ReadBytes(base::ssize count);

  // Reads up to count bytes as text; the result is shorter at the end of the
  // file.
  std::string ReadString(base::ssize count);

  // Typed spans of trivially copyable elements. The count returned is still
  // in bytes. A derived class that overrides the std::byte forms needs
  // "using File::Read;" and "using File::Write;" to keep these visible.
  template <typename T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> &&
             !std::is_same_v<std::remove_cv_t<T>, std::byte>)
  base::ssize Read(std::span<T, N> buffer) {
    return Read(std::as_writable_bytes(buffer));
  }
  template <typename T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> &&
             !std::is_same_v<std::remove_cv_t<T>, std::byte>)
  base::ssize Write(std::span<T, N> buffer) {
    return Write(std::as_bytes(buffer));
  }

  // A character buffer and a byte count, for the many callers that read text
  // or raw bytes into a char array. Only byte-sized element types are
  // accepted, so the count cannot be misread as elements; use ReadObject()
  // or a span for anything else.
  template <typename T>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  base::ssize Read(T* buffer, base::ssize count) {
    return Read(std::span(buffer, base::ToSize(count)));
  }
  template <typename T>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  base::ssize Write(const T* buffer, base::ssize count) {
    return Write(std::span(buffer, base::ToSize(count)));
  }

  // Moves the file position by offset from origin and returns the new
  // position, measured from the start of the file.
  virtual base::ssize Seek(base::ssize offset,
                           SeekOrigin origin = SeekOrigin::kCurrent) = 0;

  // Returns the size of the file in bytes.
  virtual base::ssize Size() = 0;

  virtual void Close() = 0;
};

#endif  // CNC_RED_ALERT_TECH_FILE_H_
