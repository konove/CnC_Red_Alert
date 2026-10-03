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

// IniFile, the INI text database behind the rules, scenario and settings
// files. Originally Westwood's INIClass (INI.H, Joe L. Bostic, May 1996).

#ifndef CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_
#define CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "base/fixed.h"
#include "engine/crypto/crc.h"
#include "engine/crypto/pk.h"
#include "engine/crypto/sha.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/byte_stream.h"

namespace engine::file {

// The outcome of IniFile::LoadVerified().
enum class IniLoadResult {
  kFailed,          // Nothing usable was read.
  kLoaded,          // Read; the digest was absent or matched.
  kDigestMismatch,  // Read, but the embedded digest does not match the data.
};

// A database of named sections, each a list of `entry=value` lines, in the text
// format of Windows INI files: the game rules, the scenarios and the settings.
// Section and entry names compare without ASCII case; values are kept as text
// and converted on each read.
//
// A name that occurs twice is looked up as its first occurrence. Blank lines
// and comments (from ';' to the end of the line) are dropped on load, so a
// saved file holds only the data.
//
// Example:
//   IniFile ini;
//   if (ini.Load(stream)) {
//     const int credits = ini.Get_Int("Basic", "Credits", 0);
//   }
class IniFile {
 public:
  IniFile() = default;
  ~IniFile();
  IniFile(const IniFile&) = delete;
  IniFile& operator=(const IniFile&) = delete;
  IniFile(IniFile&&) = delete;
  IniFile& operator=(IniFile&&) = delete;

  // Adds the sections read from `file` to the database; it is not cleared
  // first. Text before the first section header is skipped, and a section
  // without entries is not stored. Returns false when the data holds no
  // section header at all.
  bool Load(ByteStream& file);
  bool Load(ByteSource& file);
  // Writes every section as "[name]", its "entry=value" lines and a blank
  // line, with CR LF line ends, and finishes the sink. Returns whether every
  // byte was accepted.
  bool Save(ByteStream& file) const;
  bool Save(ByteSink& pipe) const;

  // Loads the database like Load(). A [Digest] section, if present, is
  // removed and compared with the SHA-1 of the remaining data; the data is
  // kept either way.
  IniLoadResult LoadVerified(ByteStream& file);
  IniLoadResult LoadVerified(ByteSource& file);
  // Saves the database followed by a [Digest] section holding the SHA-1 of the
  // data. The database itself is left unchanged; it must not already hold a
  // [Digest] section (LoadVerified() removes one that decodes to a non-empty
  // block).
  bool SaveWithDigest(ByteStream& file) const;
  bool SaveWithDigest(ByteSink& pipe) const;

  // A CRC of the data's SHA-1; equal for equal databases.
  [[nodiscard]] int Get_Unique_ID() const;

  // Removes `entry` from `section`, the whole `section` when `entry` is
  // nullptr, or everything when `section` is nullptr too. A name that is not
  // found removes nothing. Always returns true.
  bool Clear(const char* section = nullptr, const char* entry = nullptr);

  int Line_Count(const char* section) const;
  // True once the database holds at least one section.
  [[nodiscard]] bool Is_Loaded() const { return !SectionList.empty(); }
  [[nodiscard]] int Size() const;
  // Whether `section` holds `entry`, or, with a null `entry`, whether
  // `section` exists.
  bool Is_Present(const char* section, const char* entry = nullptr) const {
    if (entry == nullptr) {
      return Find_Section(section) != nullptr;
    }
    return Find_Entry(section, entry) != nullptr;
  }

  // The number of sections, duplicates counted.
  [[nodiscard]] int Section_Count() const;
  // Returns the name of section `index` in file order, or nullptr when out of
  // range.
  [[nodiscard]] const char* Get_Section(int index) const
      [[clang::lifetimebound]];
  bool Section_Present(const char* section) const {
    return Find_Section(section) != nullptr;
  }

  // The number of entries in `section`; 0 when there is no such section.
  int Entry_Count(const char* section) const;
  // Returns the name (the text left of the '=') of entry `index` of `section`
  // in file order, or nullptr when the section or the index does not exist.
  const char* Get_Entry(const char* section, int index) const;

  // The getters return the value of `entry` in `section`, or `defvalue` when
  // the entry is missing, a name is null, or the text does not parse as the
  // requested type.

  // Copies the value, without surrounding blanks, into the first `size` chars
  // of `buffer` and terminates it; a longer value is cut. Returns the length
  // stored. `defvalue` may be null (an empty string) and may point into
  // `buffer`. A null name stores an empty string, not the default.
  int Get_String(const char* section, const char* entry, const char* defvalue,
                 std::span<char> buffer, int size) const;
  // Reads decimal, "$FF", "0FFh" and "0xFF" forms.
  int Get_Int(const char* section, const char* entry, int defvalue = 0) const;
  // Reads the value as hexadecimal digits, with or without a "0x" prefix.
  int Get_Hex(const char* section, const char* entry, int defvalue = 0) const;
  // Decides by the first character: Y, T or 1 is true and N, F or 0 false, in
  // either case.
  bool Get_Bool(const char* section, const char* entry,
                bool defvalue = false) const;
  // Joins the values of every entry of `section`, in order and separated by
  // single spaces, into the first `len` chars of `buffer`, terminated: the
  // text Put_TextBlock() stored. Returns the length stored.
  int Get_TextBlock(const char* section, std::span<char> buffer, int len) const;
  // Decodes the base64 text Put_UUBlock() stored in `section` into the first
  // `len` bytes of `block` and returns the number of bytes decoded; data
  // beyond `len` is dropped, so a full buffer may mean a truncated block.
  int Get_UUBlock(const char* section, std::span<std::byte> block,
                  int len) const;
  // Returns the key held in [PublicKey] (the modulus) and [PrivateKey] (the
  // exponent). With `fast`, the exponent is the well-known public one and
  // only the modulus is read.
  [[nodiscard]] PKey Get_PKey(bool fast) const;
  // Reads "1.5" and "150%" forms. Text without digits reads as zero, not as
  // `defvalue`.
  Fixed Get_Fixed(const char* section, const char* entry, Fixed defvalue) const;

  // The setters store a value under `entry` in `section`, creating the
  // section and replacing an existing entry (and every duplicate of it); a new
  // entry goes last. They return false, storing nothing, for a null name.

  // Writes "1.5" style text, which Get_Fixed() reads back.
  bool Put_Fixed(const char* section, const char* entry, Fixed value);
  // A null or empty `string` removes the entry: the file format cannot hold
  // an entry without a value.
  bool Put_String(const char* section, const char* entry, const char* string);
  // Writes bare hexadecimal digits, which Get_Hex() reads back.
  bool Put_Hex(const char* section, const char* entry, int number);
  // `format` chooses the spelling, all of which Get_Int() reads back: 0 is
  // decimal, 1 hexadecimal with a trailing 'h', 2 hexadecimal with a leading
  // '$'.
  bool Put_Int(const char* section, const char* entry, int number,
               int format = 0);
  // Writes "yes" or "no".
  bool Put_Bool(const char* section, const char* entry, bool value);
  // Replaces `section` with `text` broken at blanks into lines of at most 75
  // characters, stored as the entries "1", "2", ... The section can hold
  // nothing else. A word longer than a line ends the block: it and the text
  // after it are not stored.
  bool Put_TextBlock(const char* section, const char* text);
  // Replaces `section` with the first `len` bytes of `block` as base64 text
  // in numbered entries of 70 characters. The section can hold nothing else.
  // Returns false when `len` is not in 1..block.size().
  bool Put_UUBlock(const char* section, std::span<const std::byte> block,
                   int len);
  // Stores the modulus of `key` in [PublicKey] and its exponent in
  // [PrivateKey]. A database written this way holds the private key, so it
  // must not be shipped.
  bool Put_PKey(const PKey& key);

 protected:
  // One "entry=value" line.
  struct IniEntry {
    explicit IniEntry(const char* entry = "", const char* value = "")
        : Entry(entry), Value(value) {}
    ~IniEntry();
    IniEntry(const IniEntry&) = delete;
    IniEntry& operator=(const IniEntry&) = delete;
    IniEntry(IniEntry&&) = delete;
    IniEntry& operator=(IniEntry&&) = delete;
    [[nodiscard]] uint32_t Index_ID() const { return Name_ID(Entry); }

    std::string Entry;
    std::string Value;  // Never empty: such an entry is not stored.
  };

  // One "[section]" and the entries under it.
  struct IniSection {
    explicit IniSection(const char* section) : Section(section) {}
    ~IniSection();
    IniSection(const IniSection&) = delete;
    IniSection& operator=(const IniSection&) = delete;
    IniSection(IniSection&&) = delete;
    IniSection& operator=(IniSection&&) = delete;
    // Returns the first entry named `entry`, or nullptr.
    IniEntry* Find_Entry(const char* entry) const;
    // Removes every entry whose name has index id `id` (a file can hold
    // duplicates; the index only knows the first), so none is left to
    // resurface after the removal.
    void Remove_Entries(uint32_t id);
    [[nodiscard]] uint32_t Index_ID() const { return Name_ID(Section); }

    std::string Section;
    std::vector<std::unique_ptr<IniEntry>> EntryList;     // In file order.
    absl::flat_hash_map<uint32_t, IniEntry*> EntryIndex;  // By CRC of the name.
  };

  // Return the first section or entry of that name, or nullptr when there is
  // none or a name is null.
  IniSection* Find_Section(const char* section) const;
  IniEntry* Find_Entry(const char* section, const char* entry) const;
  // Removes every section whose name has index id `id`, duplicates included.
  void Remove_Sections(uint32_t id);
  // The key of a section or entry name: names compare without ASCII case.
  static uint32_t Name_ID(std::string_view name);
  // Returns `line` up to its first ';', without trailing blanks.
  static std::string_view Strip_Comments(std::string_view line);

  std::vector<std::unique_ptr<IniSection>> SectionList;  // In file order.

  absl::flat_hash_map<uint32_t, IniSection*>
      SectionIndex;  // By CRC of the name.

 private:
  // Writes every section to `pipe` without finishing it.
  void WriteSections(ByteSink& pipe) const;
  void Calculate_Message_Digest() const;
  void Invalidate_Message_Digest() { IsDigestPresent = false; }

  // The digest is a lazily computed cache of the database contents: const
  // queries such as Get_Unique_ID() fill it on first use, and every mutation
  // invalidates it.
  mutable bool IsDigestPresent : 1 {false};
  mutable Sha1Digest Digest{};
};

}  // namespace engine::file

#endif  // CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_
