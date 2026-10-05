#include "engine/file/string_table_test_util.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "base/buffer.h"

std::vector<std::byte> MakeStringTable(
    const std::span<const std::string> strings) {
  // The offsets come first, so the first one doubles as the entry count times
  // two, which is how Extract_String() learns the count.
  std::vector<std::byte> table(strings.size() * sizeof(uint16_t), std::byte{0});
  size_t entry = 0;
  for (const std::string& text : strings) {
    CHECK_LE(table.size(), size_t{UINT16_MAX})
        << "the table has outgrown its 16-bit offsets";
    const auto offset = static_cast<uint16_t>(table.size());
    base::CopyBytes(std::span(table).subspan(entry * sizeof(uint16_t)),
                    base::ObjectBytes(offset), sizeof(offset));
    ++entry;

    for (const char c : text) {
      if (c == '\0') {
        break;
      }
      table.push_back(static_cast<std::byte>(c));
    }
    table.push_back(std::byte{0});
  }
  return table;
}
