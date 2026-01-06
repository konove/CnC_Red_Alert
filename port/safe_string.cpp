#include "port/safe_string.h"

#include <algorithm>
#include <cstring>

namespace port {

void SafeCopy(char* dest, const char* src, const size_t dest_size) {
  if (!dest || dest_size == 0) return;

  if (!src) {
    dest[0] = '\0';
    return;
  }

  // Copy at most dest_size - 1 bytes.
  std::strncpy(dest, src, dest_size - 1);

  // Force null termination at the very end.
  dest[dest_size - 1] = '\0';
}

void SafeAppend(char* dest, const char* src, const size_t dest_size) {
  if (!dest || dest_size == 0 || !src) return;

  // Find where the current string ends.
  size_t current_len = 0;
  while (current_len < dest_size && dest[current_len] != '\0') {
    current_len++;
  }

  // If buffer is already full (or corrupted/missing null), do nothing.
  if (current_len >= dest_size - 1) {
    dest[dest_size - 1] = '\0';
    return;
  }

  // Calculate how much space is left.
  const size_t remaining = dest_size - current_len - 1;

  // Copy the chunk.
  std::strncpy(&dest[current_len], src, remaining);

  // Force null termination at the end of the buffer.
  dest[dest_size - 1] = '\0';
}

char* CloneString(const char* src) {
  if (!src) return nullptr;

  // +1 for the null terminator.
  const size_t len = std::strlen(src) + 1;
  // Caller must delete[].
  const auto dest = new char[len];

  // We use memcpy here because we know the exact size and allocated it
  // ourselves. There is no risk of overflow.
  std::memcpy(dest, src, len);

  return dest;
}

}  // namespace port