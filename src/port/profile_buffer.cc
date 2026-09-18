#include "port/profile_buffer.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "absl/strings/match.h"
#include "port/safe_string.h"

namespace port {
namespace {

struct Section {
  std::size_t start;
  std::size_t body;
  std::size_t end;
};

std::string_view Trim(std::string_view text ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
    text.remove_prefix(1);
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
    text.remove_suffix(1);
  }
  return text;
}

std::size_t NextLine(std::string_view text, std::size_t position) {
  const auto end = text.find('\n', position);
  return end == std::string_view::npos ? text.size() : end + 1;
}

std::optional<Section> FindSection(std::string_view text, std::string_view name) {
  std::optional<Section> found;
  for (std::size_t pos = 0; pos < text.size();) {
    const auto next = NextLine(text, pos);
    const auto line = text.substr(pos, next - pos);
    if (line.front() == '[') {
      if (found) {
        found->end = pos;
        return found;
      }
      const auto close = line.find(']');
      if (close != std::string_view::npos &&
          absl::EqualsIgnoreCase(line.substr(1, close - 1), name)) {
        found = Section{pos, next, text.size()};
      }
    }
    pos = next;
  }
  return found;
}

struct Entry {
  std::size_t start;
  std::size_t end;
  std::string_view key;
  std::string_view value;
};

std::optional<Entry> ParseEntry(std::string_view text, std::size_t pos,
                               std::size_t end) {
  const auto line = text.substr(pos, end - pos);
  const auto trimmed = Trim(line);
  if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#') {
    return std::nullopt;
  }
  const auto equal = line.find('=');
  if (equal == std::string_view::npos) {
    return std::nullopt;
  }
  return Entry{pos, end, Trim(line.substr(0, equal)), Trim(line.substr(equal + 1))};
}

std::optional<Entry> FindEntry(std::string_view text, const Section& section,
                               std::string_view key) {
  for (auto pos = section.body; pos < section.end;) {
    const auto next = std::min(NextLine(text, pos), section.end);
    const auto entry = ParseEntry(text, pos, next);
    if (entry && absl::EqualsIgnoreCase(entry->key, key)) {
      return entry;
    }
    pos = next;
  }
  return std::nullopt;
}

void CopyValue(std::span<char> output, std::string_view value) {
  if (output.empty()) {
    return;
  }
  const auto copied = value.substr(0, output.size() - 1);
  std::ranges::copy(copied, output.begin());
  std::ranges::fill(output.subspan(copied.size()), '\0');
}

}  // namespace

std::optional<std::size_t> ReadProfile(std::string_view text,
                                     std::string_view section, const char* key,
                                     const char* default_value,
                                     std::span<char> output) {
  // Defaults may alias output (the integer reader uses this convention).
  const std::string fallback(default_value != nullptr ? default_value : "");
  CopyValue(output, fallback);
  const auto found = FindSection(text, section);
  if (!found) {
    return std::nullopt;
  }
  if (key != nullptr) {
    const auto entry = FindEntry(text, *found, key);
    if (!entry) {
      return std::nullopt;
    }
    // Legacy empty entries retain the caller's default.
    if (!entry->value.empty()) {
      CopyValue(output, entry->value);
    }
    return entry->start;
  }
  std::ranges::fill(output, '\0');
  std::size_t written = 0;
  for (auto pos = found->body; pos < found->end;) {
    const auto next = std::min(NextLine(text, pos), found->end);
    const auto entry = ParseEntry(text, pos, next);
    if (entry) {
      if (entry->key.size() + 2 > output.size() - written) {
        break;
      }
      std::ranges::copy(entry->key, output.subspan(written).begin());
      written += entry->key.size() + 1;
    }
    pos = next;
  }
  return found->body;
}

bool WriteProfile(std::span<char> storage, std::string_view section,
                  const char* key, const char* value) {
  const auto nul = std::ranges::find(storage, '\0');
  if (nul == storage.end()) {
    return false;
  }
  // Work separately so overlapping inputs and capacity failure are atomic.
  std::string result(storage.begin(), nul);
  const auto found = FindSection(result, section);
  if (key == nullptr) {
    if (found) {
      result.erase(found->start, found->end - found->start);
    }
  } else if (found) {
    const auto entry = FindEntry(result, *found, key);
    const auto insertion = entry ? entry->start : found->body;
    if (entry) {
      result.erase(entry->start, entry->end - entry->start);
    }
    if (value != nullptr) {
      const std::string separator =
          insertion != 0 && result.at(insertion - 1) != '\n' ? "\r\n" : "";
      result.insert(insertion, separator + key + "=" + value + "\r\n");
    }
  } else if (value != nullptr) {
    result += "\r\n[";
    result += section;
    result += "]\r\n";
    result += key;
    result += '=';
    result += value;
    result += "\r\n";
  }
  if (result.size() >= storage.size()) {
    return false;
  }
  SafeCopy(storage.first(result.size() + 1), result.c_str());
  return true;
}

}  // namespace port
