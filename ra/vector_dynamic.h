#ifndef CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_
#define CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_

#include <cstring>

#include "ra/vector.h"

// Dynamic array that supports adding/deleting elements. Elements are packed
// at the beginning (no holes). Auto-grows when full if GrowthStep > 0.
// Requires T to have assignment and equality operators.
template <typename T>
class DynamicVectorClass : public VectorClass<T> {
 public:
  DynamicVectorClass(unsigned size = 0, const T *array = nullptr);

  // Change maximum size of vector.
  bool Resize(unsigned newsize, const T *array = nullptr) override;

  // Resets and frees the vector array.
  void Clear() override {
    ActiveCount = 0;
    VectorClass<T>::Clear();
  }

  // Fetch number of "allocated" vector objects.
  size_t Count() const { return ActiveCount; }

  // Add object to vector (growing as necessary).
  bool Add(const T &object);
  bool Add_Head(const T &object);

  // Delete object just like this from vector.
  bool Delete(const T &object);

  // Delete object at this vector index.
  bool Delete(int index);

  // Deletes all objects in the vector.
  void Delete_All() { ActiveCount = 0; }

  // Set amount that vector grows by.
  int Set_Growth_Step(int step) { return GrowthStep = step; }

  // Fetch current growth step rate.
  int Growth_Step() const { return GrowthStep; }

  int ID(const T *ptr) override { return VectorClass<T>::ID(ptr); }
  int ID(const T &ptr) override;

 protected:
  size_t ActiveCount;  // Number of valid elements (may be less than capacity).
  int GrowthStep;      // Elements to add when growing (0 disables auto-grow).
};

// Implementation details only below here

template <class T>
DynamicVectorClass<T>::DynamicVectorClass(unsigned size, T const *array)
    : VectorClass<T>(size, array) {
  GrowthStep = 10;
  ActiveCount = 0;
}

// Resizes capacity. Truncates ActiveCount if new size is smaller.
template <class T>
bool DynamicVectorClass<T>::Resize(unsigned newsize, T const *array) {
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
bool DynamicVectorClass<T>::Add(T const &object) {
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
bool DynamicVectorClass<T>::Add_Head(T const &object) {
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

// Removes first occurrence of object by value. Returns true if found and
// deleted.
template <class T>
bool DynamicVectorClass<T>::Delete(T const &object) {
  return (Delete(ID(object)));
}

// Removes element at index, shifting subsequent elements down. Returns false if
// out of bounds.
template <class T>
bool DynamicVectorClass<T>::Delete(int index) {
  if ((unsigned)index < ActiveCount) {
    ActiveCount--;
    // Use assignment (not memcpy) to properly handle class objects.
    for (size_t i = index; i < ActiveCount; i++) {
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
int DynamicVectorClass<T>::ID(const T &ptr) {
  for (size_t index = 0; index < Count(); index++) {
    if ((*this)[index] == ptr) {
      return index;
    }
  }
  return -1;
}

#endif  // CNC_RED_ALERT_RA_VECTOR_DYNAMIC_H_
