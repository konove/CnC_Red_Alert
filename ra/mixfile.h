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
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ra/ccfile.h"  // IWYU pragma: keep
#include "ra/listnode.h"
#include "tech/pk.h"

template <class T>
class MixFileClass : public Node<MixFileClass<T>> {
 public:
  // Result of looking up a file in the mixfile system.
  struct FileLocation {
    // View into cached data (empty if not cached).
    std::span<const std::byte> data;

    // The mixfile containing this file.
    MixFileClass* mixfile;

    // Absolute file offset (if uncached) or relative (if cached).
    std::int32_t offset;

    // Size of the embedded file.
    std::int32_t size;
  };

  MixFileClass(std::string_view filename, const PKey* key);
  ~MixFileClass() override;

  // Delete copy/move to prevent slicing or list corruption.
  MixFileClass(const MixFileClass&) = delete;
  MixFileClass& operator=(const MixFileClass&) = delete;
  MixFileClass(MixFileClass&&) = delete;
  MixFileClass& operator=(MixFileClass&&) = delete;

  [[nodiscard]] const std::string& Filename() const { return filename_; }

  static bool Free(std::string_view filename);
  void Free();
  bool Cache();
  static bool Cache(std::string_view filename);
  static std::optional<FileLocation> Offset(std::string_view filename);
  static const void* Retrieve(std::string_view filename);

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
  // On-disk file header format.
#pragma pack(push, 1)
  struct FileHeader {
    std::int16_t count;
    std::int32_t size;
  };
#pragma pack(pop)

  static MixFileClass* Finder(std::string_view filename);

  std::string filename_;

  bool has_digest_ = false;    // True if mixfile has an attached SHA-1 digest.
  bool is_encrypted_ = false;  // True if the file header is encrypted.

  std::int32_t data_size_ = 0;   // Total size of embedded data.
  std::int32_t data_start_ = 0;  // File offset where raw data begins.

  std::vector<FileEntry> file_index_;  // Sorted by CRC.
  std::vector<std::byte> data_;        // Cached file data.

  // Global registry of all open mixfiles.
  // Note: In C++23, consider wrapping this in a singleton accessor to avoid
  // static initialization order fiasco, but keeping as-is for architectural
  // consistency.
  inline static List<MixFileClass> MixList;
};
