/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\vector.h_v   2.15   16 Oct 1995 16:47:38
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VECTOR.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 02/19/95 *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * VectorClass<T>::VectorClass -- Constructor for vector class. *
 *   VectorClass<T>::~VectorClass -- Default destructor for vector class. *
 *   VectorClass<T>::VectorClass -- Copy constructor for vector object. *
 *   VectorClass<T>::operator = -- The assignment operator. *
 *   VectorClass<T>::operator == -- Equality operator for vector objects. *
 *   VectorClass<T>::Clear -- Frees and clears the vector. *
 *   VectorClass<T>::Resize -- Changes the size of the vector. *
 *   DynamicVectorClass<T>::DynamicVectorClass -- Constructor for dynamic
 *vector.              * DynamicVectorClass<T>::Resize -- Changes the size of a
 *dynamic vector.                    * DynamicVectorClass<T>::Add -- Add an
 *element to the vector.                               *
 *   DynamicVectorClass<T>::Delete -- Remove the specified object from the
 *vector.             * DynamicVectorClass<T>::Delete -- Deletes the specified
 *index from the vector.             * VectorClass<T>::ID -- Pointer based
 *conversion to index number.                           * VectorClass<T>::ID --
 *Finds object ID based on value.                                     *
 *   DynamicVectorClass<T>::ID -- Find matching value in the dynamic vector. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef VECTOR_H
#define VECTOR_H

#include <cstddef>

#include "tech/noinit.h"

// IWYU pragma: no_include "td/cell.h"
// IWYU pragma: no_include "td/nodename.h"
// IWYU pragma: no_include "td/object.h"
// IWYU pragma: no_include "td/phone.h"

/**************************************************************************
**	This is a general purpose vector class. A vector is defined by this
**	class, as an array of arbitrary objects where the array can be
*dynamically *	sized. Because is deals with arbitrary object types, it can
*handle everything. *	As a result of this, it is not terribly efficient for
*integral objects (such *	as char or int). It will function correctly, but
*the copy constructor and *	equality operator could be highly optimized if
*the integral type were known. *	This efficiency can be implemented by
*deriving an integral vector template *	from this one in order to supply more
*efficient routines.
*/
template <class T>
class VectorClass {
 public:
  VectorClass(NoInitClass const&) {}
  VectorClass(unsigned size = 0, T const* array = nullptr);
  VectorClass(VectorClass const&);  // Copy constructor.
  virtual ~VectorClass();

  T& operator[](size_t index) { return Vector[index]; }
  T const& operator[](size_t index) const { return Vector[index]; }
  virtual VectorClass& operator=(VectorClass const&);
  virtual int operator==(VectorClass const&) const;
  virtual int Resize(unsigned newsize, T const* array = nullptr);
  virtual void Clear();
  unsigned Length() const { return VectorMax; }
  virtual int ID(T const* ptr);  // Pointer based identification.
  virtual int ID(T const& ptr);  // Value based identification.

 protected:
  /*
  **	This is a pointer to the allocated vector array of elements.
  */
  T* Vector;

  /*
  **	This is the maximum number of elements allowed in this vector.
  */
  unsigned VectorMax;

  /*
  **	Does the vector data pointer refer to memory that this class has
  *manually *	allocated? If so, then this class is responsible for deleting
  *it.
  */
  unsigned IsAllocated : 1;
};

/**************************************************************************
**	This derivative vector class adds the concept of adding and deleting
**	objects. The objects are packed to the beginning of the vector array.
**	If this is instantiated for a class object, then the assignment operator
**	and the equality operator must be supported. If the vector allocates its
**	own memory, then the vector can grow if it runs out of room adding
*items. *	The growth rate is controlled by setting the growth step rate. A
*growth *	step rate of zero disallows growing.
*/
template <class T>
class DynamicVectorClass : public VectorClass<T> {
 public:
  DynamicVectorClass(unsigned size = 0, T const* array = nullptr);

  // Change maximum size of vector.
  int Resize(unsigned newsize, T const* array = nullptr) override;

  // Resets and frees the vector array.
  void Clear() override {
    ActiveCount = 0;
    VectorClass<T>::Clear();
  }

  // Fetch number of "allocated" vector objects.
  size_t Count() const { return ActiveCount; }

  // Add object to vector (growing as necessary).
  int Add(T const& object);
  int Add_Head(T const& object);

  // Delete object just like this from vector.
  int Delete(T const& object);

  // Delete object at this vector index.
  int Delete(int index);

  // Deletes all objects in the vector.
  void Delete_All() { ActiveCount = 0; }

  // Set amount that vector grows by.
  int Set_Growth_Step(int step) { return GrowthStep = step; }

  // Fetch current growth step rate.
  int Growth_Step() { return GrowthStep; }

  int ID(T const* ptr) override { return VectorClass<T>::ID(ptr); }
  int ID(T const& ptr) override;

 protected:
  /*
  **	This is a count of the number of active objects in this
  **	vector. The memory array often times is bigger than this
  **	value.
  */
  size_t ActiveCount;

  /*
  **	If there is insufficient room in the vector array for a new
  **	object to be added, then the vector will grow by the number
  **	of objects specified by this value. This is controlled by
  **	the Set_Growth_Step() function.
  */
  int GrowthStep;
};

#endif
