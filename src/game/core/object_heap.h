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

#ifndef CNC_RED_ALERT_GAME_CORE_OBJECT_HEAP_H_
#define CNC_RED_ALERT_GAME_CORE_OBJECT_HEAP_H_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/archive.h"

namespace game {

// The untyped core of ObjectHeap<T>: a pool of equally sized slots and the
// list of the slots in use, in void*. ObjectHeap<T> derives from it
// privately and is what the games use; this class exists so that the
// bookkeeping is compiled once rather than once per object type.
//
// The slot index is game state, not an allocation detail: TARGET values and
// saved pointers (HeapSlotPtr) name an object by it, and a saved game puts
// every object back into the slot it came from. The in-use list keeps
// allocation order, which is the order the game iterates objects in, so
// removing one keeps the others in order. Multiplayer games stay in sync only
// if that order is the same on every machine.
class ObjectHeapBase {
 public:
  // `slot_size` is the size of one slot in bytes, and must be positive.
  explicit ObjectHeapBase(const base::ssize slot_size) noexcept
      : slot_size_(slot_size) {
    CHECK_GT(slot_size, 0);
  }
  ~ObjectHeapBase() = default;
  ObjectHeapBase(const ObjectHeapBase&) = delete;
  ObjectHeapBase& operator=(const ObjectHeapBase&) = delete;
  ObjectHeapBase(ObjectHeapBase&&) = delete;
  ObjectHeapBase& operator=(ObjectHeapBase&&) = delete;

  // Returns the number of slots in use.
  [[nodiscard]] int Count() const {
    return static_cast<int>(std::ssize(active_));
  }
  // Returns the number of slots.
  [[nodiscard]] int Capacity() const {
    return static_cast<int>(std::ssize(in_use_));
  }
  // Returns the number of free slots.
  [[nodiscard]] int FreeCount() const { return Capacity() - Count(); }

  // Returns the index of the slot `pointer` points at the start of, or -1
  // when it does not point at the start of one: nullptr, a pointer outside
  // the heap, or one into the middle of a slot.
  [[nodiscard]] int SlotIndexOf(const void* pointer) const;

  // Whether `pointer` is the start of one of this heap's slots.
  [[nodiscard]] bool Owns(const void* pointer) const {
    return SlotIndexOf(pointer) >= 0;
  }

  // Whether slot `slot_index` holds an allocated object; false for a free
  // slot or an index outside the heap.
  [[nodiscard]] bool IsAllocated(const int slot_index) const {
    return slot_index >= 0 && slot_index < Capacity() &&
           in_use_.at(base::ToSize(slot_index));
  }

  // Makes room for `capacity` objects and frees every slot. A capacity of 0
  // leaves the heap without storage, and so does a negative one, for which
  // it returns false. Called again with the same capacity, it keeps the
  // storage where it is, because rule loading re-sizes the type heaps while
  // pointers into them are still held.
  bool Reset(int capacity);

  // Returns a zero-filled free slot, the lowest-numbered one, and appends it
  // to the in-use list. Returns nullptr when every slot is taken.
  void* AllocateSlot();

  // Marks slot `slot_index` in use and appends it to the in-use list, as
  // AllocateSlot() does for the slot it picks, but leaves the bytes alone.
  // Returns nullptr, doing nothing, when the index is outside the heap or
  // the slot is taken.
  void* ClaimSlot(int slot_index);

  // Frees the slot `pointer` names and removes it from the in-use list,
  // keeping the others in order. Returns false, doing nothing, when `pointer`
  // is not the start of one of the heap's slots (null included) or the slot
  // is already free. Does not run a destructor; an object's operator delete
  // calls this once its destructor has run.
  bool Free(void* pointer);

  // Frees every slot without running destructors.
  void FreeAll();

  // Everything known about the slot `pointer` claims to be, for the DCHECKs
  // that guard heap objects and for Validate(). The ways a pointer can be
  // wrong look different here: outside the buffer, inside but not on a slot
  // boundary, on a boundary whose slot is free (use after free), or on a
  // live slot the in-use list does not hold.
  [[nodiscard]] std::string Describe(const void* pointer) const;

  // Describes the first listed pointer that is not a slot or names a free
  // slot, or returns "" when there is none.
  [[nodiscard]] std::string CheckBookkeeping() const;

  // Returns the position of `pointer` in the in-use list, or -1 when the list
  // does not hold it. Scenario INI files number team and trigger types this
  // way: the types are written in list order, so the position is the place
  // of the type's entry and, read into an empty heap, its slot. The slot it
  // has now is not, once a type was deleted.
  [[nodiscard]] int PositionOf(const void* pointer) const;

  // Returns slot `slot_index`, which must be in [0, Capacity()).
  void* at(const int slot_index) { return SlotBytes(slot_index).data(); }

  // Returns the object at `position` of the in-use list, which must be in
  // [0, Count()).
  [[nodiscard]] void* ActiveSlot(const int position) const {
    return base::At(active_, position);
  }

 private:
  // The bytes of slot `slot_index`, which must be in [0, Capacity()).
  std::span<std::byte> SlotBytes(const int slot_index) {
    CHECK_GE(slot_index, 0);
    CHECK_LT(slot_index, Capacity());
    return std::span(slots_).subspan(base::ToSize(slot_index * slot_size_),
                                     base::ToSize(slot_size_));
  }

  // How far into the slots `pointer` lies, or -1 when it lies outside them.
  [[nodiscard]] base::ssize OffsetOf(const void* pointer) const;

  // Size of one slot in bytes.
  base::ssize slot_size_;

  // The slots, back to back; empty when the heap has none. std::allocator
  // gets them from operator new, which aligns them for any type up to
  // __STDCPP_DEFAULT_NEW_ALIGNMENT__; ObjectHeap checks its type against it.
  std::vector<std::byte> slots_;

  // Whether each slot is in use; its size is the capacity.
  std::vector<bool> in_use_;

  // The slots in use, in the order they were allocated.
  std::vector<void*> active_;
};

// Whether T caches its own slot index in a public `ID` field, the way
// everything descended from AbstractClass does. Spelled as a requirement
// rather than `std::derived_from<AbstractClass>` so that object_heap.h stays
// below the game's object hierarchy.
template <class T>
concept HasHeapId = requires(const T& object) {
  { object.ID } -> std::convertible_to<int>;
};

// The pool of slots one class of game object (units, buildings, triggers,
// ...) is allocated from, through the class's operator new. It hands out T*,
// can run the destructors of what it holds, and saves and loads its objects.
//
// Save and Load exist only for element types with field-wise Serialize().
// A Serializable T needs a default constructor reachable from this class
// (declare `friend class ObjectHeap<T>;`) that fully initializes the
// object without side effects; Load placement-news it into the slot before
// reading the fields.
//
// Example:
//   ObjectHeap<Widget> heap;
//   heap.Reset(100);                                    // room for 100
//   Widget* widget = std::construct_at(heap.Allocate());  // nullptr if full
//   std::destroy_at(widget);
//   heap.Free(widget);
template <class T>
class ObjectHeap : private ObjectHeapBase {
 public:
  static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                "the slots come from operator new, which aligns no more");

  ObjectHeap() noexcept : ObjectHeapBase(base::ssize{sizeof(T)}) {}
  ~ObjectHeap() = default;
  ObjectHeap(const ObjectHeap&) = delete;
  ObjectHeap& operator=(const ObjectHeap&) = delete;
  ObjectHeap(ObjectHeap&&) = delete;
  ObjectHeap& operator=(ObjectHeap&&) = delete;

  using ObjectHeapBase::Capacity;
  using ObjectHeapBase::Count;
  using ObjectHeapBase::Describe;
  using ObjectHeapBase::FreeAll;
  using ObjectHeapBase::FreeCount;
  using ObjectHeapBase::IsAllocated;
  using ObjectHeapBase::Owns;
  using ObjectHeapBase::Reset;

  // Returns the slot index of `object`, or -1 for nullptr.
  [[nodiscard]] int SlotIndexOf(const T* object) const {
    return ObjectHeapBase::SlotIndexOf(object);
  }
  // Returns the position of `object` in the in-use list, or -1 when the list
  // does not hold it.
  [[nodiscard]] int PositionOf(const T* object) const {
    return ObjectHeapBase::PositionOf(object);
  }

  // AllocateSlot() as a T*. The slot holds zero bytes, not a T: the caller
  // constructs the object there, which is what each class's operator new is
  // for. Returns nullptr when the heap is full.
  T* Allocate() { return static_cast<T*>(AllocateSlot()); }
  // Free() for a T; see ObjectHeapBase::Free().
  bool Free(T* object) { return ObjectHeapBase::Free(object); }
  // Runs the destructor of every active object, then frees every slot.
  // FreeAll() alone hands the slots back without destroying what is in
  // them, which leaks whatever the objects own; this is for objects whose
  // destructors only release memory, not ones that act on the game.
  void DestroyAll() {
    for (int i = 0; i < Count(); i++) {
      std::destroy_at(ActiveAt(i));
    }
    FreeAll();
  }

  // Describes the first sign of corruption, or returns "" when there is none:
  // a listed pointer that is not a slot, names a free slot, or names a slot
  // other than the one its object's cached ID gives. Walks every active
  // object, so it is for the once-a-frame -CHECKHEAPS sweep, not for the
  // inner loop.
  [[nodiscard]] std::string Validate() const;

  // Writes the active count, then each object's slot index and contents.
  // Returns writer.ok(): false once the sink has failed. Not const because
  // each object's Serialize() is one template for both directions.
  bool Save(ArchiveWriter& writer)
    requires Serializable<T>;
  // Reads what Save wrote back into the same slots of an empty heap sized
  // for them. Returns false on a malformed stream, with the reason in
  // reader.error(); the heap is then partially populated.
  bool Load(ArchiveReader& reader)
    requires Serializable<T>;

  // The object at `position` of the in-use list, which must be in
  // [0, Count()).
  T* ActiveAt(const int position) {
    return static_cast<T*>(ActiveSlot(position));
  }
  [[nodiscard]] const T* ActiveAt(const int position) const {
    return static_cast<const T*>(ActiveSlot(position));
  }
  // Slot `slot_index` as a T*, whether or not it holds an object;
  // `slot_index` must be in [0, Capacity()).
  T* Slot(const int slot_index) { return static_cast<T*>(at(slot_index)); }
  // Slot() for any index: nullptr when `slot_index` is outside
  // [0, Capacity()), -1 included. A scenario or a saved game names an object
  // by its slot before the object is created or loaded; the address is the
  // object's once it is, as long as the heap keeps its size.
  T* SlotOrNull(const int slot_index) {
    if (slot_index < 0 || slot_index >= Capacity()) {
      return nullptr;
    }
    return Slot(slot_index);
  }
  // SlotOrNull() for a slot that holds an object: nullptr as well when slot
  // `slot_index` is free, which would read as a zeroed object. For a
  // reference read once every object it can name exists.
  T* FilledSlotOrNull(const int slot_index) {
    return IsAllocated(slot_index) ? Slot(slot_index) : nullptr;
  }
};

template <class T>
std::string ObjectHeap<T>::Validate() const {
  std::string problem = CheckBookkeeping();
  if constexpr (HasHeapId<T>) {
    // Only once the bookkeeping holds is every listed pointer known to be a
    // live slot whose ID field can be read.
    for (int i = 0; problem.empty() && i < Count(); i++) {
      const T* object = ActiveAt(i);
      if (SlotIndexOf(object) != int{object->ID}) {
        problem = absl::StrCat("stale ID ", object->ID, ": ", Describe(object));
      }
    }
  }
  return problem;
}

template <class T>
bool ObjectHeap<T>::Save(ArchiveWriter& writer)
  requires Serializable<T>
{
  int32_t count = Count();
  writer(count);

  for (int i = 0; i < Count(); i++) {
    // The slot index goes first so the object lands in the same slot on
    // load, which keeps TARGET values valid across the round trip.
    int32_t slot_index = SlotIndexOf(ActiveAt(i));
    writer(slot_index);
    ActiveAt(i)->Serialize(writer);
  }
  return writer.ok();
}

template <class T>
bool ObjectHeap<T>::Load(ArchiveReader& reader)
  requires Serializable<T>
{
  int32_t count = 0;
  reader(count);
  if (!reader.ok()) {
    return false;
  }
  if (count < 0 || count > Capacity()) {
    reader.Fail("invalid saved heap count");
    return false;
  }

  for (int i = 0; i < count; i++) {
    int32_t slot_index = 0;
    reader(slot_index);
    if (!reader.ok()) {
      return false;
    }
    // Refusing a taken slot keeps a slot named twice from being listed twice.
    T* object = static_cast<T*>(ClaimSlot(slot_index));
    if (object == nullptr) {
      reader.Fail("invalid heap slot");
      return false;
    }
    new (object) T();
    object->Serialize(reader);
    if (!reader.ok()) {
      return false;
    }
  }
  return true;
}

// Verifies that `object` still sits in the heap slot its own ID field names,
// and dumps the slot's state when it does not. This is the invariant every
// heap object's methods assume, so the check guards most of them; it is a
// macro because only a macro can stream context into a DCHECK and still
// compile away in a release build.
//
// Example:
//   DCHECK_HEAP_SLOT(TheObjectHeaps().infantry(), this);
#define DCHECK_HEAP_SLOT(heap, object)                     \
  DCHECK_EQ((heap).SlotIndexOf(object), int{(object)->ID}) \
      << (heap).Describe(object)

// CHECK_HEAP_SLOT is the same test kept in a release build, for the heaps
// whose corruption the game cannot survive.
#define CHECK_HEAP_SLOT(heap, object)                     \
  CHECK_EQ((heap).SlotIndexOf(object), int{(object)->ID}) \
      << (heap).Describe(object)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_OBJECT_HEAP_H_
