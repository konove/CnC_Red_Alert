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

#ifndef VECTOR_H
#define VECTOR_H

#include <new>  // IWYU pragma: keep

#include "base/types.h"
#include "ra/defines.h"  // IWYU pragma: keep
#include "ra/egos.h"     // IWYU pragma: keep
#include "tech/noinit.h"

// Resizable array of arbitrary objects. Not optimized for integral types -
// consider a specialized version for char/int if performance is critical.
template <typename T>
class VectorClass {
 public:
  VectorClass(NoInitClass const&) {}
  VectorClass(base::ssize size = 0, const T* array = nullptr);
  VectorClass(const VectorClass&);  // Copy constructor.
  virtual ~VectorClass();

  T& operator[](base::ssize index) { return Vector[index]; }
  T const& operator[](base::ssize index) const { return Vector[index]; }
  virtual VectorClass& operator=(const VectorClass&);  // Assignment operator.
  virtual bool operator==(const VectorClass&) const;   // Equality operator.
  virtual bool Resize(base::ssize newsize, const T* array = nullptr);
  virtual void Clear();
  base::ssize Length() const { return VectorMax; }
  virtual base::ssize ID(const T* ptr);  // Pointer based identification.
  virtual base::ssize ID(const T& ptr);  // Value based identification.

 protected:
  T* Vector;                // Pointer to element array.
  base::ssize VectorMax;    // Maximum number of elements.
  bool IsAllocated : true;  // True if we own the memory and must delete it.
};

// Implementation details only below here

template <class T>
VectorClass<T>::VectorClass(base::ssize size, T const* array)
    : Vector(nullptr), VectorMax(size), IsAllocated(false) {
  if (size > 0) {
    if (array) {
      Vector =
          new ((void*)array) T[size];  // Placement new into provided buffer.
    } else {
      Vector = new T[size];
      IsAllocated = true;
    }
  }
}

template <class T>
VectorClass<T>::~VectorClass() {
  VectorClass<T>::Clear();
}

template <class T>
VectorClass<T>::VectorClass(VectorClass<T> const& vector)
    : Vector(nullptr), VectorMax(0), IsAllocated(false) {
  *this = vector;
}

template <class T>
VectorClass<T>& VectorClass<T>::operator=(VectorClass<T> const& vector) {
  if (this != &vector) {
    Clear();
    VectorMax = vector.Length();
    if (VectorMax > 0) {
      Vector = new T[VectorMax];
      if (Vector) {
        IsAllocated = true;
        for (base::ssize index = 0; index < VectorMax; index++) {
          Vector[index] = vector[index];
        }
      }
    } else {
      Vector = nullptr;
      IsAllocated = false;
    }
  }
  return (*this);
}

// Element-by-element comparison. Requires T to have operator!=.
template <class T>
bool VectorClass<T>::operator==(VectorClass<T> const& vector) const {
  if (VectorMax == vector.Length()) {
    for (base::ssize index = 0; index < VectorMax; index++) {
      if (Vector[index] != vector[index]) {
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
base::ssize VectorClass<T>::ID(T const* ptr) {
  return ptr - &(*this)[0];
}

// Finds index of first element equal to object. Returns -1 if not found.
template <class T>
base::ssize VectorClass<T>::ID(T const& object) {
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
bool VectorClass<T>::Resize(base::ssize newsize, T const* array) {
  if (newsize > 0) {
    T* newptr;
    if (!array) {
      newptr = new T[newsize];
    } else {
      newptr = new ((void*)array) T[newsize];
    }
    if (!newptr) {
      return false;
    }

    if (Vector) {
      // Copy existing elements (uses assignment operator).
      base::ssize copycount = (newsize < VectorMax) ? newsize : VectorMax;
      for (base::ssize index = 0; index < copycount; index++) {
        newptr[index] = Vector[index];
      }
      if (IsAllocated) {
        delete[] Vector;
        Vector = nullptr;
      }
    }

    Vector = newptr;
    VectorMax = newsize;
    IsAllocated = (Vector && !array);
    return true;
  }
  // Resize to 0 is a failure. Use Clear() to explicitly deallocate.
  return false;
}

#endif
