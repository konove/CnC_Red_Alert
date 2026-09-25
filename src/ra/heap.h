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
#include <functional>
#include <new>
#include <span>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"
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
  virtual bool Set_Heap(int count, std::span<char> buffer = {});
  virtual void* Allocate();
  virtual void Clear();
  virtual bool Free(void* pointer);
  virtual bool Free_All();

  // How far into the buffer `pointer` lies, or -1 when it lies outside it.
  [[nodiscard]] base::ssize Offset_Of(const void* pointer) const;

  // Whether `pointer` is the start of one of this heap's slots. ID() cannot
  // answer that: it subtracts and divides whatever it is handed, so a pointer
  // from somewhere else still comes back as a plausible index.
  [[nodiscard]] bool Owns(const void* pointer) const;

  // Everything known about the slot `pointer` claims to be, for the DCHECKs
  // that guard heap objects and for Validate(). The four ways a pointer can
  // be wrong look different here: outside the buffer, inside but not on a
  // slot boundary, on a boundary whose free flag is clear (use after free),
  // or sound, which leaves the object's own ID field as the corrupted one.
  [[nodiscard]] virtual std::string Describe(const void* pointer) const;

  // Returns a storage slot; index must be in [0, Length()).
  void* at(int index) {
    CHECK_GE(index, 0);
    CHECK_LT(index, TotalCount);
    return Buffer.subspan(base::ToSize(int64_t{index} * Size)).data();
  }
  void* operator[](int index) { return at(index); }
  [[nodiscard]] const void* at(int index) const {
    CHECK_GE(index, 0);
    CHECK_LT(index, TotalCount);
    return Buffer.subspan(base::ToSize(int64_t{index} * Size)).data();
  }
  const void* operator[](int index) const { return at(index); }

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
  std::span<char> Buffer;

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

  bool Set_Heap(int count, std::span<char> buffer = {}) override;
  void* Allocate() override;
  void Clear() override;
  bool Free(void* pointer) override;
  bool Free_All() override;
  virtual int Logical_ID(const void* pointer) const;
  [[nodiscard]] virtual int Logical_ID(int id) const {
    return Logical_ID((*this).at(id));
  }

  // Adds whether ActivePointers lists the slot, which is what tells a live
  // object apart from one the layer lists but the heap has let go.
  [[nodiscard]] std::string Describe(const void* pointer) const override;

  // Describes the first disagreement between ActivePointers, FreeFlag and the
  // slot buffer, or "" when the heap is sound. Walks every active pointer, so
  // it is for the once-a-frame -CHECKHEAPS sweep, not for the inner loop.
  [[nodiscard]] virtual std::string Validate() const;

  virtual void* Active_Ptr(int index) { return ActivePointers.at(index); }
  [[nodiscard]] virtual const void* Active_Ptr(int index) const {
    return ActivePointers.at(index);
  }

  /*
  **	This is an array of pointers to allocated objects. Using this array
  **	to control iteration through the objects ensures a minimum of
  *processing. *	It also allows access to this array so that custom
  *sorting can be *	performed.
  */
  DynamicVectorClass<void*> ActivePointers;
};

// Whether T caches its own slot index in a public `ID` field, the way
// everything descended from AbstractClass does. Spelled as a requirement
// rather than `std::derived_from<AbstractClass>` so that heap.h stays below
// the game's object hierarchy.
template <class T>
concept HasHeapId = requires(const T& object) {
  { object.ID } -> std::convertible_to<int>;
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
  // Adds the check the untyped heap cannot make: that each object's cached ID
  // still names the slot it occupies.
  [[nodiscard]] std::string Validate() const override {
    std::string trouble = FixedIHeapClass::Validate();
    if constexpr (HasHeapId<T>) {
      for (int i = 0; trouble.empty() && i < ActiveCount; i++) {
        const T* object = Ptr(i);
        if (FixedIHeapClass::ID(object) != int{object->ID}) {
          trouble = absl::StrCat("stale ID ", int{object->ID}, ": ",
                                 Describe(object));
        }
      }
    }
    return trouble;
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
    return static_cast<T*>(ActivePointers.at(index));
  }
  virtual T* Raw_Ptr(int index) { return static_cast<T*>((*this).at(index)); }
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

    T* ptr = static_cast<T*>((*this).at(idx));
    FreeFlag.at(base::ToSize(idx)) = true;
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

// Verifies that `object` still sits in the heap slot its own ID field names,
// and dumps the slot's state when it does not. This is the invariant every
// heap object's methods assume, so the check guards most of them; it is a
// macro because only a macro can stream context into a DCHECK and still
// compile away in a release build.
//
// Example:
//   DCHECK_HEAP_SLOT(TheObjectHeaps().infantry(), this);
#define DCHECK_HEAP_SLOT(heap, object) \
  DCHECK_EQ((heap).ID(object), int{(object)->ID}) << (heap).Describe(object)

// CHECK_HEAP_SLOT is the same test kept in a release build, for the heaps
// whose corruption the game cannot survive.
#define CHECK_HEAP_SLOT(heap, object) \
  CHECK_EQ((heap).ID(object), int{(object)->ID}) << (heap).Describe(object)

#endif  // CNC_RED_ALERT_RA_HEAP_H_
