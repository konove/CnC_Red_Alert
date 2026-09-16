#include "sdllib/string_table.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "base/buffer.h"
#include "base/numeric.h"

std::string_view Extract_String(const std::span<const std::byte> data,
                                const int index) {
  if (data.size() < sizeof(uint16_t) || index < 0) {
    return {};
  }

  // Data format: array of uint16_t offsets followed by null-terminated strings.
  // First value is num_strings * sizeof(uint16_t) due to a quirk in the writer.
  uint16_t num_strings_x2 = 0;
  base::CopyBytes(base::ObjectBytes(num_strings_x2),
                  std::as_bytes(std::span(data)), sizeof(num_strings_x2));
  const int num_strings = num_strings_x2 / 2;

  // Don't index past the end (might happen if expansion files missing).
  if (index >= num_strings || num_strings_x2 > data.size()) {
    return {};
  }

  uint16_t string_offset = 0;
  base::CopyBytes(base::ObjectBytes(string_offset),
                  data.subspan(base::ToSize(index) * sizeof(uint16_t)),
                  sizeof(string_offset));
  if (string_offset >= data.size()) {
    return {};
  }
  const auto text = data.subspan(string_offset);
  const auto end = std::ranges::find(text, std::byte{});
  if (end == text.end()) {
    return {};
  }
  // char may alias every object representation; range is bounded above.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return {reinterpret_cast<const char*>(text.data()),
          static_cast<size_t>(end - text.begin())};
}
