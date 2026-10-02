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

// File: the fixed-slot object pool.

#include "game/core/object_heap.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "base/algorithm.h"
#include "base/numeric.h"
#include "base/types.h"

namespace game {

bool ObjectHeapBase::Reset(const int capacity) {
  if (capacity < 0) {
    Reset(0);
    return false;
  }
  // Keep the storage when the capacity is unchanged; see the declaration.
  if (capacity != Capacity()) {
    slots_ = std::vector<std::byte>(base::ToSize(capacity * slot_size_));
  }
  in_use_.assign(base::ToSize(capacity), false);
  active_.clear();
  // Reserving the whole heap up front means allocating during a game never
  // reallocates the list.
  active_.reserve(base::ToSize(capacity));
  return true;
}

void* ObjectHeapBase::AllocateSlot() {
  // Heavy fighting fills the bullet and animation heaps; refuse without
  // testing every flag.
  if (Count() >= Capacity()) {
    return nullptr;
  }
  const auto slot_index = static_cast<int>(base::first_false(in_use_));
  if (slot_index == -1) {
    return nullptr;
  }
  std::ranges::fill(SlotBytes(slot_index), std::byte{0});
  return ClaimSlot(slot_index);
}

void* ObjectHeapBase::ClaimSlot(const int slot_index) {
  if (slot_index < 0 || slot_index >= Capacity() ||
      in_use_.at(base::ToSize(slot_index))) {
    return nullptr;
  }
  in_use_.at(base::ToSize(slot_index)) = true;
  void* const pointer = at(slot_index);
  active_.push_back(pointer);
  return pointer;
}

bool ObjectHeapBase::Free(void* pointer) {
  const int slot_index = SlotIndexOf(pointer);
  if (!IsAllocated(slot_index)) {
    return false;
  }
  in_use_.at(base::ToSize(slot_index)) = false;
  base::EraseFirst(active_, pointer);
  return true;
}

int ObjectHeapBase::SlotIndexOf(const void* pointer) const {
  const base::ssize offset = OffsetOf(pointer);
  if (offset < 0 || offset % slot_size_ != 0) {
    return -1;
  }
  return static_cast<int>(offset / slot_size_);
}

// Ordering pointers into different objects is only defined through
// std::less, which is why the comparison does not simply use `<`.
base::ssize ObjectHeapBase::OffsetOf(const void* pointer) const {
  if (pointer == nullptr || slots_.empty()) {
    return -1;
  }
  const auto* address = static_cast<const std::byte*>(pointer);
  const std::span<const std::byte> slots(slots_);
  constexpr std::less<> before;
  if (before(address, slots.data()) ||
      !before(address, slots.subspan(slots.size()).data())) {
    return -1;
  }
  return address - slots.data();
}

std::string ObjectHeapBase::Describe(const void* pointer) const {
  const std::span<const std::byte> slots(slots_);
  std::string description = absl::StrFormat(
      "heap=%p slots=%d/%d size=%d buffer=[%p,%p) ptr=%p", this, Count(),
      Capacity(), slot_size_, static_cast<const void*>(slots.data()),
      static_cast<const void*>(slots.subspan(slots.size()).data()), pointer);

  const base::ssize offset = OffsetOf(pointer);
  if (slots.empty()) {
    absl::StrAppend(&description, " (heap has no buffer)");
  } else if (pointer != nullptr && offset < 0) {
    absl::StrAppend(&description, " (outside the buffer)");
  } else if (pointer != nullptr) {
    const auto slot = static_cast<int>(offset / slot_size_);
    const auto slack = static_cast<int>(offset % slot_size_);
    absl::StrAppend(&description, " slot=", slot);
    if (slack != 0) {
      absl::StrAppend(&description, " (", slack, " bytes past the boundary)");
    } else {
      absl::StrAppend(&description, IsAllocated(slot) ? " taken" : " FREE");
    }
  }
  return absl::StrCat(description,
                      " listed=", PositionOf(pointer) >= 0 ? "yes" : "no");
}

void ObjectHeapBase::FreeAll() {
  active_.clear();
  in_use_.assign(in_use_.size(), false);
}

int ObjectHeapBase::PositionOf(const void* pointer) const {
  if (pointer == nullptr) {
    return -1;
  }
  return static_cast<int>(base::IndexOf(active_, pointer));
}

std::string ObjectHeapBase::CheckBookkeeping() const {
  for (int position = 0; position < Count(); position++) {
    const void* pointer = ActiveSlot(position);
    if (!Owns(pointer)) {
      return absl::StrCat("active pointer ", position,
                          " is not a slot: ", Describe(pointer));
    }
    if (!IsAllocated(SlotIndexOf(pointer))) {
      return absl::StrCat("active pointer ", position,
                          " was freed: ", Describe(pointer));
    }
  }
  return {};
}

}  // namespace game
