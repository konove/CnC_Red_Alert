#include <algorithm>
#include <cctype>
#include <compare>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <span>
#include <string>
#include <string_view>

void _makepath(char *path, const char *drive, const char *dir,
               const char *fname, const char *ext) {
  // Use std::filesystem for robust path construction
  std::filesystem::path full_path;

  // 1. Handle Drive
  if (drive != nullptr && strlen(drive) > 0) {
    full_path += drive;
    // _makepath expects the caller to provide the colon, but if missing,
    // std::filesystem logic or manual checks might be needed depending on
    // strictness. Standard _makepath behavior simply concatenates.
  }

  // 2. Handle Directory
  if (dir != nullptr && strlen(dir) > 0) {
    // Modern approach: standard path concatenation handles separators
    full_path /= dir;
  }

  // 3. Handle Filename
  if (fname != nullptr && strlen(fname) > 0) {
    // If 'dir' didn't end in a separator and 'fname' doesn't start with one,
    // std::filesystem::path::operator/= handles it. However, _makepath
    // historically is a simple string builder.
    // To strictly mimic _makepath behavior (concatenation) while using modern
    // types:
    std::string dir_str = full_path.string();

    // _makepath logic usually dictates that if dir doesn't end in \ or /,
    // insert one. std::filesystem handles this naturally via concatenation.
    if (!dir_str.empty() && dir_str.back() != '/' && dir_str.back() != '\\') {
      full_path /= fname;
    } else {
      // If it ends with separator, simple concatenation to avoid double slash
      full_path += fname;
    }
  }

  // 4. Handle Extension
  if (ext != nullptr && strlen(ext) > 0) {
    std::string ext_str = ext;
    // _makepath automatically adds '.' if missing and ext is not empty
    if (ext_str[0] != '.') {
      full_path += ".";
    }
    full_path += ext_str;
  }

  // Copy to output buffer
  // Note: The caller is responsible for ensuring 'path' is large enough
  // (typically _MAX_PATH).
  const std::string final_str = full_path.string();
  std::strcpy(path, final_str.c_str());
}

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
