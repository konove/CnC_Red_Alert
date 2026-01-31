#include "sdllib/include/dipthong.h"

char* Extract_String(const void* data, int string) {
  const unsigned short int* ptr;

  if (!data || string < 0) return nullptr;

  ptr = static_cast<const unsigned short int*>(data);

  // assume offset of first string is end of index table
  int numstrings = ptr[0] / 2;

  // don't index past the end (might happen if expansion files missing)
  if (string >= numstrings) return nullptr;

  return (char*)data + ptr[string];
}
