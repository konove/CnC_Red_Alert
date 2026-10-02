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
// saved pointers (HeapSlotPtr) name an object by it, and a saved game puts
// every object back into the slot it came from. The in-use list keeps
// allocation order, which is the order the game iterates objects in, so
// removing one keeps the others in order. Multiplayer games stay in sync only
// if that order is the same on every machine.
//
// Nothing is virtual: nothing derives from it except the typed
// ObjectHeap below.
//
// Example:
//   ObjectHeapBase heap(sizeof(Widget));
//   heap.Reset(100);                   // room for 100 widgets
//   void* slot = heap.AllocateSlot();  // zero-filled; nullptr when full
//   heap.Free(slot);
class ObjectHeapBase {
 public:
  // Reads the slot index an object caches in its own `ID` field.
  using SlotIdReader = int (*)(const void* object);

  // `slot_size` is the size of one slot in bytes. `read_slot_id` is nullptr
  // for objects that cache no slot index; Validate() then checks the heap's
  // own bookkeeping only. clang's lifetime analysis reads `read_slot_id` as
  // borrowed storage, but a function pointer outlives any heap.
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
  explicit ObjectHeapBase(base::ssize slot_size,
                          SlotIdReader read_slot_id = nullptr) noexcept
      : slot_size_(slot_size), read_slot_id_(read_slot_id) {}
  // NOLINTEND(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
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

  // Returns the slot index of `pointer`, or -1 for nullptr. Any other pointer
  // gives a number, so use Owns() first when `pointer` may not be a slot.
  [[nodiscard]] int SlotIndexOf(const void* pointer) const;

  // Whether slot `slot_index` holds an allocated object; false for a free
  // slot or an index outside the heap.
  [[nodiscard]] bool IsAllocated(int slot_index) const {
    return slot_index >= 0 && slot_index < Capacity() &&
           in_use_.at(base::ToSize(slot_index));
  }

  // Makes room for `capacity` objects and frees every slot. Returns false for
  // a negative capacity or a heap whose slot size is not positive. A capacity
  // of 0 leaves the heap without storage. Called again with the same
  // capacity, it keeps the storage where it is, because rule loading re-sizes
  // the type heaps while pointers into them are still held.
  bool Reset(int capacity);

  // Returns a zero-filled free slot, the lowest-numbered one, and appends it
  // to the in-use list. Returns nullptr when every slot is taken.
  void* AllocateSlot();

  // Frees the slot `pointer` names and removes it from the in-use list,
  // keeping the others in order. Returns false, doing nothing, when `pointer`
  // is not the start of one of the heap's slots (null included) or the slot
  // is already free. Does not run a destructor; an object's operator delete
  // calls this once its destructor has run.
  bool Free(void* pointer);

  // Frees every slot without running destructors.
  void FreeAll();

  // Whether `pointer` is the start of one of this heap's slots. SlotIndexOf()
  // cannot answer that: it subtracts and divides whatever it is handed, so a
  // pointer from somewhere else still comes back as a plausible index.
  [[nodiscard]] bool Owns(const void* pointer) const;

  // Everything known about the slot `pointer` claims to be, for the DCHECKs
  // that guard heap objects and for Validate(). The ways a pointer can be
  // wrong look different here: outside the buffer, inside but not on a slot
  // boundary, on a boundary whose slot is free (use after free), or on a
  // live slot the in-use list does not hold.
  [[nodiscard]] std::string Describe(const void* pointer) const;

  // Describes the first sign of corruption, or returns "" when there is none:
  // a listed pointer that is not a slot, names a free slot, or names a slot
  // other than the one its object's cached ID gives. Walks every active
  // pointer, so it is for the once-a-frame -CHECKHEAPS sweep, not for the
  // inner loop.
  [[nodiscard]] std::string Validate() const;

  // Returns the position of `pointer` in the in-use list, or -1 when the list
  // does not hold it. Scenario INI files number team and trigger types this
  // way.
  [[nodiscard]] int PositionOf(const void* pointer) const;

 protected:
  // Returns slot `slot_index`, which must be in [0, Capacity()).
  void* at(int slot_index) {
    CHECK_GE(slot_index, 0);
    CHECK_LT(slot_index, Capacity());
    return slots_.subspan(base::ToSize(slot_index * slot_size_)).data();
  }

  // Returns the object at `position` of the in-use list, which must be in
  // [0, Count()).
  void* ActiveSlot(int position) { return base::At(active_, position); }
  [[nodiscard]] const void* ActiveSlot(int position) const {
    return base::At(active_, position);
  }

  // Marks slot `slot_index` in use and appends it to the in-use list, as
  // AllocateSlot() does for the slot it picks, but leaves the bytes alone.
  // Returns nullptr, doing nothing, when the index is outside the heap or
  // the slot is taken.
  void* ClaimSlot(int slot_index);

 private:
  // How far into the buffer `pointer` lies, or -1 when it lies outside it.
  [[nodiscard]] base::ssize OffsetOf(const void* pointer) const;

  // Size of one slot in bytes.
  base::ssize slot_size_;

  // The slots, viewing storage_; empty when the heap has none.
  std::span<std::byte> slots_;

  // Whether each slot is in use; its size is the capacity.
  std::vector<bool> in_use_;

  // The slots in use, in the order they were allocated.
  std::vector<void*> active_;

  // The allocation slots_ views. operator new[] aligns it for any type up to
  // __STDCPP_DEFAULT_NEW_ALIGNMENT__, which ObjectHeap checks.
  std::unique_ptr<std::byte[]> storage_;

  SlotIdReader read_slot_id_;  // nullptr when the objects cache no slot index.
};

// Whether T caches its own slot index in a public `ID` field, the way
// everything descended from AbstractClass does. Spelled as a requirement
// rather than `std::derived_from<AbstractClass>` so that heap.h stays below
// the game's object hierarchy.
template <class T>
concept HasHeapId = requires(const T& object) {
  { object.ID } -> std::convertible_to<int>;
};

// ObjectHeapBase typed for T: it hands out T* rather than void*, can run the
// destructors of what it holds, and saves and loads its objects.
//
// Save and Load exist only for element types with field-wise Serialize().
// A Serializable T needs a default constructor reachable from this class
// (declare `friend class ObjectHeap<T>;`) that fully initializes the
// object without side effects; Load placement-news it into the slot before
// reading the fields.
template <class T>
class ObjectHeap : public ObjectHeapBase {
 public:
  static_assert(
      alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
      "the slots are carved from new std::byte[], which aligns no more");

  ObjectHeap() noexcept
      : ObjectHeapBase(base::ssize{sizeof(T)}, SlotIdReaderFor()) {}
  ~ObjectHeap() = default;
  ObjectHeap(const ObjectHeap&) = delete;
  ObjectHeap& operator=(const ObjectHeap&) = delete;
  ObjectHeap(ObjectHeap&&) = delete;
  ObjectHeap& operator=(ObjectHeap&&) = delete;

  // AllocateSlot() as a T*. The slot holds zero bytes, not a T: the caller
  // constructs the object there, which is what each class's operator new is
  // for. Returns nullptr when the heap is full.
  T* Allocate() { return static_cast<T*>(ObjectHeapBase::AllocateSlot()); }
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
  // ActiveSlot() as a T*: the object at `position` of the in-use list.
  T* ActiveAt(int position) { return static_cast<T*>(ActiveSlot(position)); }
  [[nodiscard]] const T* ActiveAt(int position) const {
    return static_cast<const T*>(ActiveSlot(position));
  }
  // Slot `slot_index` as a T*, whether or not it holds an object;
  // `slot_index` must be in [0, Capacity()).
  T* Slot(int slot_index) { return static_cast<T*>((*this).at(slot_index)); }
  // Slot() for any index: nullptr when `slot_index` is outside
  // [0, Capacity()), -1 included. A scenario or a saved game names an object
  // by its slot before the object is created or loaded; the address is the
  // object's once it is, as long as the heap keeps its size.
  T* SlotOrNull(int slot_index) {
    if (slot_index < 0 || slot_index >= Capacity()) {
      return nullptr;
    }
    return Slot(slot_index);
  }

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
#define DCHECK_HEAP_SLOT(heap, object)                     \
  DCHECK_EQ((heap).SlotIndexOf(object), int{(object)->ID}) \
      << (heap).Describe(object)

// CHECK_HEAP_SLOT is the same test kept in a release build, for the heaps
// whose corruption the game cannot survive.
#define CHECK_HEAP_SLOT(heap, object)                     \
  CHECK_EQ((heap).SlotIndexOf(object), int{(object)->ID}) \
      << (heap).Describe(object)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_HEAP_H_
