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
//
// This tool is temporary: it is the gate for moving Tiberian Dawn onto
// IniFile, and the plan deletes it once that migration is done.

#include <algorithm>
#include <array>
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
constexpr base::ssize kValueCapacity = 1024;
constexpr base::ssize kKeysCapacity = 65536;

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
  std::vector<char> buffer(base::ToSize(kKeysCapacity));
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
  std::array<char, kValueCapacity> out{};
  ReadProfile(text, section, key.c_str(), default_value, out);
  return out.data();
}

std::string IniValue(const IniFile& ini, const std::string& section,
                     const std::string& key) {
  std::array<char, kValueCapacity> out{};
  ini.Get_String(section.c_str(), key.c_str(), kAbsent, out,
                 static_cast<int>(out.size()));
  return out.data();
}

// Compares the two readers on one file. Returns the number of lines printed.
base::ssize CompareFile(const std::string& path) {
  const std::unique_ptr<DiskStream> stream = OpenDiskFile(path);
  if (stream == nullptr) {
    absl::PrintF("cannot open %s\n", path);
    return 1;
  }
  const std::string text = engine::file::ReadProfileText(*stream);

  IniFile ini;
  SpanSource source(std::as_bytes(std::span(text)));
  ini.Load(source);

  base::ssize printed = 0;

  // Check 1: every key IniFile has reads the same through the profile.
  for (int i = 0; i < ini.Section_Count(); ++i) {
    const std::string section = ini.Get_Section(i);
    for (int j = 0; j < ini.Entry_Count(section.c_str()); ++j) {
      const std::string key = ini.Get_Entry(section.c_str(), j);
      const std::string profile = ProfileValue(text, section, key, kAbsent);
      const std::string value = IniValue(ini, section, key);
      if (profile != value) {
        absl::PrintF("VALUE %s [%s] %s: profile=\"%s\" ini=\"%s\"\n", path,
                     section, key, profile, value);
        ++printed;
      }
    }
  }

  // Check 2: every section and key the profile has, IniFile has too.
  for (const std::string& section : ProfileSections(text)) {
    if (!ini.Section_Present(section.c_str())) {
      absl::PrintF("SECTION %s [%s]\n", path, section);
      ++printed;
    }
    std::vector<std::string> common;
    for (const std::string& key : ProfileKeys(text, section)) {
      if (ini.Is_Present(section.c_str(), key.c_str())) {
        common.push_back(absl::AsciiStrToLower(key));
        continue;
      }
      absl::PrintF("MISSING %s [%s] %s: profile=\"%s\"\n", path, section, key,
                   ProfileValue(text, section, key, ""));
      ++printed;
    }
    std::vector<std::string> ini_order;
    for (int j = 0; j < ini.Entry_Count(section.c_str()); ++j) {
      const std::string key =
          absl::AsciiStrToLower(ini.Get_Entry(section.c_str(), j));
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
