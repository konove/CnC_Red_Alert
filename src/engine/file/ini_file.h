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
//     const int credits = ini.GetInt("Basic", "Credits", 0);
//   }
class IniFile {
 public:
  IniFile() = default;
  ~IniFile();
  IniFile(const IniFile&) = delete;
  IniFile& operator=(const IniFile&) = delete;
  IniFile(IniFile&&) = delete;
  IniFile& operator=(IniFile&&) = delete;

  // Adds the sections read from the stream to the database; it is not cleared
  // first. Text before the first section header is skipped, and a section
  // without entries is not stored. Returns false when the data holds no
  // section header at all.
  bool Load(ByteStream& stream);
  bool Load(ByteSource& source);
  // Writes every section as "[name]", its "entry=value" lines and a blank
  // line, with CR LF line ends, and finishes the sink. Returns whether every
  // byte was accepted.
  bool Save(ByteStream& stream) const;
  bool Save(ByteSink& sink) const;

  // Loads the database like Load(). A [Digest] section, if present, is
  // removed and compared with the SHA-1 of the remaining data; the data is
  // kept either way.
  IniLoadResult LoadVerified(ByteStream& stream);
  IniLoadResult LoadVerified(ByteSource& source);
  // Saves the database followed by a [Digest] section holding the SHA-1 of the
  // data. The database itself is left unchanged; it must not already hold a
  // [Digest] section (LoadVerified() removes one that decodes to a non-empty
  // block).
  bool SaveWithDigest(ByteStream& stream) const;
  bool SaveWithDigest(ByteSink& sink) const;

  // A CRC of the data's SHA-1; equal for equal databases.
  [[nodiscard]] int UniqueId() const;

  // Remove everything, one section, or one entry of a section, with every
  // duplicate of the name. A name that is not found removes nothing. They
  // always return true.
  bool Clear();
  bool Clear(std::string_view section);
  bool Clear(std::string_view section, std::string_view entry);

  // True once the database holds at least one section.
  [[nodiscard]] bool is_loaded() const { return !sections_.empty(); }

  [[nodiscard]] bool HasSection(std::string_view section) const {
    return FindSection(section) != nullptr;
  }
  [[nodiscard]] bool HasEntry(std::string_view section,
                              std::string_view entry) const {
    return FindEntry(section, entry) != nullptr;
  }

  // The number of sections, duplicates counted.
  [[nodiscard]] int SectionCount() const;
  // Returns the name of section `index` in file order, or nullptr when out of
  // range.
  [[nodiscard]] const char* SectionName(int index) const
      [[clang::lifetimebound]];

  // The number of entries in `section`; 0 when there is no such section.
  [[nodiscard]] int EntryCount(std::string_view section) const;
  // Returns the name (the text left of the '=') of entry `index` of `section`
  // in file order, or nullptr when the section or the index does not exist.
  // The analysis cannot follow the name through the section index.
  [[nodiscard]] const char* EntryName(std::string_view section, int index) const
      // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
      [[clang::lifetimebound]];

  // The getters return the value of `entry` in `section`, or `default_value`
  // when the entry is missing or its text does not parse as the requested
  // type.

  // Returns the value without surrounding blanks; `default_value` is trimmed
  // the same way.
  [[nodiscard]] std::string GetString(std::string_view section,
                                      std::string_view entry,
                                      std::string_view default_value) const;
  // As above, copied into `buffer` and terminated; a value longer than the
  // buffer is cut. Returns the length stored. `default_value` may be a view
  // of `buffer`.
  int GetString(std::string_view section, std::string_view entry,
                std::string_view default_value, std::span<char> buffer) const;
  // Reads decimal, "$FF", "0FFh" and "0xFF" forms.
  [[nodiscard]] int GetInt(std::string_view section, std::string_view entry,
                           int default_value = 0) const;
  // Reads the value as hexadecimal digits, with or without a "0x" prefix.
  [[nodiscard]] int GetHex(std::string_view section, std::string_view entry,
                           int default_value = 0) const;
  // Decides by the first character: Y, T or 1 is true and N, F or 0 false, in
  // either case.
  [[nodiscard]] bool GetBool(std::string_view section, std::string_view entry,
                             bool default_value = false) const;
  // Reads "1.5" and "150%" forms. Text without digits reads as zero, not as
  // `default_value`.
  [[nodiscard]] Fixed GetFixed(std::string_view section, std::string_view entry,
                               Fixed default_value) const;
  // Joins the values of every entry of `section`, in order and separated by
  // single spaces, into `buffer`, terminated: the text PutTextBlock() stored.
  // Text that does not fit is dropped. Returns the length stored.
  int GetTextBlock(std::string_view section, std::span<char> buffer) const;
  // Decodes the base64 text PutBase64Block() stored in `section` into `block`
  // and returns the number of bytes decoded; data that does not fit is
  // dropped, so a full buffer may mean a truncated block.
  int GetBase64Block(std::string_view section,
                     std::span<std::byte> block) const;
  // Returns the key held in [PublicKey] (the modulus) and [PrivateKey] (the
  // exponent). With `fast`, the exponent is the well-known public one and
  // only the modulus is read.
  [[nodiscard]] PKey GetPublicKey(bool fast) const;

  // The setters store a value under `entry` in `section`, creating the
  // section and replacing an existing entry (and every duplicate of it); a new
  // entry goes last. They always return true.

  // An empty `value` removes the entry: the file format cannot hold an entry
  // without a value.
  bool PutString(std::string_view section, std::string_view entry,
                 std::string_view value);
  // `format` chooses the spelling, all of which GetInt() reads back: 0 is
  // decimal, 1 hexadecimal with a trailing 'h', 2 hexadecimal with a leading
  // '$'.
  bool PutInt(std::string_view section, std::string_view entry, int number,
              int format = 0);
  // Writes bare hexadecimal digits, which GetHex() reads back.
  bool PutHex(std::string_view section, std::string_view entry, int number);
  // Writes "yes" or "no".
  bool PutBool(std::string_view section, std::string_view entry, bool value);
  // Writes "1.5" style text, which GetFixed() reads back.
  bool PutFixed(std::string_view section, std::string_view entry, Fixed value);
  // Replaces `section` with `text` broken at blanks into lines of at most 75
  // characters, stored as the entries "1", "2", ... The section can hold
  // nothing else. A word of 75 characters or more ends the block: it and the
  // text after it are not stored. Always returns true.
  bool PutTextBlock(std::string_view section, std::string_view text);
  // Replaces `section` with `block` as base64 text in numbered entries of 70
  // characters. The section can hold nothing else. Returns false, leaving
  // the section alone, for an empty `block`.
  bool PutBase64Block(std::string_view section,
                      std::span<const std::byte> block);
  // Stores the modulus of `key` in [PublicKey] and its exponent in
  // [PrivateKey]. A database written this way holds the private key, so it
  // must not be shipped. Always returns true.
  bool PutPublicKey(const PKey& key);

 private:
  // One "entry=value" line.
  struct Entry {
    [[nodiscard]] uint32_t id() const { return NameId(name); }

    std::string name;
    std::string value;  // Never empty: such an entry is not stored.
  };

  // One "[section]" and the entries under it.
  struct Section {
    // Returns the first entry named `entry_name`, or nullptr.
    [[nodiscard]] const Entry* FindEntry(std::string_view entry_name) const;
    // Removes every entry whose name has the id `entry_id` (a file can hold
    // duplicates; the index only knows the first), so none is left to
    // resurface after the removal.
    void RemoveEntries(uint32_t entry_id);
    [[nodiscard]] uint32_t id() const { return NameId(name); }

    std::string name;
    std::vector<std::unique_ptr<Entry>> entries;        // In file order.
    absl::flat_hash_map<uint32_t, Entry*> entry_index;  // By NameId().
  };

  // Return the first section or entry of that name, or nullptr.
  [[nodiscard]] Section* FindSection(std::string_view section) const;
  [[nodiscard]] const Entry* FindEntry(std::string_view section,
                                       std::string_view entry) const;
  // Removes every section whose name has the id `section_id`, duplicates
  // included.
  void RemoveSections(uint32_t section_id);
  // The key of a section or entry name: its CRC without ASCII case, which is
  // what makes names compare without case.
  static uint32_t NameId(std::string_view name);
  // Writes every section to `sink` without finishing it.
  void WriteSections(ByteSink& sink) const;
  void ComputeDigest() const;

  std::vector<std::unique_ptr<Section>> sections_;         // In file order.
  absl::flat_hash_map<uint32_t, Section*> section_index_;  // By NameId().

  // The digest is a lazily computed cache of the database contents: const
  // queries such as UniqueId() fill it on first use, and every mutation
  // clears `digest_valid_`.
  mutable bool digest_valid_ = false;
  mutable Sha1Digest digest_{};
};

}  // namespace engine::file

#endif  // CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_
