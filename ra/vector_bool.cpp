#include "ra/vector_bool.h"

#include <cstring>

BooleanVectorClass::BooleanVectorClass(unsigned size, unsigned char *array) {
  BitArray.Resize(((size + (8 - 1)) / 8), array);
  LastIndex = -1;
  BitCount = size;
}

BooleanVectorClass::BooleanVectorClass(BooleanVectorClass const &vector) {
  LastIndex = -1;
  *this = vector;
}

BooleanVectorClass &BooleanVectorClass::operator=(
    BooleanVectorClass const &vector) {
  Fixup();
  Copy = vector.Copy;
  LastIndex = vector.LastIndex;
  BitArray = vector.BitArray;
  BitCount = vector.BitCount;
  return *this;
}

int BooleanVectorClass::operator==(const BooleanVectorClass &vector) {
  Fixup(LastIndex);
  return BitCount == vector.BitCount && BitArray == vector.BitArray;
}

// Sets all bits to false.
void BooleanVectorClass::Reset(void) {
  LastIndex = -1;
  if (BitArray.Length()) {
    std::memset(&BitArray[0], '\0', BitArray.Length());
  }
}

// Sets all bits to true.
void BooleanVectorClass::Set(void) {
  LastIndex = -1;
  if (BitArray.Length()) {
    std::memset(&BitArray[0], '\xFF', BitArray.Length());
  }
}

// Frees memory. Must call Resize() before using again.
void BooleanVectorClass::Clear(void) {
  Fixup();
  BitCount = 0;
  BitArray.Clear();
}

// Changes capacity. New bits are initialized to false.
int BooleanVectorClass::Resize(unsigned size) {
  Fixup();

  if (size) {
    int oldsize = BitCount;
    // Round up to 32 bits (4 bytes) for bit scan operations.
    int success = BitArray.Resize(((size + (32 - 1)) / 32) * 4);

    BitCount = size;
    if (success && BitArray.Length() && oldsize < size) {
      // Clear new bits (no default constructor for packed bits).
      for (int index = oldsize; index < size; index++) {
        (*this)[index] = 0;
      }
    }
    return success;
  }

  Clear();
  return true;
}

// Syncs the cached Copy value with the bit array. Call with -1 before direct
// bit array manipulation. The [] operator uses this to simulate array access
// even though values are packed into bits.
void BooleanVectorClass::Fixup(int index) const {
  if ((unsigned)index >= BitCount) {
    index = -1;
  }

  if (index != LastIndex) {
    // Write back any changes to the previously cached element.
    if (LastIndex != -1) {
      Set_Bit((void *)&BitArray[0], LastIndex, Copy);
    }
    // Load the new element into the cache.
    if (index != -1) {
      ((unsigned char &)Copy) = Get_Bit(&BitArray[0], index);
    }
    ((int &)LastIndex) = index;
  }
}
