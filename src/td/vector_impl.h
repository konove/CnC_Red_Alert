// Implementation shared by production vector instantiations and heap tests.
#ifndef CNC_RED_ALERT_TD_VECTOR_IMPL_H_
#define CNC_RED_ALERT_TD_VECTOR_IMPL_H_

#include <algorithm>
#include <new>
#include <utility>

#include "base/numeric.h"
#include "td/vector.h"

/***********************************************************************************************
 * VectorClass<T>::VectorClass -- Constructor for vector class. *
 *                                                                                             *
 *    This constructor for the vector class is passed the initial size of the
 *vector and an    * optional pointer to a preallocated block of memory that the
 *vector will be placed in.    * If this optional pointer is NULL (or not
 *provided), then the vector is allocated out     * of free store (with the
 *"new" operator).                                                 *
 *                                                                                             *
 * INPUT:   size  -- The number of elements to initialize this vector to. *
 *                                                                                             *
 *          array -- Optional pointer to a previously allocated memory block to
 *hold the       * vector. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
VectorClass<T>::VectorClass(base::ssize size, std::span<T> array)
    : Vector(nullptr), VectorMax(size) {
  CHECK_GE(size, 0);
  CHECK(array.empty() || base::ToSize(size) <= array.size());
  /*
  **	Allocate the vector. The default constructor will be called for every
  **	object in this vector.
  */
  if (size) {
    if (!array.empty()) {
      Vector = new (static_cast<void*>(array.data())) T[base::ToSize(size)];
    } else {
      Vector = new T[base::ToSize(size)];
      IsAllocated = true;
    }
  }
}

/***********************************************************************************************
 * VectorClass<T>::~VectorClass -- Default destructor for vector class. *
 *                                                                                             *
 *    This is the default destructor for the vector class. It will deallocate
 *any memory       * that it may have allocated. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
VectorClass<T>::~VectorClass() {
  VectorClass<T>::Clear();
}

/***********************************************************************************************
 * VectorClass<T>::VectorClass -- Copy constructor for vector object. *
 *                                                                                             *
 *    This is the copy constructor for the vector class. It will duplicate the
 *provided        * vector into the new vector being created. *
 *                                                                                             *
 * INPUT:   vector   -- Reference to the vector to use as a copy. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
VectorClass<T>::VectorClass(const VectorClass<T>& vector) : Vector(nullptr) {
  if (this != &vector) {
    Copy_From(vector);
  }
}

template <class T>
void VectorClass<T>::Copy_From(const VectorClass<T>& vector) {
  VectorMax = vector.Length();
  if (VectorMax) {
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

/***********************************************************************************************
 * VectorClass<T>::operator = -- The assignment operator. *
 *                                                                                             *
 *    This the the assignment operator for vector objects. It will alter the
 *existing lvalue   * vector to duplicate the rvalue one. *
 *                                                                                             *
 * INPUT:   vector   -- The rvalue vector to copy into the lvalue one. *
 *                                                                                             *
 * OUTPUT:  Returns with reference to the newly copied vector. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
VectorClass<T>& VectorClass<T>::operator=(const VectorClass<T>& vector) {
  if (this == &vector) {
    return *this;
  }
  Clear();
  Copy_From(vector);
  return *this;
}

/***********************************************************************************************
 * VectorClass<T>::operator == -- Equality operator for vector objects. *
 *                                                                                             *
 *    This operator compares two vectors for equality. It does this by
 *performing an object    * by object comparison between the two vectors. *
 *                                                                                             *
 * INPUT:   vector   -- The right vector expression. *
 *                                                                                             *
 * OUTPUT:  bool; Are the two vectors essentially equal? (do they contain
 *comparable elements  * in the same order?) *
 *                                                                                             *
 * WARNINGS:   The equality operator must exist for the objects that this vector
 *contains.     *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
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

/***********************************************************************************************
 * VectorClass<T>::ID -- Pointer based conversion to index number. *
 *                                                                                             *
 *    Use this routine to convert a pointer to an element in the vector back
 *into the index    * number of that object. This routine ONLY works with actual
 *pointers to object within     * the vector. For "equivalent" object index
 *number (such as with similar integral values)  * then use the "by value" index
 *number ID function.                                        *
 *                                                                                             *
 * INPUT:   pointer  -- Pointer to an actual object in the vector. *
 *                                                                                             *
 * OUTPUT:  Returns with the index number for the object pointed to by the
 *parameter.          *
 *                                                                                             *
 * WARNINGS:   This routine is only valid for actual pointers to object that
 *exist within      * the vector. All other object pointers will yield undefined
 *results.             *
 *                                                                                             *
 * HISTORY: * 03/13/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
int VectorClass<T>::ID(const T* ptr) {
  return static_cast<int>(ptr - &(*this)[0]);
}

/***********************************************************************************************
 * VectorClass<T>::ID -- Finds object ID based on value. *
 *                                                                                             *
 *    Use this routine to find the index value of an object with equivalent
 *value in the       * vector. Typical use of this would be for integral types.
 **
 *                                                                                             *
 * INPUT:   object   -- Reference to the object that is to be looked up in the
 *vector.         *
 *                                                                                             *
 * OUTPUT:  Returns with the index number of the object that is equivalent to
 *the one          * specified. If no matching value could be found then -1 is
 *returned.                *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/13/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
int VectorClass<T>::ID(const T& object) {
  for (base::ssize index = 0; index < VectorMax; index++) {
    if ((*this)[index] == object) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

/***********************************************************************************************
 * VectorClass<T>::Clear -- Frees and clears the vector. *
 *                                                                                             *
 *    Use this routine to reset the vector to an empty (non-allocated) state. A
 *vector will    * free all allocated memory when this routine is called. In
 *order for the vector to be     * useful after this point, the Resize function
 *must be called to give it element space.    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
void VectorClass<T>::Clear() {
  if (Vector && IsAllocated) {
    delete[] Vector;
    Vector = nullptr;
  }
  IsAllocated = false;
  VectorMax = 0;
}

/***********************************************************************************************
 * VectorClass<T>::Resize -- Changes the size of the vector. *
 *                                                                                             *
 *    This routine is used to change the size (usually to increase) the size of
 *a vector. This * is the only way to increase the vector's working room (number
 *of elements).              *
 *                                                                                             *
 * INPUT:   newsize  -- The desired size of the vector. *
 *                                                                                             *
 *          array    -- Optional pointer to a previously allocated memory block
 *that the       * array will be located in. If this parameter is not supplied,
 *then      * the array will be allocated from free store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the array resized successfully? *
 *                                                                                             *
 * WARNINGS:   Failure to succeed could be the result of running out of memory.
 **
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool VectorClass<T>::Resize(base::ssize newsize, std::span<T> array) {
  CHECK_GE(newsize, 0);
  CHECK(array.empty() || base::ToSize(newsize) <= array.size());
  if (newsize) {
    /*
    **	Allocate a new vector of the size specified. The default constructor
    **	will be called for every object in this vector.
    */
    T* newptr = nullptr;
    if (array.empty()) {
      newptr = new T[base::ToSize(newsize)];
    } else {
      newptr = new (static_cast<void*>(array.data())) T[base::ToSize(newsize)];
    }
    if (!newptr) {
      return false;
    }
    // newptr is the newsize-element allocation or the checked external span
    // above. NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<T> replacement(newptr, base::ToSize(newsize));

    /*
    **	If there is an old vector, then it must be copied (as much as is
    *feasable) *	to the new vector.
    */
    if (Vector) {
      /*
      **	Copy as much of the old vector into the new vector as possible.
      *This *	presumes that there is a functional assignment operator for each
      **	of the objects in the vector.
      */
      const int copycount = static_cast<int>(
          std::cmp_less(newsize, VectorMax) ? newsize : VectorMax);
      for (int index = 0; index < copycount; index++) {
        replacement[base::ToSize(index)] = Elements()[base::ToSize(index)];
      }

      /*
      **	Delete the old vector. This might cause the destructors to be
      *called *	for all of the old elements. This makes the implementation of
      *suitable *	assignment operator very important. The default
      *assigment operator will *	only work for the simplist of objects.
      */
      if (IsAllocated) {
        delete[] Vector;
        Vector = nullptr;
      }
    }

    /*
    **	Assign the new vector data to this class.
    */
    Vector = newptr;
    VectorMax = newsize;
    IsAllocated = Vector != nullptr && array.empty();

  } else {
    /*
    **	Resizing to zero is the same as clearing the vector.
    */
    Clear();
  }
  return true;
}

/***********************************************************************************************
 * DynamicVectorClass<T>::DynamicVectorClass -- Constructor for dynamic vector.
 **
 *                                                                                             *
 *    This is the normal constructor for the dynamic vector class. It is similar
 *to the normal * vector class constructor. The vector is initialized to contain
 *the number of elements    * specified in the "size" parameter. The memory is
 *allocated from free store unless the    * optional array parameter is
 *provided. In this case it will place the vector at the       * memory location
 *specified.                                                               *
 *                                                                                             *
 * INPUT:   size  -- The maximum number of objects allowed in this vector. *
 *                                                                                             *
 *          array -- Optional pointer to the memory area to place the vector at.
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
DynamicVectorClass<T>::DynamicVectorClass(base::ssize size, std::span<T> array)
    : VectorClass<T>(size, array) {}

/***********************************************************************************************
 * DynamicVectorClass<T>::Resize -- Changes the size of a dynamic vector. *
 *                                                                                             *
 *    Use this routine to change the size of the vector. The size changed is the
 *maximum       * number of allocated objects within this vector. If a memory
 *buffer is provided, then     * the vector will be located there. Otherwise,
 *the memory will be allocated out of free    * store. *
 *                                                                                             *
 * INPUT:   newsize  -- The desired maximum size of this vector. *
 *                                                                                             *
 *          array    -- Optional pointer to a previosly allocated memory array.
 **
 *                                                                                             *
 * OUTPUT:  bool; Was vector successfully resized according to specifications? *
 *                                                                                             *
 * WARNINGS:   Failure to resize the vector could be the result of lack of free
 *store.         *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool DynamicVectorClass<T>::Resize(base::ssize newsize, std::span<T> array) {
  if (VectorClass<T>::Resize(newsize, array)) {
    if (this->Length() < ActiveCount) {
      ActiveCount = this->Length();
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * DynamicVectorClass<T>::ID -- Find matching value in the dynamic vector. *
 *                                                                                             *
 *    Use this routine to find a matching object (by value) in the vector.
 *Unlike the base     * class ID function of similar name, this one restricts
 *the scan to the current number     * of valid objects. *
 *                                                                                             *
 * INPUT:   object   -- A reference to the object that a match is to be found in
 *the           * vector. *
 *                                                                                             *
 * OUTPUT:  Returns with the index number of the object that is equivalent to
 *the one          * specified. If no equivalent object could be found then -1
 *is returned.             *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/13/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
int DynamicVectorClass<T>::ID(const T& ptr) {
  for (base::ssize index = 0; index < Count(); index++) {
    if ((*this)[index] == ptr) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

/***********************************************************************************************
 * DynamicVectorClass<T>::Add -- Add an element to the vector. *
 *                                                                                             *
 *    Use this routine to add an element to the vector. The vector will
 *automatically be       * resized to accomodate the new element IF the vector
 *was allocated previosly and the      * growth rate is not zero. *
 *                                                                                             *
 * INPUT:   object   -- Reference to the object that will be added to the
 *vector.              *
 *                                                                                             *
 * OUTPUT:  bool; Was the object added successfully? If so, the object is added
 *to the end     * of the vector. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool DynamicVectorClass<T>::Add(const T& object) {
  if (ActiveCount >= this->Length()) {
    if ((this->IsAllocated || !this->VectorMax) && GrowthStep > 0) {
      if (!Resize(this->Length() + GrowthStep)) {
        /*
        **	Failure to increase the size of the vector is an error
        *condition. *	Return with the error flag.
        */
        return false;
      }
      // Verify resize actually allocated space.
      if (ActiveCount >= this->Length()) {
        return false;
      }
    } else {
      /*
      **	Increasing the size of this vector is not allowed! Bail this
      **	routine with the error code.
      */
      return false;
    }
  }

  /*
  **	There is room for the new object now. Add it to the end of the object
  *vector.
  */
  (*this)[ActiveCount++] = object;
  return true;
}

template <class T>
bool DynamicVectorClass<T>::Add_Head(const T& object) {
  if (ActiveCount >= this->Length()) {
    if ((this->IsAllocated || !this->VectorMax) && GrowthStep > 0) {
      if (!Resize(this->Length() + GrowthStep)) {
        /*
        **	Failure to increase the size of the vector is an error
        *condition. *	Return with the error flag.
        */
        return false;
      }
      // Verify resize actually allocated space.
      if (ActiveCount >= this->Length()) {
        return false;
      }
    } else {
      /*
      **	Increasing the size of this vector is not allowed! Bail this
      **	routine with the error code.
      */
      return false;
    }
  }

  /*
  **	There is room for the new object now. Add it to the end of the object
  *vector.
  */
  // Shift by assignment rather than a raw byte move, both so that a non-trivial
  // T is handled correctly (matching Delete()) and to avoid the void* round
  // trip. For a trivially copyable T this still compiles down to a memmove.
  const auto elements = this->Elements();
  std::move_backward(elements.begin(), elements.begin() + ActiveCount,
                     elements.begin() + ActiveCount + 1);
  (*this)[0] = object;
  ActiveCount++;
  //	(*this)[ActiveCount++] = object;
  return true;
}

/***********************************************************************************************
 * DynamicVectorClass<T>::Delete -- Remove the specified object from the vector.
 **
 *                                                                                             *
 *    This routine will delete the object referenced from the vector. All
 *objects in the       * vector that follow the one deleted will be moved "down"
 *to fill the hole.                *
 *                                                                                             *
 * INPUT:   object   -- Reference to the object in this vector that is to be
 *deleted.          *
 *                                                                                             *
 * OUTPUT:  bool; Was the object deleted successfully? This should always be
 *true.             *
 *                                                                                             *
 * WARNINGS:   Do no pass a reference to an object that is NOT part of this
 *vector. The        * results of this are undefined and probably catastrophic.
 **
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool DynamicVectorClass<T>::Delete(const T& object) {
  const int index = ID(object);
  if (index != -1) {
    return Delete(index);
  }
  return false;
}

// workaround for DynamicVectorClass<int>, nobody call this please
//

/***********************************************************************************************
 * DynamicVectorClass<T>::Delete -- Deletes the specified index from the vector.
 **
 *                                                                                             *
 *    Use this routine to delete the object at the specified index from the
 *objects in the     * vector. This routine will move all the remaining objects
 *"down" in order to fill the     * hole. *
 *                                                                                             *
 * INPUT:   index -- The index number of the object in the vector that is to be
 *deleted.       *
 *                                                                                             *
 * OUTPUT:  bool; Was the object index deleted successfully? Failure might mean
 *that the index * specified was out of bounds. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/10/1995 JLB : Created. *
 *=============================================================================================*/
template <class T>
bool DynamicVectorClass<T>::Delete(int index) {
  if (std::cmp_less(index, ActiveCount)) {
    ActiveCount--;

    /*
    **	If there are any objects past the index that was deleted, copy those
    **	objects down in order to fill the hole. A simple memory copy is
    **	not sufficient since the vector could contain class objects that
    **	need to use the assignment operator for movement.
    */
    for (int i = index; i < ActiveCount; i++) {
      (*this)[i] = (*this)[i + 1];
    }
    return true;
  }
  return false;
}

#endif  // CNC_RED_ALERT_TD_VECTOR_IMPL_H_
