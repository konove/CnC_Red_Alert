#include "port/tokenizer.h"

#include <string_view>

#include "port/safe_string.h"

#include "absl/log/check.h"

namespace port {

Tokenizer::Tokenizer(char* text, const char* delimiters)
    : cursor_(MutableCString(text)), delimiters_(delimiters) {
  CHECK(text != nullptr);
  CHECK(delimiters != nullptr);
}

char* Tokenizer::Next() { return Next(delimiters_); }

char* Tokenizer::Next(const char* delimiters) {
  CHECK(delimiters != nullptr);
  const std::string_view separators(delimiters);
  while (cursor_.front() != '\0' &&
         separators.contains(cursor_.front())) {
    cursor_ = cursor_.subspan(1);
  }
  if (cursor_.front() == '\0') {
    return nullptr;
  }
  char* const start = cursor_.data();
  while (cursor_.front() != '\0' &&
         !separators.contains(cursor_.front())) {
    cursor_ = cursor_.subspan(1);
  }
  if (cursor_.front() != '\0') {
    cursor_.front() = '\0';
    cursor_ = cursor_.subspan(1);
  }
  return start;
}

}  // namespace port
