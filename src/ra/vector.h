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

// Dynamically-sized array containers for game objects.

#ifndef CNC_RED_ALERT_RA_VECTOR_H_
#define CNC_RED_ALERT_RA_VECTOR_H_

#include <new>  // IWYU pragma: keep
#include <span>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "base/types.h"
#include "ra/defines.h"  // IWYU pragma: keep
#include "ra/egos.h"     // IWYU pragma: keep

// Resizable array of arbitrary objects. Not optimized for integral types -
// consider a specialized version for char/int if performance is critical.
template <typename T>
class VectorClass {
 public:
  // clang suggests lifetimebound here, but its lifetimebound-violation check
  // cannot verify it.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
  explicit VectorClass(base::ssize size = 0, std::span<T> array = {});
  VectorClass(const VectorClass& /*vector*/);  // Copy constructor.
  virtual ~VectorClass();
  VectorClass(VectorClass&&) = delete;
  VectorClass& operator=(VectorClass&&) = delete;

  T& operator[](base::ssize index) {
    DCHECK(index >= 0 && index < VectorMax);
    return Elements()[base::ToSize(index)];
  }
  const T& operator[](base::ssize index) const {
    DCHECK(index >= 0 && index < VectorMax);
    return Elements()[base::ToSize(index)];
  }
  VectorClass& operator=(
      const VectorClass& /*vector*/);  // Assignment operator.
  virtual bool operator==(
      const VectorClass& /*vector*/) const;  // Equality operator.
  virtual bool Resize(base::ssize newsize, std::span<T> array = {});
  virtual void Clear();
  [[nodiscard]] base::ssize Length() const { return VectorMax; }
  virtual base::ssize ID(const T* ptr);  // Pointer based identification.
  virtual base::ssize ID(const T& object);  // Value based identification.

 protected:
  // The pointer and count retain their historical serialized layout. Every
  // allocation sets both together; external storage is supplied as a span and
  // checked by the constructor/Resize before this owner borrows it.
  [[nodiscard]] std::span<T> Elements() const {
    CHECK_GE(VectorMax, 0);
    CHECK(Vector != nullptr || VectorMax == 0);
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    return {Vector, base::ToSize(VectorMax)};
  }
  T* Vector;                // Pointer to element array.
  base::ssize VectorMax{0};  // Maximum number of elements.
  bool IsAllocated
      : true {false};  // True if we own the memory and must delete it.
};

// Implementation details only below here

template <class T>
VectorClass<T>::VectorClass(base::ssize size, std::span<T> array)
    : Vector(nullptr), VectorMax(size) {
  CHECK_GE(size, 0);
  CHECK(array.empty() || base::ToSize(size) <= array.size());
  if (size > 0) {
    if (!array.empty()) {
      Vector = new (static_cast<void*>(array.data()))
          T[base::ToSize(size)];  // Placement new into provided buffer.
    } else {
      // Value initialized: callers routinely read capacity that has not been
      // assigned yet (Resize copies the whole old capacity, not just the
      // elements in use), which is an indeterminate value for a POD T.
      Vector = new T[base::ToSize(size)]();
      IsAllocated = true;
    }
  }
}

template <class T>
VectorClass<T>::~VectorClass() {
  VectorClass<T>::Clear();
}

template <class T>
VectorClass<T>::VectorClass(const VectorClass<T>& vector) : Vector(nullptr) {
  *this = vector;
}

template <class T>
VectorClass<T>& VectorClass<T>::operator=(const VectorClass<T>& vector) {
  if (this != &vector) {
    Clear();
    VectorMax = vector.Length();
    if (VectorMax > 0) {
      Vector = new T[base::ToSize(VectorMax)];
      if (Vector) {
        IsAllocated = true;
        for (base::ssize index = 0; index < VectorMax; index++) {
          Elements()[base::ToSize(index)] = vector[index];
        }
      }
    } else {
      Vector = nullptr;
      IsAllocated = false;
    }
  }
  return *this;
}

// Element-by-element comparison. Requires T to have operator!=.
template <class T>
bool VectorClass<T>::operator==(const VectorClass<T>& vector) const {
  if (VectorMax == vector.Length()) {
    for (base::ssize index = 0; index < VectorMax; index++) {
      if (Elements()[base::ToSize(index)] != vector[index]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

// Converts pointer to index via pointer arithmetic. Only valid for pointers
// into this vector.
template <class T>
base::ssize VectorClass<T>::ID(const T* ptr) {
  // Uses Vector rather than &(*this)[0] so that querying an empty vector does
  // not trip the bounds check in operator[].
  return ptr - Vector;
}

// Finds index of first element equal to object. Returns -1 if not found.
template <class T>
base::ssize VectorClass<T>::ID(const T& object) {
  for (base::ssize index = 0; index < VectorMax; index++) {
    if ((*this)[index] == object) {
      return index;
    }
  }
  return -1;
}

// Frees memory and resets to empty state.
template <class T>
void VectorClass<T>::Clear() {
  if (Vector && IsAllocated) {
    delete[] Vector;
    Vector = nullptr;
  }
  IsAllocated = false;
  VectorMax = 0;
}

// Changes capacity, preserving existing elements up to new size.
// If array is provided, uses placement new into that buffer.
template <class T>
bool VectorClass<T>::Resize(base::ssize newsize, std::span<T> array) {
  CHECK_GE(newsize, 0);
  CHECK(array.empty() || base::ToSize(newsize) <= array.size());
  if (newsize > 0) {
    T* newptr = nullptr;
    if (array.empty()) {
      newptr = new T[base::ToSize(newsize)]();  // Value initialized, see the constructor.
    } else {
      newptr = new (static_cast<void*>(array.data())) T[base::ToSize(newsize)];
    }
    if (!newptr) {
      return false;
    }
    // newptr is the newsize-element allocation or the checked external span
    // above. NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<T> replacement(newptr, base::ToSize(newsize));

    if (Vector) {
      // Copy existing elements (uses assignment operator). This copies the
      // whole old capacity, including slots the owner has not assigned yet,
      // which is why the allocations above are value initialized.
      //
      // NOLINT below: the static analyzer does not model the initialization
      // performed by `new T[n]()`, so it reads every element of a heap array
      // as indeterminate no matter how it was allocated.
      const base::ssize copycount = newsize < VectorMax ? newsize : VectorMax;
      for (base::ssize index = 0; index < copycount; index++) {
        // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
        replacement[base::ToSize(index)] = Elements()[base::ToSize(index)];
      }
      if (IsAllocated) {
        delete[] Vector;
        Vector = nullptr;
      }
    }

    Vector = newptr;
    VectorMax = newsize;
    IsAllocated = Vector != nullptr && array.empty();
    return true;
  }
  // Resize to 0 is a failure. Use Clear() to explicitly deallocate.
  return false;
}

#endif  // CNC_RED_ALERT_RA_VECTOR_H_
