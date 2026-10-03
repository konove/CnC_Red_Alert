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
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "base/fixed.h"
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
// saved file holds only the data. Load() keeps reading past a NUL byte;
// callers that want the legacy "text ends at the first NUL" rule load from
// ReadIniText() or ReadIniFile().
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
  // without entries is not stored. The last line need not end in a line feed.
  // Returns false, adding nothing, when no section header line ends in a line
  // feed: for empty data and for "[S]" alone. Otherwise returns true, even
  // when no section was stored: "[S]\n" loads as true with zero sections.
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

  [[nodiscard]] bool HasSection(const std::string_view section) const {
    return FindSection(section) != nullptr;
  }
  [[nodiscard]] bool HasEntry(const std::string_view section,
                              const std::string_view entry) const {
    return FindEntry(section, entry) != nullptr;
  }

  // The number of sections, duplicates counted.
  [[nodiscard]] int SectionCount() const;
  // Returns the name of section `index` in file order, or an empty view when
  // out of range. The view is valid until the database is next changed.
  [[nodiscard]] std::string_view SectionName(int index) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // The number of entries in `section`; 0 when there is no such section.
  [[nodiscard]] int EntryCount(std::string_view section) const;
  // Returns the name (the text left of the '=') of entry `index` of `section`
  // in file order, or an empty view when the section or the index does not
  // exist. The view is valid until the database is next changed.
  [[nodiscard]] std::string_view EntryName(
      std::string_view section, int index) const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // The getters return the value of `entry` in `section`, or `default_value`
  // when the entry is missing or its text does not parse as the requested
  // type.

  // Returns the value without surrounding blanks; `default_value` is trimmed
  // the same way.
  [[nodiscard]] std::string GetString(
      std::string_view section, std::string_view entry,
      std::string_view default_value = "") const;
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
  // Returns the values of every entry of `section`, in order and joined by
  // single spaces: the text PutTextBlock() stored. Empty when there is no such
  // section.
  [[nodiscard]] std::string GetTextBlock(std::string_view section) const;
  // Returns the bytes decoded from the base64 text PutBase64Block() stored in
  // `section`; empty when there is no such section.
  [[nodiscard]] std::vector<std::byte> GetBase64Block(
      std::string_view section) const;
  // Returns the key held in [PublicKey] (the modulus) and [PrivateKey] (the
  // exponent). With `fast`, the exponent is the well-known public one and
  // only the modulus is read.
  [[nodiscard]] PKey GetPublicKey(bool fast) const;

  // The setters store a value under `entry` in `section`, creating the
  // section and replacing an existing entry (and every duplicate of it); a new
  // entry goes last. They always return true.

  // An empty `value` removes the entry: the file format cannot hold an entry
  // without a value. The names and the value are stored as given, but not all
  // text reads back after Save() and Load(): a value containing ';' is cut
  // there, leading and trailing blanks are trimmed, a line break ends the
  // value, and a name containing '=' or starting with '[' does not survive.
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

 private:
  // Hash and equality of section and entry names, which compare without
  // ASCII case: both fold each character the same way.
  struct NameHash {
    using is_transparent = void;
    [[nodiscard]] std::size_t operator()(std::string_view name) const;
  };
  struct NameEqual {
    using is_transparent = void;
    [[nodiscard]] bool operator()(std::string_view left,
                                  std::string_view right) const;
  };
  // Finds the first section or entry of a name. A key is a view of the name
  // stored in the element it maps to, so an index slot must be erased before
  // its element is destroyed.
  template <class Element>
  using NameIndex =
      absl::flat_hash_map<std::string_view, Element*, NameHash, NameEqual>;

  // One "entry=value" line.
  struct Entry {
    std::string name;
    std::string value;  // Never empty: such an entry is not stored.
  };

  // One "[section]" and the entries under it.
  struct Section {
    // Returns the first entry named `entry_name`, or nullptr.
    [[nodiscard]] const Entry* FindEntry(std::string_view entry_name) const;
    // Removes every entry named `entry_name` (a file can hold duplicates; the
    // index only knows the first), so none is left to resurface after the
    // removal.
    void RemoveEntries(std::string_view entry_name);

    std::string name;
    std::vector<std::unique_ptr<Entry>> entries;  // In file order.
    NameIndex<Entry> entry_index;
  };

  // Return the first section or entry of that name, or nullptr.
  [[nodiscard]] Section* FindSection(std::string_view section) const
      // The index holds plain pointers to the sections `sections_` owns, which
      // the analysis cannot connect to this object.
      // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  [[nodiscard]] const Entry* FindEntry(std::string_view section,
                                       std::string_view entry) const;
  // Removes every section named `section`, duplicates included.
  void RemoveSections(std::string_view section);
  // Writes every section to `sink` without finishing it.
  void WriteSections(ByteSink& sink) const;
  void ComputeDigest() const;

  std::vector<std::unique_ptr<Section>> sections_;  // In file order.
  NameIndex<Section> section_index_;

  // The digest is a lazily computed cache of the database contents: const
  // queries such as UniqueId() fill it on first use, and every mutation
  // clears `digest_valid_`.
  mutable bool digest_valid_ = false;
  mutable Sha1Digest digest_{};
};

// Returns the text of stream from its current position to its end, or up to
// its first NUL, which ends INI text for the legacy readers.
std::string ReadIniText(ByteStream& stream);

// Returns the text of the game file name (see OpenGameFile and ReadIniText),
// or nullopt if it is found nowhere.
std::optional<std::string> ReadIniFile(std::string_view name);

}  // namespace engine::file

#endif  // CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_
