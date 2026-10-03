// File: ini_compare, a command line tool that checks engine::file::IniFile
// against engine::file::ReadProfile, the text-scanning reader Tiberian Dawn
// reads its INI files with today.
//
// For every INI file named on the command line it prints a line for each way
// the two readers disagree, and exits 1 if it printed any:
//
//   VALUE   file [s] k: profile="..." ini="..."  a key reads differently
//   MISSING file [s] k: profile="..."            a key IniFile lacks
//   ORDER   file [s]                             common keys in another order
//   SECTION file [s]                             a section IniFile lacks
//   EMPTY   file                                 no sections, or Load failed
//
// Each file also gets one FILE summary line (sections in the text, keys
// compared, differences), which does not count as a difference itself.
//
// This tool is temporary: it is the gate for moving Tiberian Dawn onto
// IniFile, and the plan deletes it once that migration is done.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/file/disk_file.h"
#include "engine/file/disk_stream.h"
#include "engine/file/ini_file.h"
#include "engine/file/profile_buffer.h"
#include "engine/stream/span_source.h"

namespace {

using engine::file::IniFile;
using engine::file::ReadProfile;

// Never a real value, so a default that comes back means "not present".
constexpr const char* kAbsent = "\x01";

// Returns a buffer no value or key list drawn from text can fill: both are
// shorter than the text, and enumeration adds one terminator and a final NUL.
// A fixed size would silently truncate a long value or a long key list.
std::vector<char> BufferFor(const std::string_view text) {
  return std::vector<char>(text.size() + 2);
}

// Returns the section names the text's `[name]` header lines declare, once
// each by case-insensitive comparison. IniFile may drop or merge a section, so
// its own list cannot be the one that drives the comparison.
std::vector<std::string> ProfileSections(const std::string_view text) {
  std::vector<std::string> names;
  std::vector<std::string> seen;
  std::string_view rest = text;
  while (!rest.empty()) {
    const std::size_t end = rest.find('\n');
    std::string_view line = rest.substr(0, end);
    rest = end == std::string_view::npos ? std::string_view()
                                         : rest.substr(end + 1);
    line = absl::StripAsciiWhitespace(line);
    if (line.empty() || line.front() != '[') {
      continue;
    }
    const std::size_t close = line.find(']');
    if (close == std::string_view::npos) {
      continue;
    }
    const std::string name(line.substr(1, close - 1));
    const std::string folded = absl::AsciiStrToLower(name);
    if (!std::ranges::contains(seen, folded)) {
      seen.push_back(folded);
      names.push_back(name);
    }
  }
  return names;
}

// Returns the keys of section as the profile enumerates them, once each.
std::vector<std::string> ProfileKeys(const std::string_view text,
                                     const std::string& section) {
  std::vector<char> buffer = BufferFor(text);
  std::vector<std::string> keys;
  std::vector<std::string> seen;
  if (!ReadProfile(text, section, nullptr, nullptr, buffer)) {
    return keys;
  }
  std::string_view rest(buffer.data(), buffer.size());
  while (!rest.empty() && rest.front() != '\0') {
    const std::string_view key = rest.substr(
        0, base::ToSize(std::ranges::find(rest, '\0') - rest.begin()));
    const std::string folded = absl::AsciiStrToLower(key);
    if (!std::ranges::contains(seen, folded)) {
      seen.push_back(folded);
      keys.emplace_back(key);
    }
    rest.remove_prefix(key.size() + 1);
  }
  return keys;
}

std::string ProfileValue(const std::string_view text,
                         const std::string& section, const std::string& key,
                         const char* default_value) {
  std::vector<char> out = BufferFor(text);
  ReadProfile(text, section, key.c_str(), default_value, out);
  return out.data();
}

std::string IniValue(const IniFile& ini, const std::string_view text,
                     const std::string& section, const std::string& key) {
  std::vector<char> out = BufferFor(text);
  ini.GetString(section, key, kAbsent, out);
  return out.data();
}

// Compares the two readers on one file and prints a FILE summary line after
// any difference lines. Returns the number of difference lines printed.
base::ssize CompareFile(const std::string& path) {
  const std::unique_ptr<DiskStream> stream = OpenDiskFile(path);
  if (stream == nullptr) {
    absl::PrintF("cannot open %s\n", path);
    absl::PrintF("FILE %s: 0 sections, 0 keys, 1 differences\n", path);
    return 1;
  }
  const std::string text = engine::file::ReadProfileText(*stream);

  IniFile ini;
  SpanSource source(std::as_bytes(std::span(text)));
  const bool loaded = ini.Load(source);

  base::ssize printed = 0;
  base::ssize keys_compared = 0;
  const std::vector<std::string> sections = ProfileSections(text);
  if (!loaded || sections.empty()) {
    absl::PrintF("EMPTY %s\n", path);
    ++printed;
  }

  // Check 1: every key IniFile has reads the same through the profile.
  for (int i = 0; i < ini.SectionCount(); ++i) {
    const std::string section = ini.SectionName(i);
    for (int j = 0; j < ini.EntryCount(section); ++j) {
      const std::string key = ini.EntryName(section, j);
      const std::string profile = ProfileValue(text, section, key, kAbsent);
      const std::string value = IniValue(ini, text, section, key);
      ++keys_compared;
      if (profile != value) {
        absl::PrintF("VALUE %s [%s] %s: profile=\"%s\" ini=\"%s\"\n", path,
                     section, key, profile, value);
        ++printed;
      }
    }
  }

  // Check 2: every section and key the profile has, IniFile has too.
  for (const std::string& section : sections) {
    if (!ini.HasSection(section)) {
      absl::PrintF("SECTION %s [%s]\n", path, section);
      ++printed;
    }
    std::vector<std::string> common;
    for (const std::string& key : ProfileKeys(text, section)) {
      if (ini.HasEntry(section, key)) {
        common.push_back(absl::AsciiStrToLower(key));
        continue;
      }
      absl::PrintF("MISSING %s [%s] %s: profile=\"%s\"\n", path, section, key,
                   ProfileValue(text, section, key, ""));
      ++printed;
    }
    std::vector<std::string> ini_order;
    for (int j = 0; j < ini.EntryCount(section); ++j) {
      const std::string key = absl::AsciiStrToLower(ini.EntryName(section, j));
      if (std::ranges::contains(common, key) &&
          !std::ranges::contains(ini_order, key)) {
        ini_order.push_back(key);
      }
    }
    if (ini_order != common) {
      absl::PrintF("ORDER %s [%s]\n", path, section);
      ++printed;
    }
  }
  absl::PrintF("FILE %s: %d sections, %d keys, %d differences\n", path,
               std::ssize(sections), keys_compared, printed);
  return printed;
}

}  // namespace

int main(int argc, char** argv) {
  // argv is a pointer and a count, which is the one place a span has to be
  // built from both; argc counts the entries argv holds.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char* const> args(argv, base::ToSize(argc));
  if (args.size() < 2) {
    absl::FPrintF(stderr, "usage: ini_compare <file.ini>...\n");
    return 2;
  }
  base::ssize printed = 0;
  for (const char* const arg : args.subspan(1)) {
    printed += CompareFile(arg);
  }
  return printed == 0 ? 0 : 1;
}
