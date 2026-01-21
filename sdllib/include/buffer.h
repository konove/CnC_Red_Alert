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

#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>

class GraphicViewPortClass;

// BufferClass - A base class which holds buffer information including a pointer
// and the size of the buffer.
class BufferClass {
 public:
  // Define the base constructor and destructors for the class
  BufferClass() : Buffer(nullptr), Size(0), Allocated(false) {}
  BufferClass(long size)
      : Buffer(new uint8_t[size]), Size(size), Allocated(true) {}
  ~BufferClass() {
    if (Allocated) {
      delete[] (uint8_t*)Buffer;
    }
  }

  // Define functions which work with the buffer class.
  long To_Page(GraphicViewPortClass& view);
  long To_Page(int w, int h, GraphicViewPortClass& view);
  long To_Page(int x, int y, int w, int h, GraphicViewPortClass& view);

  // define functions to get at the protected data members
  void* Get_Buffer() { return Buffer; }
  long Get_Size() { return Size; }

 protected:
  void* Buffer;
  long Size;
  bool Allocated;

 private:
  // Define the operators we do not want to happen which are the copy, move,
  // and assignment operators. These are bad because the Allocated flag could
  // be copied and the associated buffer freed. If this were to happen it could
  // cause weird general protection faults.
  BufferClass(const BufferClass&) = delete;
  BufferClass& operator=(const BufferClass&) = delete;
  BufferClass(BufferClass&&) = delete;
  BufferClass& operator=(BufferClass&&) = delete;
};

#endif
