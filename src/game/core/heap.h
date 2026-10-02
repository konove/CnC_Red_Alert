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

// File: the fixed-slot object pools that game objects are allocated from.

#ifndef CNC_RED_ALERT_GAME_CORE_HEAP_H_
#define CNC_RED_ALERT_GAME_CORE_HEAP_H_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/archive.h"

namespace game {

// A pool of equally sized slots for one class of game object, and the list
// of the slots in use. Each class with a fixed number of objects (units,
// buildings, triggers, ...) allocates them here through its operator new.
//
// The slot index is game state, not an allocation detail: TARGET values and
// CCPtr name an object by it, and a saved game puts every object back into
// the slot it came from. The in-use list keeps allocation order, which is the
// order the game iterates objects in, so removing one keeps the others in
// order. Multiplayer games stay in sync only if that order is the same on
// every machine.
//
// Nothing is virtual: CCPtr reaches a heap through a FixedIHeapClass*, but
// only for its slots, and nothing derives from it except the typed
// TFixedIHeapClass below.
//
// Example:
//   FixedIHeapClass heap(sizeof(Widget));
//   heap.Set_Heap(100);              // room for 100 widgets
//   void* slot = heap.Allocate();    // zero-filled; nullptr when full
//   heap.Free(slot);
class FixedIHeapClass {
 public:
  // Reads the slot index an object caches in its own `ID` field.
  using SlotIdReader = int (*)(const void* object);

  // `size` is the size of one slot. slot_id is nullptr for objects that cache
  // no slot index; Validate() then checks the heap's own bookkeeping only.
  // clang's lifetime analysis reads slot_id as borrowed storage, but a
  // function pointer outlives any heap.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
  explicit FixedIHeapClass(int size, SlotIdReader slot_id = nullptr) noexcept
      : Size(size), slot_id_(slot_id) {}
  ~FixedIHeapClass() = default;
  FixedIHeapClass(const FixedIHeapClass&) = delete;
  FixedIHeapClass& operator=(const FixedIHeapClass&) = delete;
  FixedIHeapClass(FixedIHeapClass&&) = delete;
  FixedIHeapClass& operator=(FixedIHeapClass&&) = delete;

  // Returns the number of slots in use.
  [[nodiscard]] int Count() const { return ActiveCount; }
  // Returns the number of slots.
  [[nodiscard]] int Length() const { return TotalCount; }
  // Returns the number of free slots.
  [[nodiscard]] int Avail() const { return TotalCount - ActiveCount; }

  // Returns the slot index of `pointer`, or -1 for nullptr. Any other pointer
  // gives a number, so use Owns() first when `pointer` may not be a slot.
  [[nodiscard]] int ID(const void* pointer) const;

  // Whether slot `index` holds an allocated object; false for a free slot
  // or an index outside the heap.
  [[nodiscard]] bool Is_Allocated(int index) const {
    return index >= 0 && index < TotalCount && FreeFlag.at(base::ToSize(index));
  }

  // Makes room for `count` objects and frees every slot. Returns false for a
  // negative count or a heap whose slot size is not positive. A count of 0
  // leaves the heap without storage. Called again with the same count, it
  // keeps the storage where it is, because rule loading re-sizes the type
  // heaps while pointers into them are still held.
  bool Set_Heap(int count);

  // Returns a zero-filled free slot, the lowest-numbered one, and appends it
  // to the in-use list. Returns nullptr when every slot is taken.
  void* Allocate();

  // Releases the storage; the heap holds no slots afterwards.
  void Clear();

  // Frees the slot `pointer` names and removes it from the in-use list,
  // keeping the others in order. Returns false, doing nothing, when `pointer`
  // is null, outside the heap or already free. Does not run a destructor.
  bool Free(void* pointer);

  // Frees every slot without running destructors.
  void Free_All();

  // How far into the buffer `pointer` lies, or -1 when it lies outside it.
  [[nodiscard]] base::ssize Offset_Of(const void* pointer) const;

  // Whether `pointer` is the start of one of this heap's slots. ID() cannot
  // answer that: it subtracts and divides whatever it is handed, so a pointer
  // from somewhere else still comes back as a plausible index.
  [[nodiscard]] bool Owns(const void* pointer) const;

  // Everything known about the slot `pointer` claims to be, for the DCHECKs
  // that guard heap objects and for Validate(). The ways a pointer can be
  // wrong look different here: outside the buffer, inside but not on a slot
  // boundary, on a boundary whose slot is free (use after free), or on a
  // live slot the in-use list does not hold.
  [[nodiscard]] std::string Describe(const void* pointer) const;

  // Describes the first disagreement between the in-use list, the free
  // flags, the slot buffer and the slot index each object caches, or "" when
  // the heap is sound. Walks every active pointer, so it is for the
  // once-a-frame -CHECKHEAPS sweep, not for the inner loop.
  [[nodiscard]] std::string Validate() const;

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

  // Returns the position of `pointer` in the in-use list, or -1 when the list
  // does not hold it.
  [[nodiscard]] int Logical_ID(const void* pointer) const;
  [[nodiscard]] int Logical_ID(int id) const {
    return Logical_ID((*this).at(id));
  }

  // Returns the object at position `index` of the in-use list, which must be
  // in [0, Count()).
  void* Active_Ptr(int index) { return base::At(ActivePointers, index); }
  [[nodiscard]] const void* Active_Ptr(int index) const {
    return base::At(ActivePointers, index);
  }

 protected:
  // Size of one slot in bytes.
  int Size;

  // Number of slots in the buffer.
  int TotalCount{0};

  // Number of slots in use; never more than TotalCount.
  int ActiveCount{0};

  // The slots, viewing storage_; empty when the heap has none.
  std::span<char> Buffer;

  // Whether each slot is in use (true) or free.
  std::vector<bool> FreeFlag;

  // The slots in use, in the order they were allocated.
  std::vector<void*> ActivePointers;

 private:
  // The allocation Buffer views. operator new[] aligns it for any type up to
  // __STDCPP_DEFAULT_NEW_ALIGNMENT__, which TFixedIHeapClass checks.
  std::unique_ptr<char[]> storage_;

  SlotIdReader slot_id_;  // nullptr when the objects cache no slot index.
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
  static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                "the slots are carved from new char[], which aligns no more");

  TFixedIHeapClass() noexcept : FixedIHeapClass(sizeof(T), SlotIdReaderFor()) {}
  ~TFixedIHeapClass() = default;
  TFixedIHeapClass(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass& operator=(const TFixedIHeapClass&) = delete;
  TFixedIHeapClass(TFixedIHeapClass&&) = delete;
  TFixedIHeapClass& operator=(TFixedIHeapClass&&) = delete;

  using FixedIHeapClass::Free;
  using FixedIHeapClass::ID;
  using FixedIHeapClass::Logical_ID;

  [[nodiscard]] int ID(const T* pointer) const {
    return FixedIHeapClass::ID(pointer);
  }
  [[nodiscard]] int Logical_ID(const T* pointer) const {
    return FixedIHeapClass::Logical_ID(pointer);
  }

  T* Alloc() { return static_cast<T*>(FixedIHeapClass::Allocate()); }
  // Runs the destructor of every active object, then frees every slot.
  // Free_All() alone hands the slots back without destroying what is in
  // them, which leaks whatever the objects own; this is for objects whose
  // destructors only release memory, not ones that act on the game.
  void Destroy_All() {
    for (int i = 0; i < Count(); i++) {
      std::destroy_at(Ptr(i));
    }
    Free_All();
  }
  bool Free(T* pointer) { return FixedIHeapClass::Free(pointer); }
  // Writes the active count, then each object's slot index and contents.
  bool Save(ArchiveWriter& writer) const
    requires Serializable<T>;
  // Reads what Save wrote back into the same slots. Returns false on a
  // malformed stream, with the reason in reader.error(); the heap is then
  // partially populated.
  bool Load(ArchiveReader& reader)
    requires Serializable<T>;
  [[nodiscard]] T* Ptr(int index) const {
    return static_cast<T*>(base::At(ActivePointers, index));
  }
  T* Raw_Ptr(int index) { return static_cast<T*>((*this).at(index)); }

 private:
  // Lets Validate() check that each object's cached ID still names the slot
  // it occupies, for the types that cache one.
  static constexpr SlotIdReader SlotIdReaderFor() {
    if constexpr (HasHeapId<T>) {
      return [](const void* object) {
        return int{static_cast<const T*>(object)->ID};
      };
    } else {
      return nullptr;
    }
  }
};

template <class T>
bool TFixedIHeapClass<T>::Save(ArchiveWriter& writer) const
  requires Serializable<T>
{
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
bool TFixedIHeapClass<T>::Load(ArchiveReader& reader)
  requires Serializable<T>
{
  int32_t count = 0;
  reader(count);
  if (!reader.ok()) {
    return false;
  }
  if (count < 0 || count > TotalCount) {
    reader.Fail("invalid saved heap count");
    return false;
  }

  for (int i = 0; i < count; i++) {
    int32_t idx = 0;
    reader(idx);
    if (!reader.ok()) {
      return false;
    }
    // A slot named twice would be activated twice and listed twice.
    if (idx < 0 || idx >= TotalCount || FreeFlag.at(base::ToSize(idx))) {
      reader.Fail("invalid heap slot");
      return false;
    }

    T* ptr = static_cast<T*>((*this).at(idx));
    FreeFlag.at(base::ToSize(idx)) = true;
    ActiveCount++;
    ActivePointers.push_back(ptr);

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

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_HEAP_H_
