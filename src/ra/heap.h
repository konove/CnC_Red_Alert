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

#ifndef CNC_RED_ALERT_RA_HEAP_H_
#define CNC_RED_ALERT_RA_HEAP_H_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "tech/archive.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"

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
  explicit FixedHeapClass(int size) noexcept;
  virtual ~FixedHeapClass();
  FixedHeapClass(FixedHeapClass&&) = delete;
  FixedHeapClass& operator=(FixedHeapClass&&) = delete;

  [[nodiscard]] int Count() const { return ActiveCount; }
  [[nodiscard]] int Length() const { return TotalCount; }
  [[nodiscard]] int Avail() const { return TotalCount - ActiveCount; }

  virtual int ID(const void* pointer) const;
  virtual bool Set_Heap(int count, void* buffer = nullptr);
  virtual void* Allocate();
  virtual void Clear();
  virtual bool Free(void* pointer);
  virtual bool Free_All();

  void* operator[](int index) {
    return static_cast<char*>(Buffer) +
           (static_cast<base::ssize>(index) * Size);
  }
  const void* operator[](int index) const {
    return static_cast<char*>(Buffer) +
           (static_cast<base::ssize>(index) * Size);
  }

 protected:
  /*
  **	If the memory block buffer was allocated by this class, then this flag
  **	will be true. The block must be deallocated by this class if true.
  */
  bool IsAllocated : 1 {false};

  /*
  **	This is the size of each sub-block within the buffer.
  */
  int Size;

  /*
  **	This records the absolute number of sub-blocks in the buffer.
  */
  int TotalCount{0};

  /*
  **	This is the total blocks allocated out of the heap. This number
  **	will never exceed Count.
  */
  int ActiveCount{0};

  /*
  **	Pointer to the heap's memory buffer.
  */
  void* Buffer{nullptr};

  /*
  **	This is a boolean vector array of allocation flag bits.
  */
  std::vector<bool> FreeFlag;

 public:
  // The assignment operator is not supported.
  FixedHeapClass& operator=(const FixedHeapClass&) = delete;

  // The copy constructor is not supported.
  FixedHeapClass(const FixedHeapClass&) = delete;
};

// Fixed-size block allocator with fast iteration over active (allocated)
// objects. Extends FixedHeapClass by maintaining an array of pointers to all
// allocated blocks, enabling efficient iteration without scanning the entire
// pool for active objects. Memory overhead: 4-8 bytes per potential block
// (pointer size) in ActivePointers vector.
class FixedIHeapClass : public FixedHeapClass {
 public:
  explicit FixedIHeapClass(int size) noexcept : FixedHeapClass(size) {}
  ~FixedIHeapClass() override = default;
  FixedIHeapClass(const FixedIHeapClass&) = delete;
  FixedIHeapClass& operator=(const FixedIHeapClass&) = delete;
  FixedIHeapClass(FixedIHeapClass&&) = delete;
  FixedIHeapClass& operator=(FixedIHeapClass&&) = delete;

  bool Set_Heap(int count, void* buffer = nullptr) override;
  void* Allocate() override;
  void Clear() override;
  bool Free(void* pointer) override;
  bool Free_All() override;
  virtual int Logical_ID(const void* pointer) const;
  [[nodiscard]] virtual int Logical_ID(int id) const {
    return Logical_ID((*this)[id]);
  }

  virtual void* Active_Ptr(int index) { return ActivePointers[index]; }
  [[nodiscard]] virtual const void* Active_Ptr(int index) const {
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
//
// Save and Load exist only for element types with field-wise Serialize().
// A Serializable T needs a default constructor reachable from this class
// (declare `friend class TFixedIHeapClass<T>;`) that fully initializes the
// object without side effects; Load placement-news it into the slot before
// reading the fields.
template <class T>
class TFixedIHeapClass : public FixedIHeapClass {
 public:
  TFixedIHeapClass() noexcept : FixedIHeapClass(sizeof(T)) {}
  ~TFixedIHeapClass() override = default;
  TFixedIHeapClass(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass& operator=(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass(TFixedIHeapClass&&) = delete;
  TFixedIHeapClass& operator=(TFixedIHeapClass&&) = delete;

  using FixedIHeapClass::ID;
  using FixedIHeapClass::Logical_ID;

  virtual int ID(const T* pointer) const {
    return FixedIHeapClass::ID(pointer);
  }
  virtual int Logical_ID(const T* pointer) const {
    return FixedIHeapClass::Logical_ID(pointer);
  }
  [[nodiscard]] int Logical_ID(int id) const override {
    return FixedIHeapClass::Logical_ID(id);
  }
  virtual T* Alloc() { return static_cast<T*>(FixedIHeapClass::Allocate()); }
  virtual bool Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  bool Free(void* pointer) override { return FixedIHeapClass::Free(pointer); }
  // Writes the active count, then each object's slot index and contents.
  bool Save(ByteSink& file) const
    requires Serializable<T>;
  // Reads what Save wrote back into the same slots. Returns false on a
  // malformed stream; the heap is then partially populated.
  bool Load(ByteSource& file)
    requires Serializable<T>;
  [[nodiscard]] virtual T* Ptr(int index) const {
    return static_cast<T*>(ActivePointers[index]);
  }
  virtual T* Raw_Ptr(int index) {
    return static_cast<T*>((*this)[index]);
  }
};

template <class T>
bool TFixedIHeapClass<T>::Save(ByteSink& file) const
  requires Serializable<T>
{
  ArchiveWriter writer(file);
  int32_t count = ActiveCount;
  writer(count);

  for (int i = 0; i < ActiveCount; i++) {
    // The slot index goes first so the object lands in the same slot on
    // load, which keeps TARGET values valid across the round trip.
    int32_t idx = ID(Ptr(i));
    writer(idx);
    Ptr(i)->Serialize(writer);
  }
  return true;
}

template <class T>
bool TFixedIHeapClass<T>::Load(ByteSource& file)
  requires Serializable<T>
{
  ArchiveReader reader(file);
  int32_t count = 0;
  reader(count);
  if (!reader.ok() || count < 0 || count > TotalCount) {
    return false;
  }

  for (int i = 0; i < count; i++) {
    int32_t idx = 0;
    reader(idx);
    if (!reader.ok() || idx < 0 || idx >= TotalCount) {
      return false;
    }

    T* ptr = static_cast<T*>((*this)[idx]);
    FreeFlag[base::ToSize(idx)] = true;
    ActiveCount++;
    ActivePointers.Add(ptr);

    new (ptr) T();
    ptr->Serialize(reader);
    if (!reader.ok()) {
      return false;
    }
  }
  return reader.ok();
}

#endif  // CNC_RED_ALERT_RA_HEAP_H_
