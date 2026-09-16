#include "sdllib/string_table.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "base/buffer.h"
#include "base/numeric.h"

std::string_view Extract_String(const std::span<const std::byte> data,
                                const int index) {
  if (data.empty() || index < 0) {
    return {};
  }

  // Data format: array of uint16_t offsets followed by null-terminated strings.
  // First value is num_strings * sizeof(uint16_t) due to a quirk in the writer.
  uint16_t num_strings_x2 = 0;
  base::CopyBytes(base::ObjectBytes(num_strings_x2),
                  std::as_bytes(std::span(data)), sizeof(num_strings_x2));
  const int num_strings = num_strings_x2 / 2;

  // Don't index past the end (might happen if expansion files missing).
  if (index >= num_strings) {
    return {};
  }

  uint16_t string_offset = 0;
  std::memcpy(&string_offset,
              data.data() + (base::ToSize(index) * sizeof(uint16_t)),
              sizeof(string_offset));

  // char is explicitly allowed to alias any type per the standard.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const char*>(data.data() + string_offset);
}
