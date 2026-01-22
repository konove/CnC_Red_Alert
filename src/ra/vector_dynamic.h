#ifndef CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_
#define CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_

#include <cstring>
#include <type_traits>

#include "base/types.h"
#include "ra/vector.h"

// Dynamic array that supports adding/deleting elements. Elements are packed
// at the beginning (no holes). Auto-grows when full if GrowthStep > 0.
// Requires T to have assignment and equality operators.
template <typename T>
class DynamicVectorClass : public VectorClass<T> {
 public:
  DynamicVectorClass(base::ssize size = 0, const T* array = nullptr);

  // Change maximum size of vector.
  bool Resize(base::ssize newsize, const T* array = nullptr) override;

  // Resets and frees the vector array.
  void Clear() override {
    ActiveCount = 0;
    VectorClass<T>::Clear();
  }

  // Fetch number of "allocated" vector objects.
  base::ssize Count() const { return ActiveCount; }

  // Add object to vector (growing as necessary).
  bool Add(const T& object);
  bool Add_Head(const T& object);

  // Delete object by value. Constrained to avoid ambiguity with Delete(index)
  // when T is a pointer type and an integer literal like 0 is passed.
  template <typename U>
    requires(std::is_same_v<std::decay_t<U>, T>)
  bool Delete(const U& object) {
    return Delete(ID(object));
  }

  // Delete object at this vector index.
  bool Delete(base::ssize index);

  // Deletes all objects in the vector.
  void Delete_All() { ActiveCount = 0; }

  // Set amount that vector grows by.
  base::ssize Set_Growth_Step(base::ssize step) { return GrowthStep = step; }

  // Fetch current growth step rate.
  base::ssize Growth_Step() const { return GrowthStep; }

  base::ssize ID(const T* ptr) override { return VectorClass<T>::ID(ptr); }
  base::ssize ID(const T& ptr) override;

 protected:
  // Number of valid elements (may be less than capacity).
  base::ssize ActiveCount;

  // Elements to add when growing (0 disables auto-grow).
  base::ssize GrowthStep;
};

// Implementation details only below here

template <class T>
DynamicVectorClass<T>::DynamicVectorClass(base::ssize size, T const* array)
    : VectorClass<T>(size, array) {
  GrowthStep = 10;
  ActiveCount = 0;
}

// Resizes capacity. Truncates ActiveCount if new size is smaller.
template <class T>
bool DynamicVectorClass<T>::Resize(base::ssize newsize, T const* array) {
  if (VectorClass<T>::Resize(newsize, array)) {
    if (this->Length() < ActiveCount) {
      ActiveCount = this->Length();
    }
    return true;
  }
  return false;
}

// Appends object to end. Auto-grows if needed and allowed. Returns true on
// success.
template <class T>
bool DynamicVectorClass<T>::Add(T const& object) {
  if (ActiveCount >= this->Length()) {
    if ((this->IsAllocated || !this->VectorMax) && GrowthStep > 0) {
      if (!Resize(this->Length() + GrowthStep)) {
        return false;
      }
    } else {
      return false;  // Can't grow.
    }
  }
  (*this)[ActiveCount++] = object;
  return true;
}

// Inserts object at index 0, shifting existing elements. Returns true on
// success.
template <class T>
bool DynamicVectorClass<T>::Add_Head(T const& object) {
  if (ActiveCount >= this->Length()) {
    if ((this->IsAllocated || !this->VectorMax) && GrowthStep > 0) {
      if (!Resize(this->Length() + GrowthStep)) {
        return false;
      }
    } else {
      return false;  // Can't grow.
    }
  }
  if (ActiveCount) {
    // Shift elements to make room. NOLINT: sizeof(T) is intentional for raw
    // move.
    std::memmove(
        &(*this)[1], &(*this)[0],
        ActiveCount * sizeof(T));  // NOLINT(bugprone-sizeof-expression)
  }
  (*this)[0] = object;
  ActiveCount++;
  return true;
}

// Removes element at index, shifting subsequent elements down. Returns false if
// out of bounds.
template <class T>
bool DynamicVectorClass<T>::Delete(base::ssize index) {
  if (index >= 0 && index < ActiveCount) {
    ActiveCount--;
    // Use assignment (not memcpy) to properly handle class objects.
    for (base::ssize i = index; i < ActiveCount; i++) {
      (*this)[i] = (*this)[i + 1];
    }
    return true;
  }
  return false;
}

// Finds the index of an object by value. Only searches active elements,
// unlike the base class version which searches the entire allocated array.
// Returns -1 if not found.
template <typename T>
base::ssize DynamicVectorClass<T>::ID(const T& ptr) {
  for (base::ssize index = 0; index < Count(); index++) {
    if ((*this)[index] == ptr) {
      return index;
    }
  }
  return -1;
}

#endif  // CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_
