#include "port/ex_string.h"

#include <algorithm>
#include <cctype>
#include <compare>
#include <cstddef>
#include <functional>  // IWYU pragma: keep
#include <ranges>
#include <string_view>

#include "port/safe_string.h"

int port::CompareIgnoreCase(std::string_view view1, std::string_view view2) {
  const auto cmp = [](const unsigned char chr_a,
                      const unsigned char chr_b) noexcept {
    return std::tolower(chr_a) <=> std::tolower(chr_b);
  };
  const auto result = std::lexicographical_compare_three_way(
      view1.begin(), view1.end(), view2.begin(), view2.end(), cmp);

  if (result == std::strong_ordering::less) {
    return -1;
  }
  if (result == std::strong_ordering::greater) {
    return 1;
  }
  return 0;
}

#ifndef _WIN32

int stricmp(const char* string1, const char* string2) {
  const std::string_view view1(string1);
  const std::string_view view2(string2);

  const auto to_lower = [](const unsigned char chr) noexcept {
    return std::tolower(chr);
  };

  const auto result = std::ranges::lexicographical_compare(
      view1 | std::views::transform(to_lower),
      view2 | std::views::transform(to_lower));

  // lexicographical_compare returns true if first < second
  if (result) {
    return -1;
  }

  // Check if equal
  if (std::ranges::equal(view1 | std::views::transform(to_lower),
                         view2 | std::views::transform(to_lower))) {
    return 0;
  }

  return 1;
}

int strnicmp(const char* string1, const char* string2, const std::size_t count) {
  std::string_view view1(string1);
  std::string_view view2(string2);

  // Limit to count characters
  view1 = view1.substr(0, std::min(view1.size(), count));
  view2 = view2.substr(0, std::min(view2.size(), count));

  const auto to_lower = [](const unsigned char chr) noexcept {
    return std::tolower(chr);
  };

  const auto result = std::ranges::lexicographical_compare(
      view1 | std::views::transform(to_lower),
      view2 | std::views::transform(to_lower));

  // lexicographical_compare returns true if first < second
  if (result) {
    return -1;
  }

  // Check if equal
  if (std::ranges::equal(view1 | std::views::transform(to_lower),
                         view2 | std::views::transform(to_lower))) {
    return 0;
  }

  return 1;
}

// TODO(konove): Replace all usage of this function with absl::AsciiStrToUpper
char* strupr(char* str) {
  const auto text = port::MutableCString(str);
  std::ranges::transform(text.first(text.size() - 1), text.begin(),
                 [](const unsigned char chr) { return std::toupper(chr); });
  return str;
}

// TODO(konove): Replace all usage of this function with absl::AsciiStrToLower
char* strlwr(char* str) {
  const auto text = port::MutableCString(str);
  std::ranges::transform(text.first(text.size() - 1), text.begin(),
                 [](const unsigned char chr) { return std::tolower(chr); });
  return str;
}

// TODO(konove): Replace all usage of this function with std::reverse
char* strrev(char* str) {
  const auto text = port::MutableCString(str);
  std::ranges::reverse(text.first(text.size() - 1));
  return str;
}

#endif  // _WIN32
