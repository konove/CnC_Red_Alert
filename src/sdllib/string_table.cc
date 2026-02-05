#include "sdllib/string_table.h"

#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

std::string_view Extract_String(const std::span<const std::byte> data,
                                const int index) {
  if (data.empty() || index < 0) {
    return {};
  }

  // Data format: array of uint16_t offsets followed by null-terminated strings.
  // First value is num_strings * sizeof(uint16_t) due to a quirk in the writer.
  uint16_t num_strings_x2;
  std::memcpy(&num_strings_x2, data.data(), sizeof(num_strings_x2));
  const int num_strings = num_strings_x2 / 2;

  // Don't index past the end (might happen if expansion files missing).
  if (index >= num_strings) {
    return {};
  }

  uint16_t string_offset;
  std::memcpy(&string_offset, data.data() + index * sizeof(uint16_t),
              sizeof(string_offset));

  // char is explicitly allowed to alias any type per the standard.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const char*>(data.data() + string_offset);
}
