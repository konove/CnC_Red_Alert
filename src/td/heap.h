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

#ifndef CNC_RED_ALERT_TD_HEAP_H_
#define CNC_RED_ALERT_TD_HEAP_H_

#include <concepts>
#include <new>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "base/types.h"
#include "td/vector.h"
#include "tech/archive.h"

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
  explicit FixedHeapClass(int size) noexcept;
  virtual ~FixedHeapClass();
  FixedHeapClass(FixedHeapClass&&) = delete;
  FixedHeapClass& operator=(FixedHeapClass&&) = delete;

  virtual int ID(const void* pointer);
  [[nodiscard]] int Count() const { return ActiveCount; }
  [[nodiscard]] int Length() const { return TotalCount; }
  [[nodiscard]] int Avail() const { return TotalCount - ActiveCount; }
  [[nodiscard]] bool Is_Allocated(int index) const {
    return index >= 0 && index < TotalCount && FreeFlag[base::ToSize(index)];
  }

  virtual bool Set_Heap(int count, std::span<char> buffer = {});
  virtual void* Allocate();
  virtual void Clear();
  virtual bool Free(void* pointer);
  virtual bool Free_All();

 protected:
  void* operator[](int index) {
    CHECK_GE(index, 0);
    CHECK_LT(index, TotalCount);
    return Buffer.subspan(base::ToSize(int64_t{index} * Size)).data();
  }

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

/**************************************************************************
**	This is a derivative of the fixed heap class. This class adds the
**	ability to quickly iterate through the active (allocated) objects. Since
*the *	active array is a sequence of pointers, the overhead of this class
**	is 4 bytes per potential allocated object (be warned).
*/
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
  TFixedIHeapClass() noexcept : FixedIHeapClass(sizeof(T)) {}
  ~TFixedIHeapClass() override = default;
  TFixedIHeapClass(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass& operator=(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass(TFixedIHeapClass&&) = delete;
  TFixedIHeapClass& operator=(TFixedIHeapClass&&) = delete;

  using FixedIHeapClass::ID;

  virtual int ID(const T* pointer) { return FixedIHeapClass::ID(pointer); }
  virtual T* Alloc() { return static_cast<T*>(FixedIHeapClass::Allocate()); }
  virtual bool Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  bool Free(void* pointer) override { return FixedIHeapClass::Free(pointer); }
  bool Save(ArchiveWriter& /*file*/)
    requires Serializable<T>;
  bool Load(ArchiveReader& /*file*/)
    requires Serializable<T>;

  virtual T* Ptr(int index) { return static_cast<T*>(ActivePointers[index]); }
  virtual T* Raw_Ptr(int index) { return static_cast<T*>((*this)[index]); }
};

/***********************************************************************************************
 * TFixedIHeapClass::Save -- Saves all active objects *
 *                                                                                             *
 * INPUT:   file      file to write to *
 *                                                                                             *
 * OUTPUT:  true = OK, false = error *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/15/1995 BRR : Created. *
 *=============================================================================================*/
template <class T>
bool TFixedIHeapClass<T>::Save(ArchiveWriter& file)
  requires Serializable<T>
{

  /*
  ** Save the number of instances of this class
  */
  int32_t count = ActiveCount;
  file(count);

  /*
  ** Save each instance of this class
  */
  for (int i = 0; i < ActiveCount; i++) {
    /*
    ** Save the array index of the object, so it can be loaded back into the
    ** same array location (so TARGET translations will work)
    */
    int32_t idx = ID(Ptr(i));  // object index
    file(idx);

    /*
    ** Save the object itself
    */
    Ptr(i)->Serialize(file);
  }

  return true;
}

/***********************************************************************************************
 * TFixedIHeapClass::Load -- Loads all active objects *
 *                                                                                             *
 * INPUT:   file      file to read from *
 *                                                                                             *
 * OUTPUT:  true = OK, false = error *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/15/1995 BRR : Created. *
 *=============================================================================================*/
template <class T>
bool TFixedIHeapClass<T>::Load(ArchiveReader& file)
  requires Serializable<T>
{
  int32_t idx = 0;  // object index
  int32_t a_count = 0;

  /*
  ** Read the number of instances of this class
  */
  file(a_count);
  if (!file.ok()) {
    return false;
  }

  /*
  ** Error if more objects than we can hold
  */
  if (a_count < 0 || a_count > TotalCount) {
    file.Fail("invalid saved heap count");
    return false;
  }

  /*
  ** Read each class instance
  */
  for (int i = 0; i < a_count; i++) {
    /*
    ** Read the object's array index
    */
    file(idx);
    if (!file.ok()) {
      return false;
    }

    /*
    ** Get a pointer to the object, activate that object
    */
    if (idx < 0 || idx >= TotalCount || FreeFlag[base::ToSize(idx)]) {
      file.Fail("invalid heap slot");
      return false;
    }
    T* ptr = static_cast<T*>((*this)[idx]);  // object pointer
    FreeFlag[base::ToSize(idx)] = true;
    ActiveCount++;
    ActivePointers.Add(ptr);

    /*
    ** Load the object
    */
    new (ptr) T();
    ptr->Serialize(file);
    if (!file.ok()) {
      return false;
    }
  }

  return file.ok();
}

#endif  // CNC_RED_ALERT_TD_HEAP_H_
