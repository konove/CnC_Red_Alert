#ifndef CNC_RED_ALERT_RA_VECTOR_BOOL_H_
#define CNC_RED_ALERT_RA_VECTOR_BOOL_H_

#include "ra/jshell.h"
#include "ra/vector.h"

// Bit-packed boolean array (8 values per byte, 87.5% memory savings).
// WARNING: Do not take pointers to elements - the [] operator returns a
// reference to a temporary copy that's only valid until the next access.
class BooleanVectorClass {
 public:
  BooleanVectorClass(unsigned size = 0, unsigned char *array = nullptr);
  BooleanVectorClass(const BooleanVectorClass &vector);

  // Assignment operator.
  BooleanVectorClass &operator=(const BooleanVectorClass &vector);

  // Equivalency operator.
  int operator==(const BooleanVectorClass &vector);

  // Fetch number of boolean objects in vector.
  int Length() const { return BitCount; }

  // Set all boolean values to false;
  void Reset();

  // Set all boolean values to true.
  void Set();

  // Resets vector to zero length (frees memory).
  void Clear();

  // Change size of this boolean vector.
  int Resize(unsigned size);

  // Fetch reference to specified index.
  const bool &operator[](int index) const {
    if (LastIndex != index) Fixup(index);
    return Copy;
  }

  bool &operator[](int index) {
    if (LastIndex != index) Fixup(index);
    return Copy;
  }

  // Quick check on boolean state.
  bool Is_True(int index) const {
    if (index == LastIndex) {
      return Copy;
    }
    return Get_Bit(&BitArray[0], index);
  }

  // Find first index that is false.
  int First_False() const {
    if (LastIndex != -1) {
      Fixup(-1);
    }

    int retval = First_False_Bit(&BitArray[0]);
    if (retval < BitCount) return retval;
    return -1;  // Not found.
  }

  // Find first index that is true.
  int First_True() const {
    if (LastIndex != -1) {
      Fixup(-1);
    }

    int retval = First_True_Bit(&BitArray[0]);
    if (retval < BitCount) return retval;
    return -1;  // Not found.
  }

 private:
  void Fixup(int index = -1) const;

  int BitCount;   // Number of boolean values (not necessarily multiple of 8).
  bool Copy;      // Cached copy of last accessed element for [] operator.
  int LastIndex;  // Index of element cached in Copy (-1 if none).
  VectorClass<unsigned char> BitArray;  // Packed bit storage.
};

#endif  // CNC_RED_ALERT_RA_VECTOR_BOOL_H_
