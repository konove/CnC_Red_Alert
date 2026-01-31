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

/* $Header:   F:\projects\c&c\vcs\code\heap.h_v   2.15   16 Oct 1995 16:47:08
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : HEAP.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 02/18/95 *
 *                                                                                             *
 *                  Last Update : February 18, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef HEAP_H
#define HEAP_H

#include <vector>

#include "td/vector.h"
#include "tech/wwfile.h"

// Heap templates are generic - users must include type headers themselves.
// IWYU pragma: no_include "td/aircraft.h"
// IWYU pragma: no_include "td/anim.h"
// IWYU pragma: no_include "td/building.h"
// IWYU pragma: no_include "td/bullet.h"
// IWYU pragma: no_include "td/factory.h"
// IWYU pragma: no_include "td/house.h"
// IWYU pragma: no_include "td/infantry.h"
// IWYU pragma: no_include "td/overlay.h"
// IWYU pragma: no_include "td/smudge.h"
// IWYU pragma: no_include "td/team.h"
// IWYU pragma: no_include "td/teamtype.h"
// IWYU pragma: no_include "td/template.h"
// IWYU pragma: no_include "td/terrain.h"
// IWYU pragma: no_include "td/trigger.h"
// IWYU pragma: no_include "td/unit.h"

// Fixed-size block allocator. Manages a pool of same-sized memory blocks
// without type information, making it suitable for overloading new/delete.
class FixedHeapClass {
 public:
  FixedHeapClass(int size);
  virtual ~FixedHeapClass();

  virtual int ID(const void* pointer);
  int Count() { return ActiveCount; }
  int Length() { return TotalCount; }
  int Avail() { return TotalCount - ActiveCount; }

  virtual int Set_Heap(int count, void* buffer = nullptr);
  virtual void* Allocate();
  virtual void Clear();
  virtual int Free(void* pointer);
  virtual int Free_All();

 protected:
  void* operator[](int index) {
    return static_cast<char*>(Buffer) + index * Size;
  }

  /*
  **	If the memory block buffer was allocated by this class, then this flag
  **	will be true. The block must be deallocated by this class if true.
  */
  unsigned IsAllocated : 1;

  /*
  **	This is the size of each sub-block within the buffer.
  */
  int Size;

  /*
  **	This records the absolute number of sub-blocks in the buffer.
  */
  int TotalCount;

  /*
  **	This is the total blocks allocated out of the heap. This number
  **	will never exceed Count.
  */
  int ActiveCount;

  /*
  **	Pointer to the heap's memory buffer.
  */
  void* Buffer;

  /*
  **	This is a boolean vector array of allocation flag bits.
  */
  std::vector<bool> FreeFlag;

 private:
  // The assignment operator is not supported.
  FixedHeapClass& operator=(const FixedHeapClass&);

  // The copy constructor is not supported.
  FixedHeapClass(const FixedHeapClass&);
};

/**************************************************************************
**	This template serves only as an interface to the heap manager class. By
**	using this template, the object pointers are automatically converted
**	to the correct type without any code overhead.
*/
template <class T>
class TFixedHeapClass : public FixedHeapClass {
 public:
  TFixedHeapClass() : FixedHeapClass(sizeof(T)) {}
  ~TFixedHeapClass() override = default;

  virtual int ID(const T* pointer) { return FixedHeapClass::ID(pointer); }

  virtual T* Alloc() { return static_cast<T*>(FixedHeapClass::Allocate()); }
  virtual int Free(T* pointer) { return FixedHeapClass::Free(pointer); }

 protected:
  T& operator[](int index) {
    return *(static_cast<char*>(Buffer) + index * Size);
  }
};

/**************************************************************************
**	This is a derivative of the fixed heap class. This class adds the
**	ability to quickly iterate through the active (allocated) objects. Since
*the *	active array is a sequence of pointers, the overhead of this class
**	is 4 bytes per potential allocated object (be warned).
*/
class FixedIHeapClass : public FixedHeapClass {
 public:
  FixedIHeapClass(int size) : FixedHeapClass(size) {}
  ~FixedIHeapClass() override = default;

  int Set_Heap(int count, void* buffer = nullptr) override;
  void* Allocate() override;
  void Clear() override;
  int Free(void* pointer) override;
  int Free_All() override;

  virtual void* Active_Ptr(int index) { return ActivePointers[index]; }

  /*
  **	This is an array of pointers to allocated objects. Using this array
  **	to control iteration through the objects ensures a minimum of
  *processing. *	It also allows access to this array so that custom
  *sorting can be *	performed.
  */
  DynamicVectorClass<void*> ActivePointers;
};

/**************************************************************************
**	This template serves only as an interface to the iteratable heap manager
**	class. By using this template, the object pointers are automatically
*converted *	to the correct type without any code overhead.
*/
template <class T>
class TFixedIHeapClass : public FixedIHeapClass {
 public:
  TFixedIHeapClass() : FixedIHeapClass(sizeof(T)) {}
  ~TFixedIHeapClass() override = default;

  virtual int ID(const T* pointer) { return FixedIHeapClass::ID(pointer); }
  virtual T* Alloc() { return static_cast<T*>(FixedIHeapClass::Allocate()); }
  virtual int Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  int Free(void* pointer) override { return FixedIHeapClass::Free(pointer); }
  virtual int Save(FileClass&);
  virtual int Load(FileClass&);
  virtual void Code_Pointers();
  virtual void Decode_Pointers();

  virtual T* Ptr(int index) { return static_cast<T*>(ActivePointers[index]); }
  virtual T* Raw_Ptr(int index) { return static_cast<T*>((*this)[index]); }
};

#endif
