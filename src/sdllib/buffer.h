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

#ifndef CNC_RED_ALERT_SDLLIB_BUFFER_H_
#define CNC_RED_ALERT_SDLLIB_BUFFER_H_

#include <cstdint>
#include <span>

#include "base/numeric.h"

class PixelView;

// BufferClass - A base class which holds buffer information including a pointer
// and the size of the buffer.
class BufferClass {
 public:
  // Define the base constructor and destructors for the class
  BufferClass() : Buffer(nullptr), Size(0), Allocated(false) {}
  explicit BufferClass(int32_t size)
      : Buffer(new uint8_t[base::ToSize(size)]), Size(size), Allocated(true) {
    // Buffer was allocated immediately above with exactly Size elements.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    bytes_ = std::span(static_cast<uint8_t*>(Buffer), base::ToSize(Size));
  }
  ~BufferClass() {
    if (Allocated) {
      delete[] static_cast<uint8_t*>(Buffer);
    }
  }

  // Copies the buffer's bytes into `view` as w x h pixels with their top
  // left corner at x,y in the view, and returns the number of bytes
  // read. The overloads default x,y to the view's corner and w,h to its
  // size. Defined in graphic_buffer.h, which has the complete PixelView type.
  int32_t To_Page(PixelView& view);
  int32_t To_Page(int w, int h, PixelView& view);
  int32_t To_Page(int x, int y, int w, int h, PixelView& view);

  // define functions to get at the protected data members
  void* Get_Buffer() { return Buffer; }
  [[nodiscard]] int32_t Get_Size() const { return Size; }
  // Returns the complete owned or caller-supplied allocation.
  [[nodiscard]] std::span<uint8_t> Get_Bytes() { return bytes_; }

 protected:
  std::span<uint8_t> bytes_;
  void* Buffer;
  int32_t Size;
  bool Allocated;

 private:
 public:
  // Define the operators we do not want to happen which are the copy, move,
  // and assignment operators. These are bad because the Allocated flag could
  // be copied and the associated buffer freed. If this were to happen it could
  // cause weird general protection faults.
  BufferClass(const BufferClass&) = delete;
  BufferClass& operator=(const BufferClass&) = delete;
  BufferClass(BufferClass&&) = delete;
  BufferClass& operator=(BufferClass&&) = delete;
};

#endif  // CNC_RED_ALERT_SDLLIB_BUFFER_H_
