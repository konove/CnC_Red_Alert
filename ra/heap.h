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

/* $Header: /CounterStrike/HEAP.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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

#include <cstddef>
#include <vector>

#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "tech/pipe.h"
#include "tech/straw.h"

// Fixed-size block memory allocator that manages a pool of uniformly-sized
// memory blocks.
//
// This class provides efficient allocation and deallocation of fixed-size
// memory blocks, similar to an array but with dynamic allocation tracking. Each
// block is the same size, determined at construction time. The class is
// commonly used to implement custom operator new/delete for game objects,
// enabling fast allocation from a pre-allocated pool.
//
// Key features:
// - Allocates blocks from a contiguous buffer (user-provided or internally
// allocated)
// - Tracks free/allocated blocks using a bitmap (BooleanVectorClass)
// - Supports indexed access to blocks via operator[]
// - Returns nullptr when no blocks are available
// - Provides Count() for active allocations and Avail() for remaining capacity
//
// Thread safety: Not thread-safe. Caller must synchronize access if used
// concurrently.
//
// Example usage:
//   FixedHeapClass heap(sizeof(MyClass));
//   heap.Set_Heap(100);  // Pre-allocate 100 blocks
//   void* ptr = heap.Allocate();  // Get a block
//   heap.Free(ptr);  // Return block to pool
class FixedHeapClass {
 public:
  FixedHeapClass(int size);
  virtual ~FixedHeapClass();

  int Count() const { return ActiveCount; }
  int Length() const { return TotalCount; }
  int Avail() const { return TotalCount - ActiveCount; }

  virtual int ID(void const* pointer) const;
  virtual int Set_Heap(int count, void* buffer = nullptr);
  virtual void* Allocate();
  virtual void Clear();
  virtual int Free(void* pointer);
  virtual int Free_All();

  void* operator[](int index) { return ((char*)Buffer) + (index * Size); }
  void const* operator[](int index) const {
    return ((char*)Buffer) + (index * Size);
  }

 protected:
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
  FixedHeapClass& operator=(FixedHeapClass const&);

  // The copy constructor is not supported.
  FixedHeapClass(FixedHeapClass const&);
};

// Type-safe wrapper around FixedHeapClass that provides automatic type
// conversion. Eliminates the need for manual casting when allocating/freeing
// objects of type T. No runtime overhead - all type conversions happen at
// compile time.
template <class T>
class TFixedHeapClass : public FixedHeapClass {
 public:
  TFixedHeapClass() : FixedHeapClass(sizeof(T)) {}
  ~TFixedHeapClass() override {}

  int ID(T const* pointer) const override {
    return FixedHeapClass::ID(pointer);
  }
  virtual T* Alloc() { return (T*)FixedHeapClass::Allocate(); }
  int Free(T* pointer) override { return (FixedHeapClass::Free(pointer)); }

  T& operator[](int index) { return *(T*)(((char*)Buffer) + (index * Size)); }
  T const& operator[](int index) const {
    return *(T*)(((char*)Buffer) + (index * Size));
  }
};

// Fixed-size block allocator with fast iteration over active (allocated)
// objects. Extends FixedHeapClass by maintaining an array of pointers to all
// allocated blocks, enabling efficient iteration without scanning the entire
// pool for active objects. Memory overhead: 4-8 bytes per potential block
// (pointer size) in ActivePointers vector.
class FixedIHeapClass : public FixedHeapClass {
 public:
  FixedIHeapClass(int size) : FixedHeapClass(size) {}
  ~FixedIHeapClass() override {}

  int Set_Heap(int count, void* buffer = nullptr) override;
  void* Allocate() override;
  void Clear() override;
  int Free(void* pointer) override;
  int Free_All() override;
  virtual int Logical_ID(void const* pointer) const;
  virtual int Logical_ID(int id) const { return (Logical_ID((*this)[id])); }

  virtual void* Active_Ptr(int index) { return ActivePointers[index]; }
  virtual void const* Active_Ptr(int index) const {
    return ActivePointers[index];
  }

  /*
  **	This is an array of pointers to allocated objects. Using this array
  **	to control iteration through the objects ensures a minimum of
  *processing. *	It also allows access to this array so that custom
  *sorting can be *	performed.
  */
  DynamicVectorClass<void*> ActivePointers;
};

// Type-safe wrapper around FixedIHeapClass with automatic type conversion.
// Provides type-safe access to iterable heap functionality plus serialization
// support. All type conversions are compile-time with zero runtime overhead.
template <class T>
class TFixedIHeapClass : public FixedIHeapClass {
 public:
  TFixedIHeapClass() : FixedIHeapClass(sizeof(T)) {}
  ~TFixedIHeapClass() override {}

  virtual int ID(T const* pointer) const {
    return FixedIHeapClass::ID(pointer);
  }
  virtual int Logical_ID(T const* pointer) const {
    return (FixedIHeapClass::Logical_ID(pointer));
  }
  int Logical_ID(int id) const override {
    return (FixedIHeapClass::Logical_ID(id));
  }
  virtual T* Alloc() { return (T*)FixedIHeapClass::Allocate(); }
  virtual int Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  int Free(void* pointer) override { return FixedIHeapClass::Free(pointer); }
  virtual int Save(Pipe& file) const;
  virtual int Load(Straw& file);
  virtual void Code_Pointers();
  virtual void Decode_Pointers();

  virtual T* Ptr(std::size_t index) const { return (T*)ActivePointers[index]; }
  virtual T* Raw_Ptr(std::size_t index) { return (T*)((*this)[index]); }
};

#endif
