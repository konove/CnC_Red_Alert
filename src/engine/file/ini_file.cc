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

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
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

}  // namespace

uint32_t IniFile::Name_ID(std::string_view name) {
  return CrcEngine::Compute(absl::AsciiStrToUpper(name));
}

IniFile::~IniFile() { Clear(); }

IniFile::IniEntry::~IniEntry() = default;

IniFile::IniSection::~IniSection() = default;

bool IniFile::Clear(const char* section, const char* entry) {
  Invalidate_Message_Digest();
  if (section == nullptr) {
    SectionList.clear();
    SectionIndex.clear();
  } else {
    IniSection* secptr = Find_Section(section);
    if (secptr != nullptr) {
      if (entry != nullptr) {
        const IniEntry* entptr = secptr->Find_Entry(entry);
        if (entptr != nullptr) {
          secptr->Remove_Entries(entptr->Index_ID());
        }
      } else {
        Remove_Sections(secptr->Index_ID());
      }
    }
  }
  return true;
}

bool IniFile::Load(ByteStream& file) {
  StreamSource fs(file);
  return Load(fs);
}

bool IniFile::Load(ByteSource& file) {
  Invalidate_Message_Digest();
  bool end_of_file = false;
  std::string line;

  // Skip to the first section header. Data without one is not an INI file.
  while (!end_of_file) {
    line = ReadLine(file, end_of_file);
    if (end_of_file) {
      return false;
    }
    if (IsSectionLine(line)) {
      break;
    }
  }

  // One pass per section; `line` holds its header on entry.
  while (!end_of_file) {
    const std::string_view name = absl::StripAsciiWhitespace(
        std::string_view(line).substr(1, std::string_view(line).find(']') - 1));
    auto secptr = std::make_unique<IniSection>(std::string(name).c_str());

    while (!end_of_file) {
      // The next header ends this section; the outer loop takes it from
      // `line`.
      line = ReadLine(file, end_of_file);
      if (IsSectionLine(line)) {
        break;
      }

      // Blank lines, whole-line comments and lines without an entry name are
      // dropped, so they do not survive a load and save.
      const std::string_view text = Strip_Comments(line);
      if (text.empty() || text.front() == ';' || text.front() == '=') {
        continue;
      }

      // Split at the first '='. A line without one and "name=" with no value
      // are ignored; "=value" was dropped above.
      const auto split = text.find('=');
      if (split == std::string_view::npos) {
        continue;
      }
      const std::string_view key =
          absl::StripAsciiWhitespace(text.substr(0, split));
      const std::string_view value =
          absl::StripAsciiWhitespace(text.substr(split + 1));
      if (key.empty() || value.empty()) {
        continue;
      }

      auto entryptr = std::make_unique<IniEntry>(std::string(key).c_str(),
                                                 std::string(value).c_str());
      secptr->EntryIndex.try_emplace(entryptr->Index_ID(), entryptr.get());
      secptr->EntryList.push_back(std::move(entryptr));
    }

    // A section without entries is not stored. try_emplace keeps the first of
    // two sections with one name as the one lookups find.
    if (!secptr->EntryList.empty()) {
      SectionIndex.try_emplace(secptr->Index_ID(), secptr.get());
      SectionList.push_back(std::move(secptr));
    }
  }
  return true;
}

bool IniFile::Save(ByteStream& file) const {
  StreamSink fp(file);
  return Save(fp);
}

bool IniFile::Save(ByteSink& pipe) const {
  WriteSections(pipe);
  return pipe.Finish();
}

// Writes the sections without finishing the sink, so SaveWithDigest can
// append a second database's sections before the one Finish().
void IniFile::WriteSections(ByteSink& pipe) const {
  const auto put = [&pipe](std::string_view text) {
    pipe.Write(std::as_bytes(std::span(text)));
  };

  for (const auto& secptr : SectionList) {
    put("[");
    put(secptr->Section);
    put("]");
    put("\r\n");

    for (const auto& entryptr : secptr->EntryList) {
      put(entryptr->Entry);
      put("=");
      put(entryptr->Value);
      put("\r\n");
    }

    // A blank line after each section, for readability.
    put("\r\n");
  }
}

IniFile::IniSection* IniFile::Find_Section(const char* section) const {
  if (section != nullptr) {
    const auto found = SectionIndex.find(Name_ID(section));
    if (found != SectionIndex.end()) {
      return found->second;
    }
  }
  return nullptr;
}

int IniFile::Section_Count() const {
  return static_cast<int>(SectionList.size());
}

const char* IniFile::Get_Section(int index) const {
  if (index < 0 || index >= Section_Count()) {
    return nullptr;
  }
  return SectionList.at(base::ToSize(index))->Section.c_str();
}

int IniFile::Entry_Count(const char* section) const {
  const IniSection* secptr = Find_Section(section);
  if (secptr != nullptr) {
    return static_cast<int>(secptr->EntryList.size());
  }
  return 0;
}

IniFile::IniEntry* IniFile::Find_Entry(const char* section,
                                       const char* entry) const {
  const IniSection* secptr = Find_Section(section);
  if (secptr != nullptr) {
    return secptr->Find_Entry(entry);
  }
  return nullptr;
}

const char* IniFile::Get_Entry(const char* section, int index) const {
  const IniSection* secptr = Find_Section(section);

  if (secptr != nullptr && index >= 0 &&
      index < std::ssize(secptr->EntryList)) {
    return secptr->EntryList.at(base::ToSize(index))->Entry.c_str();
  }
  return nullptr;
}

bool IniFile::Put_UUBlock(const char* section, std::span<const std::byte> block,
                          int len) {
  if (section == nullptr || len < 1 || std::cmp_greater(len, block.size())) {
    return false;
  }

  Invalidate_Message_Digest();
  Clear(section);

  SpanSource straw(block.first(base::ToSize(len)));
  Base64Source bstraw(Base64Mode::kEncode, straw);

  int counter = 1;

  for (;;) {
    char buffer[71];
    char sbuffer[32];

    const auto length = static_cast<int>(bstraw.Read(
        std::as_writable_bytes(std::span(buffer).first(sizeof(buffer) - 1))));
    base::At(buffer, length) = '\0';
    if (length == 0) {
      break;
    }

    absl::SNPrintF(sbuffer, sizeof(sbuffer), "%d", counter);
    Put_String(section, sbuffer, buffer);
    counter++;
  }
  return true;
}

int IniFile::Get_UUBlock(const char* section, std::span<std::byte> block,
                         int len) const {
  if (section == nullptr) {
    return 0;
  }

  if (len < 0 || std::cmp_greater(len, block.size())) {
    return 0;
  }
  SpanSink bpipe(block.first(base::ToSize(len)));
  Base64Sink b64pipe(Base64Mode::kDecode, bpipe);

  const int counter = Entry_Count(section);
  for (int index = 0; index < counter; index++) {
    char buffer[128];

    const int length = Get_String(section, Get_Entry(section, index), "=",
                                  buffer, sizeof(buffer));
    b64pipe.Write(std::as_bytes(std::span(buffer).first(base::ToSize(length))));
  }
  b64pipe.Finish();
  return static_cast<int>(bpipe.bytes_written());
}

bool IniFile::Put_TextBlock(const char* section, const char* text) {
  if (section == nullptr) {
    return false;
  }

  Invalidate_Message_Digest();
  Clear(section);
  std::string_view remaining(text == nullptr ? "" : text);

  int index = 1;
  while (!remaining.empty()) {
    char buffer[128];

    base::SafeCopy(std::span(buffer).first(76), remaining);

    char b[32];
    absl::SNPrintF(b, sizeof(b), "%d", index);

    // A full buffer may have cut a word: break the line at its last blank.
    int count = static_cast<int>(std::string_view(buffer).size());
    if (count > 0) {
      if (count >= 75) {
        while (count) {
          const char c = base::At(buffer, count);

          if (isspace(c)) {
            break;
          }
          count--;
        }

        if (count == 0) {
          break;
        }
        base::At(buffer, count) = '\0';
      }

      strtrim(base::MutableCString(buffer));
      Put_String(section, b, buffer);
      index++;
      remaining.remove_prefix(base::ToSize(count));
    } else {
      break;
    }
  }
  return true;
}

int IniFile::Get_TextBlock(const char* section, std::span<char> buffer,
                           int len) const {
  if (len <= 0 || std::cmp_greater(len, buffer.size())) {
    return 0;
  }

  base::At(buffer, 0) = '\0';
  if (len <= 1) {
    return 0;
  }

  const int elen = Entry_Count(section);
  int total = 0;
  for (int index = 0; index < elen; index++) {
    // One space stands for each line break Put_TextBlock() made.
    if (index > 0) {
      base::At(buffer, 0) = ' ';
      buffer = buffer.subspan(1);
      len--;
      total++;
    }

    Get_String(section, Get_Entry(section, index), "", buffer, len);

    const int partial =
        static_cast<int>(std::string_view(buffer.data()).size());
    total += partial;
    buffer = buffer.subspan(base::ToSize(partial));
    len -= partial;
    if (len <= 1) {
      break;
    }
  }
  return total;
}

bool IniFile::Put_Int(const char* section, const char* entry, int number,
                      int format) {
  char buffer[32];

  switch (format) {
    default:
    case 0:
      absl::SNPrintF(buffer, sizeof(buffer), "%d", number);
      break;

    case 1:
      absl::SNPrintF(buffer, sizeof(buffer), "%Xh",
                     static_cast<unsigned int>(number));
      break;

    case 2:
      absl::SNPrintF(buffer, sizeof(buffer), "$%X",
                     static_cast<unsigned int>(number));
      break;
  }
  return Put_String(section, entry, buffer);
}

int IniFile::Get_Int(const char* section, const char* entry,
                     int defvalue) const {
  if (section == nullptr || entry == nullptr) {
    return defvalue;
  }

  const IniEntry* entryptr = Find_Entry(section, entry);
  if (entryptr) {
    return base::ParseIniIntegerOr(entryptr->Value, defvalue);
  }
  return defvalue;
}

bool IniFile::Put_Hex(const char* section, const char* entry, int number) {
  char buffer[32];

  absl::SNPrintF(buffer, sizeof(buffer), "%X",
                 static_cast<unsigned int>(number));
  return Put_String(section, entry, buffer);
}

int IniFile::Get_Hex(const char* section, const char* entry,
                     int defvalue) const {
  if (section == nullptr || entry == nullptr) {
    return defvalue;
  }

  const IniEntry* entryptr = Find_Entry(section, entry);
  if (entryptr) {
    if (const auto value = base::ParseHex<uint32_t>(entryptr->Value)) {
      return static_cast<int>(*value);
    }
  }
  return defvalue;
}

bool IniFile::Put_String(const char* section, const char* entry,
                         const char* string) {
  if (section == nullptr || entry == nullptr) {
    return false;
  }

  Invalidate_Message_Digest();
  IniSection* secptr = Find_Section(section);

  if (secptr == nullptr) {
    secptr =
        SectionList.emplace_back(std::make_unique<IniSection>(section)).get();
    SectionIndex.try_emplace(secptr->Index_ID(), secptr);
  }

  // Remove the old entry and any duplicate of it.
  secptr->Remove_Entries(Name_ID(entry));

  // An empty value is not stored: Load() would drop the line anyway.
  if (string != nullptr && !std::string_view(string).empty()) {
    IniEntry* entryptr =
        secptr->EntryList
            .emplace_back(std::make_unique<IniEntry>(entry, string))
            .get();
    secptr->EntryIndex.try_emplace(entryptr->Index_ID(), entryptr);
  }
  return true;
}

int IniFile::Get_String(const char* section, const char* entry,
                        const char* defvalue, std::span<char> buffer,
                        int size) const {
  if (size <= 0 || std::cmp_greater(size, buffer.size())) {
    return 0;
  }
  buffer = buffer.first(base::ToSize(size));
  if (section == nullptr || entry == nullptr) {
    base::At(buffer, 0) = '\0';
    return 0;
  }
  const IniEntry* entryptr = Find_Entry(section, entry);
  if (entryptr) {
    defvalue = entryptr->Value.c_str();
  }
  // A caller may use its output as the default. Copy before clearing it.
  const std::string value = defvalue == nullptr ? "" : defvalue;
  base::SafeCopy(buffer, value.c_str());
  strtrim(buffer);
  return static_cast<int>(std::string_view(buffer.data()).size());
}

bool IniFile::Put_Bool(const char* section, const char* entry, bool value) {
  return Put_String(section, entry, value ? "yes" : "no");
}

bool IniFile::Get_Bool(const char* section, const char* entry,
                       bool defvalue) const {
  if (section == nullptr || entry == nullptr) {
    return defvalue;
  }

  IniEntry* entryptr = Find_Entry(section, entry);
  if (entryptr) {
    switch (toupper(entryptr->Value.at(0))) {
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
  return defvalue;
}

IniFile::IniEntry* IniFile::IniSection::Find_Entry(const char* entry) const {
  if (entry != nullptr) {
    const auto found = EntryIndex.find(Name_ID(entry));
    if (found != EntryIndex.end()) {
      return found->second;
    }
  }
  return nullptr;
}

IniLoadResult IniFile::LoadVerified(ByteStream& file) {
  StreamSource fs(file);
  return LoadVerified(fs);
}

IniLoadResult IniFile::LoadVerified(ByteSource& file) {
  if (!Load(file)) {
    return IniLoadResult::kFailed;
  }

  Sha1Digest embedded{};
  const int len = Get_UUBlock("Digest", base::ObjectBytes(embedded),
                              static_cast<int>(embedded.size()));
  if (len <= 0) {
    return IniLoadResult::kLoaded;
  }

  // The digest covers the data without its own section.
  Clear("Digest");
  Calculate_Message_Digest();
  return embedded == Digest ? IniLoadResult::kLoaded
                            : IniLoadResult::kDigestMismatch;
}

bool IniFile::SaveWithDigest(ByteStream& file) const {
  StreamSink fp(file);
  return SaveWithDigest(fp);
}

// The digest section is built in a scratch database and saved after the data,
// which is what appending it to this one would produce.
bool IniFile::SaveWithDigest(ByteSink& pipe) const {
  Calculate_Message_Digest();
  IniFile digest_section;
  digest_section.Put_UUBlock("Digest", base::ObjectBytes(Digest),
                             static_cast<int>(Digest.size()));
  DCHECK(!Section_Present("Digest"));
  WriteSections(pipe);
  digest_section.WriteSections(pipe);
  return pipe.Finish();
}

int IniFile::Get_Unique_ID() const {
  if (!IsDigestPresent) {
    Calculate_Message_Digest();
  }

  CrcEngine crc;
  for (const std::byte byte : Digest) {
    crc.Update(std::to_integer<uint8_t>(byte));
  }
  return static_cast<int>(crc.Value());
}

void IniFile::Calculate_Message_Digest() const {
  NullSink discard;
  Sha1Sink sha(discard);
  Save(sha);
  Digest = sha.digest();
  IsDigestPresent = true;
}

// The index slot goes first so no pointer to a destroyed entry is compared.
// Names are matched by CRC, the same key the index looks them up by.
void IniFile::IniSection::Remove_Entries(uint32_t id) {
  EntryIndex.erase(id);
  std::erase_if(EntryList,
                [id](const auto& owned) { return owned->Index_ID() == id; });
}

// Same as IniSection::Remove_Entries, for sections.
void IniFile::Remove_Sections(uint32_t id) {
  SectionIndex.erase(id);
  std::erase_if(SectionList,
                [id](const auto& owned) { return owned->Index_ID() == id; });
}

bool IniFile::Put_PKey(const PKey& key) {
  char buffer[512];

  int len = key.Encode_Modulus(base::ObjectBytes(buffer));
  Put_UUBlock("PublicKey", base::ObjectBytes(buffer), len);

  len = key.Encode_Exponent(base::ObjectBytes(buffer));
  Put_UUBlock("PrivateKey", base::ObjectBytes(buffer), len);
  return true;
}

PKey IniFile::Get_PKey(bool fast) const {
  PKey key;
  unsigned char buffer[512];

  // The fast key's exponent is a known constant, so a database published
  // without [PrivateKey] still yields it.
  if (fast) {
    const BigInt exp = PKey::Fast_Exponent();
    const auto length = exp.DEREncode(buffer);
    key.Decode_Exponent(base::ObjectBytes(buffer).first(base::ToSize(length)));
  } else {
    const int length =
        Get_UUBlock("PrivateKey", base::ObjectBytes(buffer), sizeof(buffer));
    key.Decode_Exponent(base::ObjectBytes(buffer).first(base::ToSize(length)));
  }

  const int length =
      Get_UUBlock("PublicKey", base::ObjectBytes(buffer), sizeof(buffer));
  key.Decode_Modulus(base::ObjectBytes(buffer).first(base::ToSize(length)));

  return key;
}

Fixed IniFile::Get_Fixed(const char* section, const char* entry,
                         Fixed defvalue) const {
  char buffer[128];
  Fixed retval = defvalue;

  if (Get_String(section, entry, "", buffer, sizeof(buffer))) {
    retval = Fixed::FromString(buffer);
  }
  return retval;
}

bool IniFile::Put_Fixed(const char* section, const char* entry, Fixed value) {
  return Put_String(section, entry, value.ToString().c_str());
}

std::string_view IniFile::Strip_Comments(std::string_view line) {
  return absl::StripTrailingAsciiWhitespace(line.substr(0, line.find(';')));
}

}  // namespace engine::file
