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
#include <vector>

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
  int Count() { return ActiveCount; }
  int Length() { return TotalCount; }
  int Avail() { return TotalCount - ActiveCount; }
  [[nodiscard]] bool Is_Allocated(int index) const {
    return index >= 0 && index < TotalCount && FreeFlag[index];
  }

  virtual int Set_Heap(int count, void* buffer = nullptr);
  virtual void* Allocate();
  virtual void Clear();
  virtual int Free(void* pointer);
  virtual int Free_All();

 protected:
  void* operator[](int index) {
    return static_cast<char*>(Buffer) +
           (static_cast<base::ssize>(index) * Size);
  }

  /*
  **	If the memory block buffer was allocated by this class, then this flag
  **	will be true. The block must be deallocated by this class if true.
  */
  unsigned IsAllocated : 1 {false};

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
  TFixedIHeapClass() noexcept : FixedIHeapClass(sizeof(T)) {}
  ~TFixedIHeapClass() override = default;
  TFixedIHeapClass(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass& operator=(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass(TFixedIHeapClass&&) = delete;
  TFixedIHeapClass& operator=(TFixedIHeapClass&&) = delete;

  using FixedIHeapClass::ID;

  virtual int ID(const T* pointer) { return FixedIHeapClass::ID(pointer); }
  virtual T* Alloc() { return static_cast<T*>(FixedIHeapClass::Allocate()); }
  virtual int Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  int Free(void* pointer) override { return FixedIHeapClass::Free(pointer); }
  int Save(ArchiveWriter&)
    requires Serializable<T>;
  int Load(ArchiveReader&)
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
int TFixedIHeapClass<T>::Save(ArchiveWriter& file)
  requires Serializable<T>
{
  int i;    // loop counter
  int32_t idx;  // object index

  /*
  ** Save the number of instances of this class
  */
  int32_t count = ActiveCount;
  file(count);

  /*
  ** Save each instance of this class
  */
  for (i = 0; i < ActiveCount; i++) {
    /*
    ** Save the array index of the object, so it can be loaded back into the
    ** same array location (so TARGET translations will work)
    */
    idx = ID(Ptr(i));
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
int TFixedIHeapClass<T>::Load(ArchiveReader& file)
  requires Serializable<T>
{
  int i;    // loop counter
  int32_t idx;  // object index
  T* ptr;   // object pointer
  int32_t a_count;

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
  for (i = 0; i < a_count; i++) {
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
    if (idx < 0 || idx >= TotalCount || FreeFlag[idx]) {
      file.Fail("invalid heap slot");
      return false;
    }
    ptr = static_cast<T*>((*this)[idx]);
    FreeFlag[idx] = true;
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
