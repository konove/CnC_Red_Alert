#include "dipthong.h"

char *Extract_String(void const *data, int string) {
  unsigned short int const *ptr;

  if (!data || string < 0) return (nullptr);

  ptr = (unsigned short int const *)data;

  // assume offset of first string is end of index table
  int numstrings = ptr[0] / 2;

  // don't index past the end (might happen if expansion files missing)
  if (string >= numstrings) return nullptr;

  return (((char *)data) + ptr[string]);
}