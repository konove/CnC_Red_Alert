#include "ra/vector_bool.h"

#include <cstring>

BooleanVectorClass::BooleanVectorClass(base::ssize size, unsigned char *array) {
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

bool BooleanVectorClass::operator==(const BooleanVectorClass &vector) {
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
bool BooleanVectorClass::Resize(base::ssize size) {
  Fixup();

  if (size > 0) {
    base::ssize old_byte_count = BitArray.Length();
    // Round up to 32 bits (4 bytes) for bit scan operations.
    bool success = BitArray.Resize(((size + (32 - 1)) / 32) * 4);

    BitCount = size;
    if (success && BitArray.Length() > old_byte_count) {
      // Zero-initialize new bytes (uninitialized after allocation).
      std::memset(&BitArray[old_byte_count], 0,
                  BitArray.Length() - old_byte_count);
    }
    return success;
  }

  Clear();
  return true;
}

// Syncs the cached Copy value with the bit array. Call with -1 before direct
// bit array manipulation. The [] operator uses this to simulate array access
// even though values are packed into bits.
void BooleanVectorClass::Fixup(base::ssize index) const {
  if (index < 0 || index >= BitCount) {
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
    ((base::ssize &)LastIndex) = index;
  }
}
