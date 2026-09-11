// Checked integer conversion for configuration and protocol text.
#ifndef CNC_RED_ALERT_TECH_NUMBER_PARSE_H_
#define CNC_RED_ALERT_TECH_NUMBER_PARSE_H_

#include <cstdint>
#include <optional>
#include <string_view>

#include "absl/strings/numbers.h"

namespace tech {

// Parses a complete decimal integer, allowing a sign and surrounding ASCII
// whitespace. Returns nullopt for malformed input or values outside T's range.
// Example: ParseInteger<int>("42").value_or(default_value).
template <typename T>
std::optional<T> ParseInteger(std::string_view text) {
  T value{};
  if (!absl::SimpleAtoi(text, &value)) {
    return std::nullopt;
  }
  return value;
}

// As above, accepting nullable tokens returned by legacy tokenizers.
template <typename T>
std::optional<T> ParseInteger(const char* text) {
  if (text == nullptr) {
    return std::nullopt;
  }
  return ParseInteger<T>(std::string_view{text});
}

// Parses a complete hexadecimal integer, optionally prefixed by 0x, with a
// sign and surrounding ASCII whitespace. Returns nullopt on syntax/range
// errors.
template <typename T>
std::optional<T> ParseHex(std::string_view text) {
  T value{};
  if (!absl::SimpleHexAtoi(text, &value)) {
    return std::nullopt;
  }
  return value;
}

// As above, accepting nullable tokens returned by legacy tokenizers.
template <typename T>
std::optional<T> ParseHex(const char* text) {
  if (text == nullptr) {
    return std::nullopt;
  }
  return ParseHex<T>(std::string_view{text});
}

// Parses a decimal 32-bit bit pattern, accepting both unsigned decimal and the
// signed decimal spelling emitted by legacy coordinate writers.
std::optional<uint32_t> ParseDecimalBits(std::string_view text);

// Parses RA INI decimal, $hex, and hexH forms. Hex values retain all 32 bits,
// including the sign bit used by legacy bit masks. Empty/invalid text is
// rejected.
std::optional<int> ParseIniInteger(std::string_view text);

}  // namespace tech

#endif  // CNC_RED_ALERT_TECH_NUMBER_PARSE_H_
