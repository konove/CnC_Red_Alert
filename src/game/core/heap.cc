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

#include "game/core/heap.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "base/algorithm.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"

namespace game {

bool ObjectHeapBase::Reset(int capacity) {
  // Keep the storage when the capacity is unchanged; see the declaration.
  if (slot_size_ <= 0 || capacity < 0 || capacity != Capacity() ||
      storage_ == nullptr) {
    slots_ = {};
    storage_.reset();
    in_use_.clear();
    active_.clear();
    if (slot_size_ <= 0 || capacity < 0) {
      return false;
    }
    if (capacity == 0) {
      return true;
    }
    const auto extent = base::ToSize(capacity * slot_size_);
    storage_ = std::make_unique_for_overwrite<std::byte[]>(extent);
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    slots_ = std::span<std::byte>(storage_.get(), extent);
  }

  in_use_.assign(base::ToSize(capacity), false);
  active_.clear();
  // Reserving the whole heap up front means allocating during a game never
  // reallocates the list.
  active_.reserve(base::ToSize(capacity));
  return true;
}

void* ObjectHeapBase::AllocateSlot() {
  const auto slot_index = static_cast<int>(base::first_false(in_use_));
  if (slot_index == -1) {
    return nullptr;
  }
  void* const pointer = ClaimSlot(slot_index);
  base::FillBytes(slots_.subspan(base::ToSize(slot_index * slot_size_),
                                 base::ToSize(slot_size_)),
                  0, slot_size_);
  return pointer;
}

void* ObjectHeapBase::ClaimSlot(int slot_index) {
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
  // SlotIndexOf() alone would round a pointer into the middle of a slot down
  // to that slot, free it, and leave the in-use list holding it.
  if (!Owns(pointer)) {
    return false;
  }
  const int slot_index = SlotIndexOf(pointer);
  if (!in_use_.at(base::ToSize(slot_index))) {
    return false;
  }
  in_use_.at(base::ToSize(slot_index)) = false;
  base::EraseFirst(active_, pointer);
  return true;
}

// Truncates toward zero, so a pointer just below the buffer also reads as
// slot 0; callers that cannot trust the pointer ask Owns() first.
int ObjectHeapBase::SlotIndexOf(const void* pointer) const {
  if (pointer && slot_size_) {
    return static_cast<int>(
        (static_cast<const std::byte*>(pointer) - slots_.data()) / slot_size_);
  }
  return -1;
}

// Ordering pointers into different objects is only defined through
// std::less, which is why the comparison does not simply use `<`.
base::ssize ObjectHeapBase::OffsetOf(const void* pointer) const {
  if (pointer == nullptr || slots_.empty()) {
    return -1;
  }
  const auto* address = static_cast<const std::byte*>(pointer);
  const std::less<> before;
  if (before(address, slots_.data()) ||
      !before(address, slots_.subspan(slots_.size()).data())) {
    return -1;
  }
  return address - slots_.data();
}

bool ObjectHeapBase::Owns(const void* pointer) const {
  const base::ssize offset = OffsetOf(pointer);
  return slot_size_ > 0 && offset >= 0 && offset % slot_size_ == 0;
}

std::string ObjectHeapBase::Describe(const void* pointer) const {
  std::string description = absl::StrFormat(
      "heap=%p slots=%d/%d size=%d buffer=[%p,%p) ptr=%p", this, Count(),
      Capacity(), slot_size_, static_cast<const void*>(slots_.data()),
      static_cast<const void*>(
          slots_.empty() ? nullptr : slots_.subspan(slots_.size()).data()),
      pointer);

  const base::ssize offset = OffsetOf(pointer);
  if (slots_.empty()) {
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
      absl::StrAppend(&description,
                      in_use_.at(base::ToSize(slot)) ? " taken" : " FREE");
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

std::string ObjectHeapBase::Validate() const {
  for (int position = 0; position < Count(); position++) {
    const void* pointer = ActiveSlot(position);
    if (!Owns(pointer)) {
      return absl::StrCat("active pointer ", position,
                          " is not a slot: ", Describe(pointer));
    }
    if (!in_use_.at(base::ToSize(SlotIndexOf(pointer)))) {
      return absl::StrCat("active pointer ", position,
                          " was freed: ", Describe(pointer));
    }
    // Only now is the pointer known to be a live slot whose ID field can be
    // read.
    if (read_slot_id_ != nullptr) {
      const int cached = read_slot_id_(pointer);
      if (SlotIndexOf(pointer) != cached) {
        return absl::StrCat("stale ID ", cached, ": ", Describe(pointer));
      }
    }
  }
  return {};
}

}  // namespace game
