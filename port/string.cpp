#include <algorithm>
#include <cctype>
#include <compare>
#include <cstring>
#include <functional>
#include <ranges>
#include <span>
#include <string_view>

int stricmp(const char *string1, const char *string2) {
  const std::string_view view1(string1);
  const std::string_view view2(string2);

  auto to_lower = [](unsigned char chr) { return std::tolower(chr); };

  auto result = std::ranges::lexicographical_compare(
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

int strnicmp(const char *string1, const char *string2, size_t count) {
  std::string_view view1(string1);
  std::string_view view2(string2);

  // Limit to count characters
  view1 = view1.substr(0, std::min(view1.size(), count));
  view2 = view2.substr(0, std::min(view2.size(), count));

  auto to_lower = [](unsigned char chr) { return std::tolower(chr); };

  auto result = std::ranges::lexicographical_compare(
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

// TODO(konove): Replace all usage of this function with absl::EqualsIgnoreCase
int memicmp(const void *buffer1, const void *buffer2, size_t count) {
  auto view1 = std::span(static_cast<const unsigned char *>(buffer1), count);
  auto view2 = std::span(static_cast<const unsigned char *>(buffer2), count);

  auto cmp = [](unsigned char chr_a, unsigned char chr_b) {
    return std::tolower(chr_a) <=> std::tolower(chr_b);
  };
  auto result = std::lexicographical_compare_three_way(
      view1.begin(), view1.end(), view2.begin(), view2.end(), cmp);

  if (result < 0) {
    return -1;
  }
  if (result > 0) {
    return 1;
  }
  return 0;
}

// TODO(konove): Replace all usage of this function with absl::AsciiStrToUpper
char *strupr(char *str) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::transform(str, str + strlen(str), str,
                 [](unsigned char chr) { return std::toupper(chr); });
  return str;
}

// TODO(konove): Replace all usage of this function with absl::AsciiStrToLower
char *strlwr(char *str) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::transform(str, str + strlen(str), str,
                 [](unsigned char chr) { return std::tolower(chr); });
  return str;
}

// TODO(konove): Replace all usage of this function with std::reverse
char *strrev(char *str) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::reverse(str, str + strlen(str));
  return str;
}
