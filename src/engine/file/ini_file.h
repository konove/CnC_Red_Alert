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

/* $Header: /CounterStrike/INI.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INI.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/15/96 *
 *                                                                                             *
 *                  Last Update : May 15, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_
#define CNC_RED_ALERT_ENGINE_FILE_INI_FILE_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <span>
#include <string>
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

/*
**	This is an INI database handler class. It handles a database with a disk
*format identical *	to the INI files commonly used by Windows.
*/
class IniFile {
 public:
  IniFile() = default;
  ~IniFile();
  IniFile(const IniFile&) = delete;
  IniFile& operator=(const IniFile&) = delete;
  IniFile(IniFile&&) = delete;
  IniFile& operator=(IniFile&&) = delete;

  /*
  **	Fetch and store INI data.
  */
  bool Load(ByteStream& file);
  bool Load(ByteSource& file);
  bool Save(ByteStream& file) const;
  bool Save(ByteSink& pipe) const;

  // Loads the database like Load(). A [Digest] section, if present, is
  // removed and compared with the SHA-1 of the remaining data; the data is
  // kept either way.
  IniLoadResult LoadVerified(ByteStream& file);
  IniLoadResult LoadVerified(ByteSource& file);
  // Saves the database followed by a [Digest] section holding the SHA-1 of the
  // data. The database itself is left unchanged; it must not already hold a
  // [Digest] section, which LoadVerified() always removes.
  bool SaveWithDigest(ByteStream& file) const;
  bool SaveWithDigest(ByteSink& pipe) const;

  // A CRC of the data's SHA-1; equal for equal databases.
  [[nodiscard]] int Get_Unique_ID() const;

  /*
  **	Erase all data within this INI file manager.
  */
  bool Clear(const char* section = nullptr, const char* entry = nullptr);

  int Line_Count(const char* section) const;
  [[nodiscard]] bool Is_Loaded() const { return !SectionList.empty(); }
  [[nodiscard]] int Size() const;
  bool Is_Present(const char* section, const char* entry = nullptr) const {
    if (entry == nullptr) {
      return Find_Section(section) != nullptr;
    }
    return Find_Entry(section, entry) != nullptr;
  }

  /*
  **	Fetch the number of sections in the INI file or verify if a specific
  **	section is present.
  */
  [[nodiscard]] int Section_Count() const;
  bool Section_Present(const char* section) const {
    return Find_Section(section) != nullptr;
  }

  /*
  **	Fetch the number of entries in a section or get a particular entry in a
  *section.
  */
  int Entry_Count(const char* section) const;
  const char* Get_Entry(const char* section, int index) const;

  /*
  **	Get the various data types from the section and entry specified.
  */
  int Get_String(const char* section, const char* entry, const char* defvalue,
                 std::span<char> buffer, int size) const;
  int Get_Int(const char* section, const char* entry, int defvalue = 0) const;
  int Get_Hex(const char* section, const char* entry, int defvalue = 0) const;
  bool Get_Bool(const char* section, const char* entry,
                bool defvalue = false) const;
  int Get_TextBlock(const char* section, std::span<char> buffer, int len) const;
  int Get_UUBlock(const char* section, std::span<std::byte> block,
                  int len) const;
  [[nodiscard]] PKey Get_PKey(bool fast) const;
  Fixed Get_Fixed(const char* section, const char* entry, Fixed defvalue) const;

  /*
  **	Put a data type to the section and entry specified.
  */
  bool Put_Fixed(const char* section, const char* entry, Fixed value);
  bool Put_String(const char* section, const char* entry, const char* string);
  bool Put_Hex(const char* section, const char* entry, int number);
  bool Put_Int(const char* section, const char* entry, int number,
               int format = 0);
  bool Put_Bool(const char* section, const char* entry, bool value);
  bool Put_TextBlock(const char* section, const char* text);
  bool Put_UUBlock(const char* section, std::span<const std::byte> block,
                   int len);
  bool Put_PKey(const PKey& key);

 protected:
  static constexpr int kMaxLineLength = 128;

  /*
  **	The value entries for the INI file are stored as objects of this type.
  **	The entry identifier and value string are combined into this object.
  */
  struct IniEntry {
    explicit IniEntry(const char* entry = "", const char* value = "")
        : Entry(entry), Value(value) {}
    ~IniEntry();
    IniEntry(const IniEntry&) = delete;
    IniEntry& operator=(const IniEntry&) = delete;
    IniEntry(IniEntry&&) = delete;
    IniEntry& operator=(IniEntry&&) = delete;
    [[nodiscard]] uint32_t Index_ID() const {
      return CrcEngine::Compute(Entry);
    }

    std::string Entry;
    std::string Value;
  };

  /*
  **	Each section (bracketed) is represented by an object of this type. All
  *entries *	subordinate to this section are attached.
  */
  struct IniSection {
    explicit IniSection(const char* section) : Section(section) {}
    ~IniSection();
    IniSection(const IniSection&) = delete;
    IniSection& operator=(const IniSection&) = delete;
    IniSection(IniSection&&) = delete;
    IniSection& operator=(IniSection&&) = delete;
    IniEntry* Find_Entry(const char* entry) const;
    // Removes every entry whose name has index id `id` (a file can hold
    // duplicates; the index only knows the first), so none is left to
    // resurface after the removal.
    void Remove_Entries(uint32_t id);
    [[nodiscard]] uint32_t Index_ID() const {
      return CrcEngine::Compute(Section);
    }

    std::string Section;
    std::vector<std::unique_ptr<IniEntry>> EntryList;     // In file order.
    absl::flat_hash_map<uint32_t, IniEntry*> EntryIndex;  // By CRC of the name.
  };

  /*
  **	Utility routines to help find the appropriate section and entry objects.
  */
  IniSection* Find_Section(const char* section) const;
  IniEntry* Find_Entry(const char* section, const char* entry) const;
  // Removes every section whose name has index id `id`, duplicates included.
  void Remove_Sections(uint32_t id);
  static void Strip_Comments(std::span<char> buffer);

  /*
  **	This is the list of all sections within this INI file.
  */
  std::vector<std::unique_ptr<IniSection>> SectionList;  // In file order.

  absl::flat_hash_map<uint32_t, IniSection*>
      SectionIndex;  // By CRC of the name.

 private:
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
