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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "ra/ccfile.h"
#include "ra/compat.h"
#include "ra/conquer.h"
#include "ra/externs.h"
#include "ra/startup.h"
#include "sdllib/include/misc.h"
#include "tech/pkstraw.h"
#include "tech/shastraw.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

// Opens a mixfile and reads its index. The file data is not loaded until
// Cache() is called. Supports both plain and extended (encrypted/signed)
// mixfile formats.
template <class T>
MixFileClass<T>::MixFileClass(const std::string_view filename,
                              const PKey *key) {
  if (!Force_CD_Available(RequiredCD)) {
    Emergency_Exit(EXIT_FAILURE);
  }

  T file(std::string(filename).c_str());
  filename_ = file.File_Name();

  FileStraw file_straw(file);
  PKStraw pstraw(PKStraw::DECRYPT, CryptRandom);
  Straw *straw = &file_straw;

  if (!file.Is_Available()) {
    return;
  }

  FileHeader file_header;
  struct {
    short First;   // Zero indicates extended format.
    short Second;  // Bit 0: has digest, Bit 1: encrypted.
  } alternate;

  straw->Get(&alternate, sizeof(alternate));

  // Extended format: First field is zero, Second contains feature flags.
  // Plain format: First two bytes are already part of the file header.
  if (alternate.First == 0) {
    has_digest_ = (alternate.Second & 0x01) != 0;
    is_encrypted_ = (alternate.Second & 0x02) != 0;

    if (is_encrypted_) {
      pstraw.Key(key);
      pstraw.Get_From(&file_straw);
      straw = &pstraw;
    }

    straw->Get(&file_header, sizeof(file_header));
  } else {
    // Plain format: reinterpret bytes already read as start of header.
    memmove(&file_header, &alternate, sizeof(alternate));
    straw->Get(reinterpret_cast<char *>(&file_header) + sizeof(alternate),
               sizeof(file_header) - sizeof(alternate));
  }

  data_size_ = file_header.size;

  file_index_.resize(file_header.count);
  straw->Get(file_index_.data(), file_index_.size() * sizeof(FileEntry));

  // Encrypted headers are padded so data_start_ aligns to current position.
  data_start_ = file.Seek(0, SEEK_CUR) + file.BiasStart;

  MixList.Add_Tail(this);
}

template <class T>
MixFileClass<T>::~MixFileClass() {
  this->Unlink();  // Remove from global registry.
}

template <class T>
bool MixFileClass<T>::Free(const std::string_view filename) {
  if (MixFileClass *ptr = Finder(filename)) {
    ptr->Free();
    return true;
  }
  return false;
}

// Releases cached data while keeping the index. Allows re-caching later.
template <class T>
void MixFileClass<T>::Free() {
  data_.clear();
  data_.shrink_to_fit();
}

// Loads all file data into memory for fast access. Verifies SHA-1 digest
// if present to detect corruption. Returns false on I/O error or bad digest.
template <class T>
bool MixFileClass<T>::Cache() {
  if (!data_.empty()) {
    return true;  // Already cached.
  }

  data_.resize(data_size_);

  T file(filename_.c_str());
  FileStraw file_straw(file);
  Straw *straw = &file_straw;

  // Chain SHA computation into the read stream to verify integrity.
  SHAStraw sha;
  if (has_digest_) {
    sha.Get_From(file_straw);
    straw = &sha;
  }

  file.Open(READ);
  file.Bias(0);
  file.Bias(data_start_);

  long actual = straw->Get(data_.data(), data_size_);
  if (actual != data_size_) {
    data_.clear();
    file.Error(EIO);
    return false;
  }

  // Verify digest matches if present.
  if (has_digest_) {
    char expected[20];
    char computed[20];
    sha.Result(computed);
    file_straw.Get(expected, sizeof(expected));
    if (memcmp(expected, computed, sizeof(expected)) != 0) {
      data_.clear();
      return false;
    }
  }

  return true;
}

template <class T>
bool MixFileClass<T>::Cache(const std::string_view filename) {
  if (auto *mixer = Finder(filename)) {
    return mixer->Cache();
  }
  return false;
}

// Locates a file across all registered mixfiles by CRC lookup.
template <class T>
std::optional<typename MixFileClass<T>::FileLocation> MixFileClass<T>::Offset(
    const std::string_view filename) {
  if (filename.empty()) {
    return std::nullopt;
  }

  // Filenames are stored as CRCs for compact indexing and fast lookup.
  int32_t crc = Calculate_CRC(absl::AsciiStrToUpper(filename));

  for (auto *mix = MixList.First(); mix->Is_Valid(); mix = mix->Next()) {
    auto it =
        std::ranges::lower_bound(mix->file_index_, crc, {}, &FileEntry::crc);

    if (it != mix->file_index_.end() && it->crc == crc) {
      return FileLocation{
          .data = mix->data_.empty()
                      ? std::span<std::byte>{}
                      : std::span<std::byte>(mix->data_.data() + it->offset,
                                             it->size),
          .mixfile = mix,
          // Return absolute file offset if not cached, relative if cached.
          .offset =
              mix->data_.empty() ? it->offset + mix->data_start_ : it->offset,
          .size = it->size,
      };
    }
  }

  return std::nullopt;
}

// Returns cached file data, or nullptr if not cached or not found.
template <class T>
const void *MixFileClass<T>::Retrieve(const std::string_view filename) {
  auto loc = Offset(filename);
  return loc ? loc->data.data() : nullptr;
}

template <class T>
MixFileClass<T> *MixFileClass<T>::Finder(const std::string_view filename) {
  for (auto *ptr = MixList.First(); ptr->Is_Valid(); ptr = ptr->Next()) {
    // Compare basename only; paths may differ.
    auto basename = std::filesystem::path(ptr->filename_).filename().string();
    if (absl::EqualsIgnoreCase(basename, filename)) {
      return ptr;
    }
  }
  return nullptr;
}

template class MixFileClass<CCFileClass>;
