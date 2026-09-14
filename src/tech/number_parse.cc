// Integer formats used by legacy INI files.
#include "tech/number_parse.h"

#include <cstdint>
#include <optional>
#include <string_view>

#include "absl/strings/ascii.h"

namespace tech {

std::optional<uint32_t> ParseDecimalBits(std::string_view text) {
  text = absl::StripAsciiWhitespace(text);
  if (!text.empty() && text.front() == '-') {
    if (const auto value = ParseInteger<int32_t>(text)) {
      return static_cast<uint32_t>(*value);
    }
    return std::nullopt;
  }
  return ParseInteger<uint32_t>(text);
}

std::optional<int> ParseIniInteger(std::string_view text) {
  text = absl::StripAsciiWhitespace(text);
  if (text.empty()) {
    return std::nullopt;
  }
  bool hex = false;
  if (text.front() == '$') {
    text.remove_prefix(1);
    hex = true;
  } else if (text.back() == 'h' || text.back() == 'H') {
    text.remove_suffix(1);
    hex = true;
  }
  if (hex) {
    if (const auto value = ParseHex<uint32_t>(text)) {
      return static_cast<int>(*value);
    }
    return std::nullopt;
  }
  return ParseInteger<int>(text);
}

}  // namespace tech
