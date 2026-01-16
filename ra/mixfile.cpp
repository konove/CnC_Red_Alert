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

// MIX file archive format implementation. MIX files are Westwood's archive
// format that bundles game assets (sprites, sounds, etc.) into single files.
// Files are indexed by CRC of their filename for fast lookup.

#include "mixfile.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <new>
#include <span>
#include <string_view>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "ra/ccfile.h"
#include "ra/compat.h"
#include "ra/conquer.h"
#include "ra/externs.h"
#include "ra/startup.h"
#include "tech/crc.h"
#include "tech/pkstraw.h"
#include "tech/shastraw.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

template <class T>
MixFileClass<T>::MixFileClass(std::string_view filename, const PKey* key) {
  if (!Force_CD_Available(RequiredCD)) {
    Emergency_Exit(EXIT_FAILURE);
  }

  T file(std::string(filename).c_str());
  filename_ = file.File_Name();

  FileStraw file_straw(file);
  PKStraw pk_straw(PKStraw::DECRYPT, CryptRandom);
  Straw* straw = &file_straw;

  if (!file.Is_Available()) {
    return;
  }

  FileHeader file_header{};
  struct MixMetadata {
    std::int16_t First;   // Zero indicates extended format.
    std::int16_t Second;  // Bit 0: has digest, Bit 1: encrypted.
  } alternate{};

  // Read initial metadata to determine format
  straw->Get(&alternate, sizeof(alternate));

  if (alternate.First == 0) {
    // Extended Format
    has_digest_ = (alternate.Second & 0x01) != 0;
    is_encrypted_ = (alternate.Second & 0x02) != 0;

    if (is_encrypted_) {
      pk_straw.Key(key);
      pk_straw.Get_From(&file_straw);
      straw = &pk_straw;
    }

    straw->Get(&file_header, sizeof(file_header));
  } else {
    // Plain Format: The bytes read into 'alternate' are actually the start of
    // FileHeader standard layout allows memcpy, though strictly
    // reinterpret_cast is valid here due to packing.
    std::memcpy(&file_header, &alternate, sizeof(alternate));

    // Read the remainder of the header
    char* header_ptr = reinterpret_cast<char*>(&file_header);
    straw->Get(header_ptr + sizeof(alternate),
               sizeof(file_header) - sizeof(alternate));
  }

  data_size_ = file_header.size;

  // Resize index and read entries
  file_index_.resize(file_header.count);
  straw->Get(file_index_.data(), file_index_.size() * sizeof(FileEntry));

  // Calculate start position.
  // Seek returns long, cast to int32_t to match class member (assuming < 2GB
  // files)
  data_start_ =
      static_cast<std::int32_t>(file.Seek(0, SEEK_CUR) + file.BiasStart);

  MixList.Add_Tail(this);
}

template <class T>
MixFileClass<T>::~MixFileClass() {
  this->Unlink();
}

template <class T>
bool MixFileClass<T>::Free(const std::string_view filename) {
  if (MixFileClass* ptr = Finder(filename)) {
    ptr->Free();
    return true;
  }
  return false;
}

template <class T>
void MixFileClass<T>::Free() {
  // Clear and force deallocation
  std::vector<std::byte>().swap(data_);
}

template <class T>
bool MixFileClass<T>::Cache() {
  if (!data_.empty()) {
    return true;
  }

  try {
    data_.resize(data_size_);
  } catch (const std::bad_alloc&) {
    return false;
  }

  T file(filename_.c_str());
  FileStraw file_straw(file);
  Straw* straw = &file_straw;

  SHAStraw sha;
  if (has_digest_) {
    sha.Get_From(file_straw);
    straw = &sha;
  }

  if (!file.Open(READ)) {
    data_.clear();
    return false;
  }

  // Bias alignment logic
  file.Bias(0);
  file.Bias(data_start_);

  // Read directly into the vector buffer
  long actual = straw->Get(data_.data(), data_size_);

  if (actual != data_size_) {
    data_.clear();
    file.Error(EIO);
    return false;
  }

  if (has_digest_) {
    constexpr int kShaSize = 20;
    char expected[kShaSize];
    char computed[kShaSize];

    sha.Result(computed);
    file_straw.Get(expected, sizeof(expected));

    if (std::memcmp(expected, computed, sizeof(expected)) != 0) {
      data_.clear();  // Corrupt data
      return false;
    }
  }

  return true;
}

template <class T>
bool MixFileClass<T>::Cache(const std::string_view filename) {
  if (auto* mixer = Finder(filename)) {
    return mixer->Cache();
  }
  return false;
}

template <class T>
std::optional<typename MixFileClass<T>::FileLocation> MixFileClass<T>::Offset(
    const std::string_view filename) {
  if (filename.empty()) {
    return std::nullopt;
  }

  // NOTE: CRC calculation depends on specific implementation.
  // Using upper case for case-insensitivity consistency.
  const std::int32_t crc = CrcEngine::Compute(absl::AsciiStrToUpper(filename));

  // Iterate through mixfiles (Most Recently Added / Tail priority is typical
  // for override mods)
  for (auto* mix = MixList.First(); mix->Is_Valid(); mix = mix->Next()) {
    // Use C++20/23 ranges::lower_bound with projection
    auto it =
        std::ranges::lower_bound(mix->file_index_, crc, {}, &FileEntry::crc);

    if (it != mix->file_index_.end() && it->crc == crc) {
      const bool cached = !mix->data_.empty();

      // Safe span construction
      std::span<const std::byte> view;
      if (cached) {
        // Ensure bounds safety
        if (it->offset + it->size <= mix->data_.size()) {
          view = {mix->data_.data() + it->offset,
                  static_cast<std::size_t>(it->size)};
        }
      }

      return FileLocation{
          .data = view,
          .mixfile = mix,
          // If cached, offset is relative to buffer. If not, absolute file
          // offset.
          .offset = cached ? it->offset : (it->offset + mix->data_start_),
          .size = it->size,
      };
    }
  }

  return std::nullopt;
}

template <class T>
std::span<const std::byte> MixFileClass<T>::RetrieveData(
    std::string_view filename) {
  auto loc = Offset(filename);
  return loc ? loc->data : std::span<const std::byte>{};
}

// Legacy API for backward compatibility with code expecting void*.
template <class T>
const void* MixFileClass<T>::Retrieve(std::string_view filename) {
  auto data = RetrieveData(filename);
  return data.empty() ? nullptr : data.data();
}

template <class T>
MixFileClass<T>* MixFileClass<T>::Finder(const std::string_view filename) {
  for (auto* ptr = MixList.First(); ptr->Is_Valid(); ptr = ptr->Next()) {
    // Compare basename only; paths may differ.
    auto basename = std::filesystem::path(ptr->filename_).filename().string();
    if (absl::EqualsIgnoreCase(basename, filename)) {
      return ptr;
    }
  }
  return nullptr;
}

// Explicit template instantiation
template class MixFileClass<CCFileClass>;
