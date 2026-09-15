#include "port/tokenizer.h"

#include <cstring>

#include "absl/log/check.h"

namespace port {

Tokenizer::Tokenizer(char* text, const char* delimiters)
    : cursor_(text), delimiters_(delimiters) {
  CHECK(text != nullptr);
  CHECK(delimiters != nullptr);
}

char* Tokenizer::Next() { return Next(delimiters_); }

char* Tokenizer::Next(const char* delimiters) {
  char* const start = cursor_ + strspn(cursor_, delimiters);
  if (*start == '\0') {
    // Leave the cursor on the NUL so Remaining() stays valid and every later
    // call also finds nothing.
    cursor_ = start;
    return nullptr;
  }
  char* const end = start + strcspn(start, delimiters);
  if (*end == '\0') {
    cursor_ = end;
  } else {
    *end = '\0';
    cursor_ = end + 1;
  }
  return start;
}

}  // namespace port
