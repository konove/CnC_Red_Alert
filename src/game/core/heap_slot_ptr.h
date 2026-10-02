#ifndef CNC_RED_ALERT_GAME_CORE_HEAP_SLOT_PTR_H_
#define CNC_RED_ALERT_GAME_CORE_HEAP_SLOT_PTR_H_

// File: a save-game proxy that stores a pointer into a fixed heap as the
// index of the slot it points at.

#include <cstdint>
#include <type_traits>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "game/core/heap.h"

namespace game {

// Serializes a pointer to an object in `heap` as its slot index: an int32,
// -1 for nullptr. On read the pointer is set to the slot's address whether or
// not the slot is filled yet, so a pointer into a heap that loads later is
// right once that heap has loaded. A loaded index outside the heap records an
// error on the reader and reads as nullptr. T may be const.
//
// Red Alert's former CCPtr wrote the same encoding, so saves from before its
// removal still load.
//
// Example:
//   template <class Archive>
//   void TeamClass::Serialize(Archive& ar) {
//     ar(HeapSlotPtr(Trigger, TheObjectHeaps().trigger()));
//   }
template <class T>
class HeapSlotPtr {
 public:
  using Heap = ObjectHeap<std::remove_const_t<T>>;

  HeapSlotPtr(T*& pointer ABSL_ATTRIBUTE_LIFETIME_BOUND,
              Heap& heap ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : pointer_(pointer), heap_(heap) {}

  template <class Archive>
  void Serialize(Archive& ar) {
    int32_t slot_index = -1;
    if constexpr (!Archive::kIsReading) {
      if (pointer_ != nullptr) {
        DCHECK(heap_.Owns(pointer_)) << heap_.Describe(pointer_);
        slot_index = heap_.SlotIndexOf(pointer_);
      }
    }
    ar(slot_index);
    if constexpr (Archive::kIsReading) {
      pointer_ = heap_.SlotOrNull(slot_index);
      if (pointer_ == nullptr && slot_index != -1) {
        ar.Fail("saved slot index outside its heap");
      }
    }
  }

 private:
  T*& pointer_;
  Heap& heap_;
};

template <class T>
HeapSlotPtr(T*&, ObjectHeap<std::remove_const_t<T>>&) -> HeapSlotPtr<T>;

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_HEAP_SLOT_PTR_H_
