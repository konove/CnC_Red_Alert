#include "port/safe_string.h"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>

namespace port {

void SafeCopy(std::span<char> dest, const char* src) {
  if (dest.empty()) {
    return;
  }
  if (src == nullptr) {
    dest.front() = '\0';
    return;
  }
  SafeCopy(dest, std::string_view(src));
}

void SafeCopy(std::span<char> dest, std::string_view src) {
  if (dest.empty()) {
    return;
  }
  const auto copied = src.substr(0, dest.size() - 1);
  std::ranges::copy(copied, dest.begin());
  std::ranges::fill(dest.subspan(copied.size()), '\0');
}

void SafeAppend(std::span<char> dest, const char* src) {
  if (dest.empty() || src == nullptr) {
    return;
  }
  SafeAppend(dest, std::string_view(src));
}

void SafeAppend(std::span<char> dest, std::string_view src) {
  if (dest.empty()) {
    return;
  }
  const auto end = std::ranges::find(dest, '\0');
  const auto used = static_cast<std::size_t>(end - dest.begin());
  if (used >= dest.size() - 1) {
    dest.back() = '\0';
    return;
  }
  SafeCopy(dest.subspan(used), src);
}

std::span<char> MutableCString(char* text) {
  if (text == nullptr) {
    return {};
  }
  // The C-string contract establishes readable/writable storage through its
  // terminator. The extent is derived here, never supplied independently.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  return {text, std::string_view(text).size() + 1};
}

char* CloneString(const char* src) {
  if (src == nullptr) {
    return nullptr;
  }
  const std::size_t size = std::string_view(src).size() + 1;
  auto* const dest = new char[size];
  // This allocation is exactly size elements; the owner establishes the bound.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  SafeCopy(std::span(dest, size), src);
  return dest;
}

}  // namespace port
