/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "engine/file/ini_file.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/codec/base64.h"
#include "engine/codec/base64_sink.h"
#include "engine/codec/base64_source.h"
#include "engine/crypto/crc.h"
#include "engine/crypto/int.h"
#include "engine/crypto/pk.h"
#include "engine/crypto/sha.h"
#include "engine/crypto/sha1_sink.h"
#include "engine/file/game_file.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/readline.h"
#include "engine/stream/span_source.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"
#include "engine/stream/vector_sink.h"

namespace engine::file {

namespace {

// The section LoadVerified() and SaveWithDigest() keep the SHA-1 in, and the
// two that hold a public key.
constexpr std::string_view kDigestSection = "Digest";
constexpr std::string_view kPublicKeySection = "PublicKey";
constexpr std::string_view kPrivateKeySection = "PrivateKey";

// The longest line PutTextBlock() stores, and the base64 characters
// PutBase64Block() puts in one entry.
constexpr int kTextLineLength = 75;
constexpr int kBase64LineLength = 70;

// Returns the next line without a length limit: stops at LF, drops CR and
// trims. The end of the data sets `eof` and ends the line it cuts short, which
// is returned like any other: a file need not end in a line feed. Only when
// the data ends right after a line feed, or is empty, is that last line empty.
std::string ReadLine(ByteSource& source, bool& eof) {
  std::string line;
  for (;;) {
    char c = 0;
    if (!source.ReadObject(c)) {
      eof = true;
      break;
    }
    if (c == '\n') {
      break;
    }
    if (c != '\r') {
      line.push_back(c);
    }
  }
  absl::StripAsciiWhitespace(&line);
  return line;
}

// True for a section header, "[name]".
bool IsSectionLine(const std::string_view line) {
  return !line.empty() && line.front() == '[' && line.contains(']');
}

// Returns `line` up to its first ';', without trailing blanks.
std::string_view StripComment(const std::string_view line) {
  return absl::StripTrailingAsciiWhitespace(line.substr(0, line.find(';')));
}

}  // namespace

// FNV-1a over the upper-cased characters: names that NameEqual calls equal
// hash alike, and no folded copy of the name is allocated per lookup.
std::size_t IniFile::NameHash::operator()(const std::string_view name) const {
  constexpr uint64_t kOffsetBasis = 0xCBF29CE484222325U;
  constexpr uint64_t kPrime = 0x100000001B3U;
  uint64_t hash = kOffsetBasis;
  for (const char c : name) {
    hash ^= static_cast<unsigned char>(
        absl::ascii_toupper(static_cast<unsigned char>(c)));
    hash *= kPrime;
  }
  return static_cast<std::size_t>(hash);
}

bool IniFile::NameEqual::operator()(const std::string_view left,
                                    const std::string_view right) const {
  return absl::EqualsIgnoreCase(left, right);
}

IniFile::~IniFile() = default;

bool IniFile::Clear() {
  digest_valid_ = false;
  // The index goes first: its keys are views of the section names.
  section_index_.clear();
  sections_.clear();
  return true;
}

bool IniFile::Clear(const std::string_view section) {
  digest_valid_ = false;
  RemoveSections(section);
  return true;
}

bool IniFile::Clear(const std::string_view section,
                    const std::string_view entry) {
  digest_valid_ = false;
  if (Section* found = FindSection(section)) {
    found->RemoveEntries(entry);
  }
  return true;
}

bool IniFile::Load(ByteStream& stream) {
  StreamSource source(stream);
  return Load(source);
}

bool IniFile::Load(ByteSource& source) {
  digest_valid_ = false;
  bool end_of_file = false;
  std::string line;

  // Skip to the first section header. Data without one is not an INI file.
  while (!end_of_file) {
    line = ReadLine(source, end_of_file);
    if (end_of_file) {
      // A header that is the last line of the data has no entries after it.
      return false;
    }
    if (IsSectionLine(line)) {
      break;
    }
  }

  // One pass per section; `line` holds its header on entry.
  while (!end_of_file) {
    const std::string_view header = line;
    auto section = std::make_unique<Section>();
    section->name = std::string(
        absl::StripAsciiWhitespace(header.substr(1, header.find(']') - 1)));

    while (!end_of_file) {
      // The next header ends this section; the outer loop takes it from
      // `line`. A last line without a line feed is parsed below like the rest,
      // and the loop ends after it.
      line = ReadLine(source, end_of_file);
      if (IsSectionLine(line)) {
        break;
      }

      // Blank lines, whole-line comments and lines without an entry name are
      // dropped, so they do not survive a load and save.
      const std::string_view text = StripComment(line);
      if (text.empty() || text.front() == ';' || text.front() == '=') {
        continue;
      }

      // Split at the first '='. A line without one and "name=" with no value
      // are ignored; "=value" was dropped above.
      const auto split = text.find('=');
      if (split == std::string_view::npos) {
        continue;
      }
      const std::string_view name =
          absl::StripAsciiWhitespace(text.substr(0, split));
      const std::string_view value =
          absl::StripAsciiWhitespace(text.substr(split + 1));
      if (name.empty() || value.empty()) {
        continue;
      }

      auto entry =
          std::make_unique<Entry>(std::string(name), std::string(value));
      // try_emplace keeps the first of two entries with one name as the one
      // lookups find.
      section->entry_index.try_emplace(entry->name, entry.get());
      section->entries.push_back(std::move(entry));
    }

    // A section without entries is not stored. As with entries, the first of
    // two sections with one name is the one lookups find.
    if (!section->entries.empty()) {
      section_index_.try_emplace(section->name, section.get());
      sections_.push_back(std::move(section));
    }
  }
  return true;
}

bool IniFile::Save(ByteStream& stream) const {
  StreamSink sink(stream);
  return Save(sink);
}

bool IniFile::Save(ByteSink& sink) const {
  WriteSections(sink);
  return sink.Finish();
}

// Writes the sections without finishing the sink, so SaveWithDigest can
// append a second database's sections before the one Finish().
void IniFile::WriteSections(ByteSink& sink) const {
  const auto put = [&sink](std::string_view text) {
    sink.Write(std::as_bytes(std::span(text)));
  };

  for (const auto& section : sections_) {
    put("[");
    put(section->name);
    put("]");
    put("\r\n");

    for (const auto& entry : section->entries) {
      put(entry->name);
      put("=");
      put(entry->value);
      put("\r\n");
    }

    // A blank line after each section, for readability.
    put("\r\n");
  }
}

IniFile::Section* IniFile::FindSection(const std::string_view section) const {
  const auto found = section_index_.find(section);
  return found == section_index_.end() ? nullptr : found->second;
}

int IniFile::SectionCount() const { return static_cast<int>(sections_.size()); }

std::string_view IniFile::SectionName(const int index) const {
  if (index < 0 || index >= SectionCount()) {
    return {};
  }
  return sections_.at(base::ToSize(index))->name;
}

int IniFile::EntryCount(const std::string_view section) const {
  const Section* found = FindSection(section);
  return found == nullptr ? 0 : static_cast<int>(found->entries.size());
}

const IniFile::Entry* IniFile::FindEntry(const std::string_view section,
                                         const std::string_view entry) const {
  const Section* found = FindSection(section);
  return found == nullptr ? nullptr : found->FindEntry(entry);
}

std::string_view IniFile::EntryName(const std::string_view section,
                                    const int index) const {
  const Section* found = FindSection(section);
  if (found == nullptr || index < 0 || index >= std::ssize(found->entries)) {
    return {};
  }
  return found->entries.at(base::ToSize(index))->name;
}

bool IniFile::PutBase64Block(const std::string_view section,
                             const std::span<const std::byte> block) {
  if (block.empty()) {
    return false;
  }

  Clear(section);

  SpanSource source(block);
  Base64Source encoder(Base64Mode::kEncode, source);

  for (int line_number = 1;; ++line_number) {
    char line[kBase64LineLength];
    const auto length = encoder.Read(std::as_writable_bytes(std::span(line)));
    if (length == 0) {
      break;
    }
    PutString(section, absl::StrCat(line_number),
              std::string_view(line, base::ToSize(length)));
  }
  return true;
}

// The entries are fetched by name, as the original did, so a section with a
// duplicated entry name decodes the first of them twice.
std::vector<std::byte> IniFile::GetBase64Block(
    const std::string_view section) const {
  std::vector<std::byte> block;
  VectorSink sink(block);
  Base64Sink decoder(Base64Mode::kDecode, sink);

  const int entry_count = EntryCount(section);
  for (int index = 0; index < entry_count; index++) {
    const std::string line = GetString(section, EntryName(section, index), "=");
    decoder.Write(std::as_bytes(std::span(line)));
  }
  decoder.Finish();
  return block;
}

bool IniFile::PutTextBlock(const std::string_view section,
                           std::string_view text) {
  Clear(section);

  for (int line_number = 1; !text.empty(); ++line_number) {
    char line[kTextLineLength + 1];
    base::SafeCopy(line, text);

    // A full buffer may have cut a word: break the line at its last blank.
    // The terminator the scan starts on is not a blank.
    int length = static_cast<int>(std::string_view(line).size());
    if (length == 0) {
      break;
    }
    if (length >= kTextLineLength) {
      while (length > 0 && !absl::ascii_isspace(static_cast<unsigned char>(
                               base::At(line, length)))) {
        length--;
      }
      if (length == 0) {
        break;
      }
      base::At(line, length) = '\0';
    }

    strtrim(line);
    PutString(section, absl::StrCat(line_number), line);
    text.remove_prefix(base::ToSize(length));
  }
  return true;
}

std::string IniFile::GetTextBlock(const std::string_view section) const {
  std::string text;
  const int entry_count = EntryCount(section);
  for (int index = 0; index < entry_count; index++) {
    // One space stands for each line break PutTextBlock() made.
    if (index > 0) {
      text += ' ';
    }
    text += GetString(section, EntryName(section, index), "");
  }
  return text;
}

bool IniFile::PutInt(const std::string_view section,
                     const std::string_view entry, const int number,
                     const int format) {
  const auto bits = static_cast<unsigned int>(number);
  switch (format) {
    case 1:
      return PutString(section, entry, absl::StrFormat("%Xh", bits));
    case 2:
      return PutString(section, entry, absl::StrFormat("$%X", bits));
    default:
      return PutString(section, entry, absl::StrCat(number));
  }
}

int IniFile::GetInt(const std::string_view section,
                    const std::string_view entry,
                    const int default_value) const {
  const Entry* found = FindEntry(section, entry);
  return found == nullptr
             ? default_value
             : base::ParseIniIntegerOr(found->value, default_value);
}

bool IniFile::PutHex(const std::string_view section,
                     const std::string_view entry, const int number) {
  return PutString(section, entry,
                   absl::StrFormat("%X", static_cast<unsigned int>(number)));
}

int IniFile::GetHex(const std::string_view section,
                    const std::string_view entry,
                    const int default_value) const {
  if (const Entry* found = FindEntry(section, entry)) {
    // All 32 bits are kept, so "FFFFFFFF" reads as -1.
    if (const auto value = base::ParseHex<uint32_t>(found->value)) {
      return static_cast<int>(*value);
    }
  }
  return default_value;
}

bool IniFile::PutString(const std::string_view section,
                        const std::string_view entry,
                        const std::string_view value) {
  digest_valid_ = false;
  Section* found = FindSection(section);
  if (found == nullptr) {
    found = sections_.emplace_back(std::make_unique<Section>()).get();
    found->name = std::string(section);
    section_index_.try_emplace(found->name, found);
  }

  // An empty value is not stored: Load() would drop the line anyway. The new
  // entry is built before the old one goes, because `entry` may be the old
  // one's EntryName().
  auto replacement =
      value.empty()
          ? nullptr
          : std::make_unique<Entry>(std::string(entry), std::string(value));

  // Remove the old entry and any duplicate of it.
  found->RemoveEntries(entry);

  if (replacement != nullptr) {
    Entry* added = found->entries.emplace_back(std::move(replacement)).get();
    found->entry_index.try_emplace(added->name, added);
  }
  return true;
}

std::string IniFile::GetString(const std::string_view section,
                               const std::string_view entry,
                               const std::string_view default_value) const {
  const Entry* found = FindEntry(section, entry);
  return std::string(absl::StripAsciiWhitespace(
      found == nullptr ? default_value : std::string_view(found->value)));
}

bool IniFile::PutBool(const std::string_view section,
                      const std::string_view entry, const bool value) {
  return PutString(section, entry, value ? "yes" : "no");
}

bool IniFile::GetBool(const std::string_view section,
                      const std::string_view entry,
                      const bool default_value) const {
  if (const Entry* found = FindEntry(section, entry)) {
    switch (
        absl::ascii_toupper(static_cast<unsigned char>(found->value.front()))) {
      case 'Y':
      case 'T':
      case '1':
        return true;

      case 'N':
      case 'F':
      case '0':
        return false;
      default:
        break;
    }
  }
  return default_value;
}

const IniFile::Entry* IniFile::Section::FindEntry(
    const std::string_view entry_name) const {
  const auto found = entry_index.find(entry_name);
  return found == entry_index.end() ? nullptr : found->second;
}

IniLoadResult IniFile::LoadVerified(ByteStream& stream) {
  StreamSource source(stream);
  return LoadVerified(source);
}

IniLoadResult IniFile::LoadVerified(ByteSource& source) {
  if (!Load(source)) {
    return IniLoadResult::kFailed;
  }

  const std::vector<std::byte> embedded = GetBase64Block(kDigestSection);
  if (embedded.empty()) {
    return IniLoadResult::kLoaded;
  }

  // The digest covers the data without its own section.
  Clear(kDigestSection);
  ComputeDigest();
  return std::ranges::equal(embedded, digest_) ? IniLoadResult::kLoaded
                                               : IniLoadResult::kDigestMismatch;
}

bool IniFile::SaveWithDigest(ByteStream& stream) const {
  StreamSink sink(stream);
  return SaveWithDigest(sink);
}

// The digest section is built in a scratch database and saved after the data,
// which is what appending it to this one would produce.
bool IniFile::SaveWithDigest(ByteSink& sink) const {
  ComputeDigest();
  IniFile digest_section;
  digest_section.PutBase64Block(kDigestSection, base::ObjectBytes(digest_));
  DCHECK(!HasSection(kDigestSection));
  WriteSections(sink);
  digest_section.WriteSections(sink);
  return sink.Finish();
}

int IniFile::UniqueId() const {
  if (!digest_valid_) {
    ComputeDigest();
  }

  CrcEngine crc;
  for (const std::byte byte : digest_) {
    crc.Update(std::to_integer<uint8_t>(byte));
  }
  return static_cast<int>(crc.Value());
}

void IniFile::ComputeDigest() const {
  NullSink discard;
  Sha1Sink sha(discard);
  Save(sha);
  digest_ = sha.digest();
  digest_valid_ = true;
}

// The index holds the first element of every name present, so a name it does
// not know has nothing to remove: writing a new entry does not scan the
// section. The slot goes before the elements because its key is a view of
// the first one's name; the scan then matches names the way the index does.
// The name is copied for the scan: a caller may pass EntryName(), a view of
// an entry the scan destroys.
void IniFile::Section::RemoveEntries(const std::string_view entry_name) {
  if (entry_index.erase(entry_name) == 0) {
    return;
  }
  const std::string removed(entry_name);
  std::erase_if(entries, [&removed](const auto& owned) {
    return NameEqual()(owned->name, removed);
  });
}

// Same as Section::RemoveEntries, for sections.
void IniFile::RemoveSections(const std::string_view section) {
  if (section_index_.erase(section) == 0) {
    return;
  }
  const std::string removed(section);
  std::erase_if(sections_, [&removed](const auto& owned) {
    return NameEqual()(owned->name, removed);
  });
}

PKey IniFile::GetPublicKey(const bool fast) const {
  PKey key;

  // The fast key's exponent is a known constant, so a database published
  // without [PrivateKey] still yields it.
  if (fast) {
    unsigned char encoded[512];
    const BigInt exponent = PKey::Fast_Exponent();
    const auto length = exponent.DEREncode(encoded);
    key.Decode_Exponent(base::ObjectBytes(encoded).first(base::ToSize(length)));
  } else {
    key.Decode_Exponent(GetBase64Block(kPrivateKeySection));
  }

  key.Decode_Modulus(GetBase64Block(kPublicKeySection));

  return key;
}

Fixed IniFile::GetFixed(const std::string_view section,
                        const std::string_view entry,
                        const Fixed default_value) const {
  // A value that is only blanks counts as missing.
  const std::string text = GetString(section, entry, "");
  return text.empty() ? default_value : Fixed::FromString(text);
}

bool IniFile::PutFixed(const std::string_view section,
                       const std::string_view entry, const Fixed value) {
  return PutString(section, entry, value.ToString());
}

std::string ReadIniText(ByteStream& stream) {
  std::string text = stream.ReadString(stream.Size() - stream.Tell());
  if (const auto nul = text.find('\0'); nul != std::string::npos) {
    text.resize(nul);
  }
  return text;
}

std::optional<std::string> ReadIniFile(std::string_view name) {
  const auto file = OpenGameFile(name);
  if (!file) {
    return std::nullopt;
  }
  return ReadIniText(*file);
}

}  // namespace engine::file
