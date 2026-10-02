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
#include <iterator>
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

bool FixedIHeapClass::Set_Heap(int count) {
  if (Size <= 0 || count < 0) {
    Clear();
    return false;
  }

  // Keep the storage when the slot count is unchanged; see the declaration.
  if (count != TotalCount || storage_ == nullptr) {
    Clear();
    if (count == 0) {
      return true;
    }
    const auto extent = base::ToSize(count * Size);
    storage_ = std::make_unique_for_overwrite<std::byte[]>(extent);
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    Buffer = std::span<std::byte>(storage_.get(), extent);
    TotalCount = count;
  }

  ActiveCount = 0;
  FreeFlag.assign(base::ToSize(count), false);
  ActivePointers.clear();
  // Reserving the whole heap up front means allocating during a game never
  // reallocates the list.
  ActivePointers.reserve(base::ToSize(count));
  return true;
}

void* FixedIHeapClass::Allocate() {
  // Full: skip the scan, which could only fail.
  if (ActiveCount >= TotalCount) {
    return nullptr;
  }
  const auto index = static_cast<int>(base::first_false(FreeFlag));
  if (index == -1) {
    return nullptr;
  }
  ActiveCount++;
  FreeFlag.at(base::ToSize(index)) = true;
  void* const pointer = (*this).at(index);
  ActivePointers.push_back(pointer);
  base::FillBytes(
      Buffer.subspan(base::ToSize(index * Size), base::ToSize(Size)), 0, Size);
  return pointer;
}

bool FixedIHeapClass::Free(void* pointer) {
  // ID() alone would round a pointer into the middle of a slot down to that
  // slot, free it, and leave the in-use list holding it.
  if (ActiveCount == 0 || !Owns(pointer)) {
    return false;
  }
  const int index = ID(pointer);
  if (!FreeFlag.at(base::ToSize(index))) {
    return false;
  }
  ActiveCount--;
  FreeFlag.at(base::ToSize(index)) = false;
  base::EraseFirst(ActivePointers, pointer);
  return true;
}

// Truncates toward zero, so a pointer just below the buffer also reads as
// slot 0; callers that cannot trust the pointer ask Owns() first.
int FixedIHeapClass::ID(const void* pointer) const {
  if (pointer && Size) {
    return static_cast<int>(
        (static_cast<const std::byte*>(pointer) - Buffer.data()) / Size);
  }
  return -1;
}

// Ordering pointers into different objects is only defined through
// std::less, which is why the comparison does not simply use `<`.
base::ssize FixedIHeapClass::Offset_Of(const void* pointer) const {
  if (pointer == nullptr || Buffer.empty()) {
    return -1;
  }
  const auto* address = static_cast<const std::byte*>(pointer);
  const std::less<> before;
  if (before(address, Buffer.data()) ||
      !before(address, Buffer.subspan(Buffer.size()).data())) {
    return -1;
  }
  return address - Buffer.data();
}

bool FixedIHeapClass::Owns(const void* pointer) const {
  const base::ssize offset = Offset_Of(pointer);
  return Size > 0 && offset >= 0 && offset % Size == 0;
}

std::string FixedIHeapClass::Describe(const void* pointer) const {
  std::string description = absl::StrFormat(
      "heap=%p slots=%d/%d size=%d buffer=[%p,%p) ptr=%p", this, ActiveCount,
      TotalCount, Size, static_cast<const void*>(Buffer.data()),
      static_cast<const void*>(
          Buffer.empty() ? nullptr : Buffer.subspan(Buffer.size()).data()),
      pointer);

  const base::ssize offset = Offset_Of(pointer);
  if (Buffer.empty()) {
    absl::StrAppend(&description, " (heap has no buffer)");
  } else if (pointer != nullptr && offset < 0) {
    absl::StrAppend(&description, " (outside the buffer)");
  } else if (pointer != nullptr) {
    const auto slot = static_cast<int>(offset / Size);
    const auto slack = static_cast<int>(offset % Size);
    absl::StrAppend(&description, " slot=", slot);
    if (slack != 0) {
      absl::StrAppend(&description, " (", slack, " bytes past the boundary)");
    } else {
      absl::StrAppend(&description,
                      FreeFlag.at(base::ToSize(slot)) ? " taken" : " FREE");
    }
  }
  return absl::StrCat(description,
                      " listed=", Logical_ID(pointer) >= 0 ? "yes" : "no");
}

void FixedIHeapClass::Clear() {
  Buffer = {};
  storage_.reset();
  ActiveCount = 0;
  TotalCount = 0;
  FreeFlag.clear();
  ActivePointers.clear();
}

void FixedIHeapClass::Free_All() {
  ActivePointers.clear();
  ActiveCount = 0;
  FreeFlag.assign(FreeFlag.size(), false);
}

int FixedIHeapClass::Logical_ID(const void* pointer) const {
  if (pointer == nullptr) {
    return -1;
  }
  return static_cast<int>(base::IndexOf(ActivePointers, pointer));
}

std::string FixedIHeapClass::Validate() const {
  if (std::ssize(ActivePointers) != ActiveCount) {
    return absl::StrCat("ActivePointers holds ", ActivePointers.size(), " of ",
                        ActiveCount, " active objects");
  }

  for (int index = 0; index < ActiveCount; index++) {
    const void* pointer = Active_Ptr(index);
    if (!Owns(pointer)) {
      return absl::StrCat("active pointer ", index,
                          " is not a slot: ", Describe(pointer));
    }
    if (!FreeFlag.at(base::ToSize(ID(pointer)))) {
      return absl::StrCat("active pointer ", index,
                          " was freed: ", Describe(pointer));
    }
    // Only now is the pointer known to be a live slot whose ID field can be
    // read.
    if (slot_id_ != nullptr) {
      const int cached = slot_id_(pointer);
      if (ID(pointer) != cached) {
        return absl::StrCat("stale ID ", cached, ": ", Describe(pointer));
      }
    }
  }
  return {};
}

}  // namespace game
