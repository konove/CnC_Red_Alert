// File: MixArchive implementation.
//
// Originally MIXFILE.CPP by Joe L. Bostic, August 8, 1994.

#include "tech/mix_archive.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "sdllib/file_access.h"
#include "tech/blwstraw.h"
#include "tech/crc.h"
#include "tech/file.h"
#include "tech/game_file.h"
#include "tech/listnode.h"
#include "tech/pk.h"
#include "tech/pkstraw.h"
#include "tech/rndstraw.h"
#include "tech/shastraw.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

bool MixArchive::Open(std::string_view filename, const PKey* key) {
  GameFile file(filename);
  filename_ = file.FileName();

  FileStraw file_straw(file);
  std::unique_ptr<BlowStraw> decrypt_straw;
  Straw* straw = &file_straw;

  if (!file.IsAvailable()) {
    return false;
  }

  FileHeader file_header{};
  struct MixMetadata {
    std::int16_t First;   // Zero indicates extended format.
    std::int16_t Second;  // Bit 0: has digest, Bit 1: encrypted.
  } alternate{};

  // Read initial metadata to determine format
  if (straw->Get(&alternate, sizeof(alternate)) != sizeof(alternate)) {
    return false;
  }

  if (alternate.First == 0) {
    // Extended Format
    has_digest_ = (alternate.Second & 0x01) != 0;
    is_encrypted_ = (alternate.Second & 0x02) != 0;

    if (is_encrypted_) {
      assert(key != nullptr);
      decrypt_straw = MakePKDecryptStraw(file_straw, *key);
      if (decrypt_straw == nullptr) {
        return false;  // Failed to read encrypted key header.
      }
      straw = decrypt_straw.get();
    }

    if (straw->Get(&file_header, sizeof(file_header)) != sizeof(file_header)) {
      return false;
    }
  } else {
    // Plain Format: The bytes read into 'alternate' are actually the start of
    // FileHeader. Reassemble via a byte buffer to avoid reinterpret_cast.
    char header_buf[sizeof(file_header)];
    std::memcpy(header_buf, &alternate, sizeof(alternate));
    const int rest = sizeof(file_header) - sizeof(alternate);
    if (straw->Get(header_buf + sizeof(alternate), rest) != rest) {
      return false;
    }
    std::memcpy(&file_header, header_buf, sizeof(file_header));
  }

  // A corrupt header would size the index from a negative count, which makes
  // resize() throw, or claim data the file does not hold.
  if (file_header.count < 0 || file_header.size < 0) {
    return false;
  }
  data_size_ = file_header.size;

  file_index_.resize(static_cast<std::size_t>(file_header.count));
  const int index_bytes = file_header.count * int{sizeof(FileEntry)};
  if (straw->Get(file_index_.data(), index_bytes) != index_bytes) {
    return false;
  }

  // Offset() hands out spans into the cached data, so every entry must lie
  // inside it.
  for (const FileEntry& entry : file_index_) {
    if (entry.offset < 0 || entry.size < 0 ||
        int64_t{entry.offset} + entry.size > data_size_) {
      return false;
    }
  }

  if (int64_t{file.Seek(0, SeekOrigin::kCurrent)} + data_size_ >
      int64_t{file.Size()}) {
    return false;
  }

  // Calculate start position.
  // Seek returns long, cast to int32_t to match class member (assuming < 2GB
  // files)
  data_start_ = static_cast<std::int32_t>(file.Seek(0, SeekOrigin::kCurrent));

  return true;
}

MixArchive::~MixArchive() { this->Unlink(); }

bool MixArchive::Free(const std::string_view filename) {
  if (MixArchive* ptr = Finder(filename)) {
    ptr->Free();
    return true;
  }
  return false;
}

void MixArchive::Free() {
  // Clear and force deallocation
  std::vector<std::byte>().swap(data_);
}

bool MixArchive::Cache() {
  if (!data_.empty()) {
    return true;
  }

  try {
    data_.resize(static_cast<std::size_t>(data_size_));
  } catch (const std::bad_alloc&) {
    return false;
  }

  GameFile file(filename_);
  FileStraw file_straw(file);
  Straw* straw = &file_straw;

  SHAStraw sha;
  if (has_digest_) {
    sha.SetSource(file_straw);
    straw = &sha;
  }

  if (!file.Open(FileAccess::kRead)) {
    data_.clear();
    return false;
  }

  file.Seek(data_start_, SeekOrigin::kBegin);

  // Read directly into the vector buffer
  if (const int actual = straw->Get(data_.data(), data_size_);
      actual != data_size_) {
    data_.clear();
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

bool MixArchive::Cache(const std::string_view filename) {
  if (auto* mixer = Finder(filename)) {
    return mixer->Cache();
  }
  return false;
}

std::optional<MixArchive::FileLocation> MixArchive::Offset(
    const std::string_view filename) {
  if (filename.empty()) {
    return std::nullopt;
  }

  // CRC calculation uses upper case for case-insensitivity consistency.
  // FileEntry keeps the CRC as the signed value the index is sorted by.
  const auto crc = std::bit_cast<std::int32_t>(
      CrcEngine::Compute(absl::AsciiStrToUpper(filename)));

  // Iterate through mixfiles (Most Recently Added / Tail priority is typical
  // for override mods)
  for (auto* mix = MixList.First(); mix->Is_Valid(); mix = mix->Next()) {
    // Use C++20/23 ranges::lower_bound with projection
    const auto it =
        std::ranges::lower_bound(mix->file_index_, crc, {}, &FileEntry::crc);

    if (it != mix->file_index_.end() && it->crc == crc) {
      const bool cached = !mix->data_.empty();

      // Safe span construction
      std::span<const std::byte> view;
      if (cached && it->offset + it->size <=
                        static_cast<std::int32_t>(mix->data_.size())) {
        view = {mix->data_.data() + it->offset,
                static_cast<std::size_t>(it->size)};
      }

      return FileLocation{
          .data = view,
          .mixfile = mix,
          .offset = cached ? it->offset : it->offset + mix->data_start_,
          .size = it->size,
      };
    }
  }

  return std::nullopt;
}

std::span<const std::byte> MixArchive::RetrieveData(
    const std::string_view filename) {
  auto loc = Offset(filename);
  return loc ? loc->data : std::span<const std::byte>{};
}

// Legacy API for backward compatibility with code expecting void*.
const void* MixArchive::Retrieve(std::string_view filename) {
  const auto data = RetrieveData(filename);
  return data.empty() ? nullptr : data.data();
}

MixArchive* MixArchive::Register(std::string_view filename, const PKey* key,
                                 RandomStraw* /*unused*/) {
  if (auto* existing = Finder(filename)) {
    return existing;
  }
  auto* mix = new MixArchive();
  if (!mix->Open(filename, key)) {
    delete mix;
    return nullptr;
  }
  MixList.Add_Tail(mix);
  return mix;
}

bool MixArchive::Unregister(std::string_view filename) {
  if (const auto* mix = Finder(filename)) {
    delete mix;
    return true;
  }
  return false;
}

void MixArchive::Free_All() {
  for (const auto* node = MixList.First(); node->Is_Valid();) {
    auto* next = node->Next();
    delete node;
    node = next;
  }
}

MixArchive* MixArchive::Finder(const std::string_view filename) {
  for (auto* ptr = MixList.First(); ptr->Is_Valid(); ptr = ptr->Next()) {
    // Compare basename only; paths may differ.
    const auto basename =
        std::filesystem::path(ptr->filename_).filename().string();
    if (absl::EqualsIgnoreCase(basename, filename)) {
      return ptr;
    }
  }
  return nullptr;
}
