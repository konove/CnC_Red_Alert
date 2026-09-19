// Checked integer conversion for configuration and protocol text.
#ifndef CNC_RED_ALERT_TECH_NUMBER_PARSE_H_
#define CNC_RED_ALERT_TECH_NUMBER_PARSE_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include "absl/strings/numbers.h"

namespace tech {

// Parses a complete decimal integer, allowing a sign and surrounding ASCII
// whitespace. Returns nullopt for malformed input or values outside T's range.
// When a failure only means "use a default", call ParseIntegerOr() instead.
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

// The *Or() variants below return `fallback` for malformed, out-of-range or
// null text, and otherwise parse exactly like the functions above:
//   const int count = ParseIntegerOr<int>(token, 1);
//
// They exist for clang-tidy's sake, not for brevity. Any call on a
// std::optional makes bugprone-unchecked-optional-access run its dataflow
// over the whole enclosing function, and a single
// ParseInteger<int>(...).value_or(...) inside a thousand-line dialog or INI
// loader cost 7-28 s per translation unit (measured 2026-09-19). These keep
// std::optional out of the caller.

// Decimal, as ParseInteger().
template <typename T>
T ParseIntegerOr(std::string_view text, std::type_identity_t<T> fallback) {
  T value{};
  return absl::SimpleAtoi(text, &value) ? value : fallback;
}

template <typename T>
T ParseIntegerOr(const char* text, std::type_identity_t<T> fallback) {
  return text == nullptr ? fallback
                         : ParseIntegerOr<T>(std::string_view{text}, fallback);
}

// Hexadecimal, as ParseHex().
template <typename T>
T ParseHexOr(std::string_view text, std::type_identity_t<T> fallback) {
  T value{};
  return absl::SimpleHexAtoi(text, &value) ? value : fallback;
}

template <typename T>
T ParseHexOr(const char* text, std::type_identity_t<T> fallback) {
  return text == nullptr ? fallback
                         : ParseHexOr<T>(std::string_view{text}, fallback);
}

// Parses a decimal 32-bit bit pattern, accepting both unsigned decimal and the
// signed decimal spelling emitted by legacy coordinate writers.
std::optional<uint32_t> ParseDecimalBits(std::string_view text);

// Parses RA INI decimal, $hex, and hexH forms. Hex values retain all 32 bits,
// including the sign bit used by legacy bit masks. Empty/invalid text is
// rejected.
std::optional<int> ParseIniInteger(std::string_view text);

// As ParseIniInteger(), returning `fallback` for empty or invalid text.
int ParseIniIntegerOr(std::string_view text, int fallback);

}  // namespace tech

#endif  // CNC_RED_ALERT_TECH_NUMBER_PARSE_H_
