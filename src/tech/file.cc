// File: out-of-line helpers of the File interface.

#include "tech/file.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"

std::vector<std::byte> File::ReadBytes(const base::ssize count) {
  std::vector<std::byte> bytes(base::ToSize(count));
  bytes.resize(base::ToSize(Read(bytes)));
  return bytes;
}

std::string File::ReadString(const base::ssize count) {
  std::string text(base::ToSize(count), '\0');
  text.resize(base::ToSize(Read(std::span(text))));
  return text;
}
