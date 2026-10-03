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

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
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
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/readline.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"

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
// trims. A line cut short by the end of the data is discarded (the result is
// empty) and `eof` set, as Read_Line does.
std::string ReadLine(ByteSource& source, bool& eof) {
  std::string line;
  for (;;) {
    char c = 0;
    if (!source.ReadObject(c)) {
      eof = true;
      line.clear();
      return line;
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
bool IsSectionLine(std::string_view line) {
  return !line.empty() && line.front() == '[' && line.contains(']');
}

// Returns `line` up to its first ';', without trailing blanks.
std::string_view StripComment(std::string_view line) {
  return absl::StripTrailingAsciiWhitespace(line.substr(0, line.find(';')));
}

}  // namespace

uint32_t IniFile::NameId(std::string_view name) {
  return CrcEngine::Compute(absl::AsciiStrToUpper(name));
}

IniFile::~IniFile() = default;

bool IniFile::Clear() {
  digest_valid_ = false;
  sections_.clear();
  section_index_.clear();
  return true;
}

bool IniFile::Clear(std::string_view section) {
  digest_valid_ = false;
  if (const Section* found = FindSection(section)) {
    RemoveSections(found->id());
  }
  return true;
}

bool IniFile::Clear(std::string_view section, std::string_view entry) {
  digest_valid_ = false;
  if (Section* found = FindSection(section)) {
    if (const Entry* found_entry = found->FindEntry(entry)) {
      found->RemoveEntries(found_entry->id());
    }
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
      // `line`.
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
      section->entry_index.try_emplace(entry->id(), entry.get());
      section->entries.push_back(std::move(entry));
    }

    // A section without entries is not stored. try_emplace keeps the first of
    // two sections with one name as the one lookups find.
    if (!section->entries.empty()) {
      section_index_.try_emplace(section->id(), section.get());
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

IniFile::Section* IniFile::FindSection(std::string_view section) const {
  const auto found = section_index_.find(NameId(section));
  return found == section_index_.end() ? nullptr : found->second;
}

int IniFile::SectionCount() const { return static_cast<int>(sections_.size()); }

const char* IniFile::SectionName(int index) const {
  if (index < 0 || index >= SectionCount()) {
    return nullptr;
  }
  return sections_.at(base::ToSize(index))->name.c_str();
}

int IniFile::EntryCount(std::string_view section) const {
  const Section* found = FindSection(section);
  return found == nullptr ? 0 : static_cast<int>(found->entries.size());
}

const IniFile::Entry* IniFile::FindEntry(std::string_view section,
                                         std::string_view entry) const {
  const Section* found = FindSection(section);
  return found == nullptr ? nullptr : found->FindEntry(entry);
}

const char* IniFile::EntryName(std::string_view section, int index) const {
  const Section* found = FindSection(section);
  if (found == nullptr || index < 0 || index >= std::ssize(found->entries)) {
    return nullptr;
  }
  return found->entries.at(base::ToSize(index))->name.c_str();
}

bool IniFile::PutBase64Block(std::string_view section,
                             std::span<const std::byte> block) {
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
int IniFile::GetBase64Block(std::string_view section,
                            std::span<std::byte> block) const {
  SpanSink sink(block);
  Base64Sink decoder(Base64Mode::kDecode, sink);

  const int entry_count = EntryCount(section);
  for (int index = 0; index < entry_count; index++) {
    char line[128];
    const int length = GetString(section, EntryName(section, index), "=", line);
    decoder.Write(std::as_bytes(std::span(line).first(base::ToSize(length))));
  }
  decoder.Finish();
  return static_cast<int>(sink.bytes_written());
}

bool IniFile::PutTextBlock(std::string_view section, std::string_view text) {
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

int IniFile::GetTextBlock(std::string_view section,
                          std::span<char> buffer) const {
  if (buffer.empty()) {
    return 0;
  }
  buffer.front() = '\0';

  const int entry_count = EntryCount(section);
  int total = 0;
  for (int index = 0; index < entry_count && buffer.size() > 1; index++) {
    // One space stands for each line break PutTextBlock() made.
    if (index > 0) {
      buffer.front() = ' ';
      buffer = buffer.subspan(1);
      total++;
    }

    const int length =
        GetString(section, EntryName(section, index), "", buffer);
    total += length;
    buffer = buffer.subspan(base::ToSize(length));
  }
  return total;
}

bool IniFile::PutInt(std::string_view section, std::string_view entry,
                     int number, int format) {
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

int IniFile::GetInt(std::string_view section, std::string_view entry,
                    int default_value) const {
  const Entry* found = FindEntry(section, entry);
  return found == nullptr
             ? default_value
             : base::ParseIniIntegerOr(found->value, default_value);
}

bool IniFile::PutHex(std::string_view section, std::string_view entry,
                     int number) {
  return PutString(section, entry,
                   absl::StrFormat("%X", static_cast<unsigned int>(number)));
}

int IniFile::GetHex(std::string_view section, std::string_view entry,
                    int default_value) const {
  if (const Entry* found = FindEntry(section, entry)) {
    // All 32 bits are kept, so "FFFFFFFF" reads as -1.
    if (const auto value = base::ParseHex<uint32_t>(found->value)) {
      return static_cast<int>(*value);
    }
  }
  return default_value;
}

bool IniFile::PutString(std::string_view section, std::string_view entry,
                        std::string_view value) {
  digest_valid_ = false;
  Section* found = FindSection(section);
  if (found == nullptr) {
    found = sections_.emplace_back(std::make_unique<Section>()).get();
    found->name = std::string(section);
    section_index_.try_emplace(found->id(), found);
  }

  // Remove the old entry and any duplicate of it.
  found->RemoveEntries(NameId(entry));

  // An empty value is not stored: Load() would drop the line anyway.
  if (!value.empty()) {
    Entry* added = found->entries
                       .emplace_back(std::make_unique<Entry>(
                           std::string(entry), std::string(value)))
                       .get();
    found->entry_index.try_emplace(added->id(), added);
  }
  return true;
}

std::string IniFile::GetString(std::string_view section, std::string_view entry,
                               std::string_view default_value) const {
  const Entry* found = FindEntry(section, entry);
  return std::string(absl::StripAsciiWhitespace(
      found == nullptr ? default_value : std::string_view(found->value)));
}

int IniFile::GetString(std::string_view section, std::string_view entry,
                       std::string_view default_value,
                       std::span<char> buffer) const {
  if (buffer.empty()) {
    return 0;
  }
  const Entry* found = FindEntry(section, entry);
  // A caller may use its output as the default. Copy before overwriting it.
  const std::string value(found == nullptr ? default_value
                                           : std::string_view(found->value));
  // Cut to the buffer first and trim after, so a cut that ends in blanks
  // loses them too.
  base::SafeCopy(buffer, value);
  strtrim(buffer);
  return static_cast<int>(std::string_view(buffer.data()).size());
}

bool IniFile::PutBool(std::string_view section, std::string_view entry,
                      bool value) {
  return PutString(section, entry, value ? "yes" : "no");
}

bool IniFile::GetBool(std::string_view section, std::string_view entry,
                      bool default_value) const {
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
    std::string_view entry_name) const {
  const auto found = entry_index.find(NameId(entry_name));
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

  Sha1Digest embedded{};
  if (GetBase64Block(kDigestSection, base::ObjectBytes(embedded)) <= 0) {
    return IniLoadResult::kLoaded;
  }

  // The digest covers the data without its own section.
  Clear(kDigestSection);
  ComputeDigest();
  return embedded == digest_ ? IniLoadResult::kLoaded
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

// The index slot goes first so no pointer to a destroyed entry is compared.
// Names are matched by id, the same key the index looks them up by.
void IniFile::Section::RemoveEntries(uint32_t entry_id) {
  entry_index.erase(entry_id);
  std::erase_if(entries, [entry_id](const auto& owned) {
    return owned->id() == entry_id;
  });
}

// Same as Section::RemoveEntries, for sections.
void IniFile::RemoveSections(uint32_t section_id) {
  section_index_.erase(section_id);
  std::erase_if(sections_, [section_id](const auto& owned) {
    return owned->id() == section_id;
  });
}

bool IniFile::PutPublicKey(const PKey& key) {
  std::byte encoded[512];

  int length = key.Encode_Modulus(encoded);
  PutBase64Block(kPublicKeySection,
                 std::span(encoded).first(base::ToSize(length)));

  length = key.Encode_Exponent(encoded);
  PutBase64Block(kPrivateKeySection,
                 std::span(encoded).first(base::ToSize(length)));
  return true;
}

PKey IniFile::GetPublicKey(bool fast) const {
  PKey key;
  unsigned char encoded[512];

  // The fast key's exponent is a known constant, so a database published
  // without [PrivateKey] still yields it.
  if (fast) {
    const BigInt exponent = PKey::Fast_Exponent();
    const auto length = exponent.DEREncode(encoded);
    key.Decode_Exponent(base::ObjectBytes(encoded).first(base::ToSize(length)));
  } else {
    const int length =
        GetBase64Block(kPrivateKeySection, base::ObjectBytes(encoded));
    key.Decode_Exponent(base::ObjectBytes(encoded).first(base::ToSize(length)));
  }

  const int length =
      GetBase64Block(kPublicKeySection, base::ObjectBytes(encoded));
  key.Decode_Modulus(base::ObjectBytes(encoded).first(base::ToSize(length)));

  return key;
}

Fixed IniFile::GetFixed(std::string_view section, std::string_view entry,
                        Fixed default_value) const {
  // A value that is only blanks counts as missing.
  const std::string text = GetString(section, entry, "");
  return text.empty() ? default_value : Fixed::FromString(text);
}

bool IniFile::PutFixed(std::string_view section, std::string_view entry,
                       Fixed value) {
  return PutString(section, entry, value.ToString());
}

}  // namespace engine::file
