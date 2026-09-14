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

// MIX file archive format. MIX files are Westwood's archive format that
// bundles game assets (sprites, sounds, etc.) into single files. Files are
// indexed by CRC of their filename for fast O(log n) lookup.
//
// MIX files come in two formats:
// - Plain: FileHeader + FileEntry[] + raw data
// - Extended: metadata flags + optional PK-encrypted header + optional SHA-1
//   digest
//
// Example:
//   MixArchive::Register("GENERAL.MIX");  // Creates and registers in global
//   list MixArchive::Cache("GENERAL.MIX");     // Load into RAM void* data =
//   MixArchive::Retrieve("MOUSE.SHP");

#ifndef CNC_RED_ALERT_TECH_MIX_ARCHIVE_H_
#define CNC_RED_ALERT_TECH_MIX_ARCHIVE_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "tech/listnode.h"
#include "tech/pk.h"

// An archive is opened, and its files are served, through GameFile, so an
// archive packed inside another registered archive works too.
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class MixArchive : public Node<MixArchive> {
 public:
  // Result of looking up a file in the mixfile system.
  struct FileLocation {
    // View into cached data (empty if not cached).
    std::span<const std::byte> data;

    // The mixfile containing this file.
    MixArchive* mixfile;

    // Offset of the file from the start of the mixfile (if uncached) or of the
    // cached data (if cached).
    std::int32_t offset = 0;

    // Size of the embedded file.
    std::int32_t size = 0;
  };

  ~MixArchive() override;

  // Delete copy/move to prevent slicing or list corruption.
  MixArchive(const MixArchive&) = delete;
  MixArchive& operator=(const MixArchive&) = delete;
  MixArchive(MixArchive&&) = delete;
  MixArchive& operator=(MixArchive&&) = delete;

  [[nodiscard]] const std::string& Filename() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return filename_;
  }

  static bool Free(std::string_view filename);
  void Free();
  bool Cache();
  static bool Cache(std::string_view filename);
  static std::optional<FileLocation> Offset(std::string_view filename);

  // Returns cached file data as a span, or empty span if not found/not cached.
  static std::span<const std::byte> RetrieveData(std::string_view filename);

  // Legacy API: returns raw pointer for backward compatibility.
  static const void* Retrieve(std::string_view filename);

  // Factory: returns existing instance if already registered, otherwise
  // creates a new MixArchive and adds it to the global list.
  static MixArchive* Register(std::string_view filename,
                              const PKey* key = nullptr);

  // Removes and deletes a mixfile by name. Returns true if found.
  static bool Unregister(std::string_view filename);

  // Deletes all registered mixfiles.
  static void Free_All();

  // Index entry for an embedded file within the mixfile.
  struct FileEntry {
    std::int32_t crc;     // CRC of the filename (lookup key).
    std::int32_t offset;  // Offset from start of data section.
    std::int32_t size;    // Size of the embedded file.

    // Default spaceship operator for easy comparison
    auto operator<=>(const FileEntry& other) const = default;
    // Comparison with raw CRC for binary search projections
    auto operator<=>(std::int32_t other_crc) const { return crc <=> other_crc; }
  };

 private:
  MixArchive() = default;

  // Opens and parses the MIX file. Returns true on success.
  // For encrypted MIX files, provide key; for plain MIX files, may be nullptr.
  bool Open(std::string_view filename, const PKey* key);

  // On-disk file header format.
#pragma pack(push, 1)
  struct FileHeader {
    std::int16_t count;
    std::int32_t size;
  };
#pragma pack(pop)

  static MixArchive* Finder(std::string_view filename);

  std::string filename_;

  bool has_digest_ = false;    // True if mixfile has an attached SHA-1 digest.
  bool is_encrypted_ = false;  // True if the file header is encrypted.

  std::int32_t data_size_ = 0;   // Total size of embedded data.
  std::int32_t data_start_ = 0;  // File offset where raw data begins.

  std::vector<FileEntry> file_index_;  // Sorted by CRC.
  std::vector<std::byte> data_;        // Cached file data.

  // Global registry of all open mixfiles.
  inline static List<MixArchive> MixList;
};

#endif  // CNC_RED_ALERT_TECH_MIX_ARCHIVE_H_
