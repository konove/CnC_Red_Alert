// File: GameFile implementation.
//
// Originally CCFILE.CPP by Joe L. Bostic, started August 8, 1994.

#include "tech/game_file.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"
#include "tech/mix_archive.h"
#include "tech/search_paths.h"

std::unique_ptr<ByteStream> GameFile::OpenStream(const std::string_view name,
                                                 const FileAccess rights) {
  if (name.empty()) {
    return nullptr;
  }

  // Writes never search: they target the name as given, so a loose file is
  // created next to the executable rather than on the CD.
  if (HasAccess(rights, FileAccess::kWrite)) {
    return DiskStream::Open(name, rights);
  }

  // A loose file on disk wins over the packed copy, so patches work.
  if (const std::optional<std::string> path = SearchPaths::Resolve(name)) {
    return DiskStream::Open(*path, rights);
  }

  const std::optional<MixArchive::FileLocation> location =
      MixArchive::Offset(name);
  if (!location) {
    return nullptr;
  }

  // A cached archive holds the bytes in memory; the file is a view of them.
  if (!location->data.empty()) {
    return std::make_unique<MemoryStream>(location->data);
  }

  // An archive still on disk is opened by its own name, which resolves the
  // same way, so an archive packed inside another one becomes a window of a
  // window. The offset is relative to the archive's own start.
  std::unique_ptr<ByteStream> archive =
      OpenStream(location->mixfile->Filename(), FileAccess::kRead);
  if (archive == nullptr) {
    return nullptr;
  }
  return std::make_unique<RangeStream>(std::move(archive), location->offset,
                                       location->size);
}

void GameFile::SetName(const std::string_view name) {
  Close();
  name_ = name;
}

bool GameFile::Create() {
  Close();
  return DiskStream::Open(name_, FileAccess::kWrite) != nullptr;
}

bool GameFile::Delete() {
  Close();
  const std::optional<std::string> path = SearchPaths::Resolve(name_);
  return path.has_value() && IO_Delete_File(path->c_str());
}

bool GameFile::IsAvailable() {
  if (IsOpen()) {
    return true;
  }
  // The archive index is in memory, so it is checked before the disk.
  return MixArchive::Offset(name_).has_value() ||
         SearchPaths::Resolve(name_).has_value();
}

bool GameFile::Open(const std::string_view name, const FileAccess rights) {
  SetName(name);
  return Open(rights);
}

bool GameFile::Open(const FileAccess rights) {
  Close();
  failed_ = false;
  stream_ = OpenStream(name_, rights);
  return IsOpen();
}

base::ssize GameFile::Read(const std::span<std::byte> buffer) {
  const bool opened_for_this_read = !IsOpen() && Open(FileAccess::kRead);
  if (!IsOpen()) {
    return 0;
  }
  const base::ssize bytes_read = stream_->Read(buffer);
  if (!stream_->ok()) {
    failed_ = true;
  }
  if (opened_for_this_read) {
    Close();
  }
  return bytes_read;
}

base::ssize GameFile::Write(const std::span<const std::byte> buffer) {
  const bool opened_for_this_write = !IsOpen() && Open(FileAccess::kWrite);
  if (!IsOpen()) {
    return 0;
  }
  const base::ssize bytes_written = stream_->Write(buffer);
  if (!stream_->ok()) {
    failed_ = true;
  }
  if (opened_for_this_write) {
    Close();
  }
  return bytes_written;
}

base::ssize GameFile::Seek(const base::ssize offset, const SeekOrigin origin) {
  return IsOpen() ? stream_->Seek(offset, origin) : 0;
}

base::ssize GameFile::Size() {
  if (IsOpen()) {
    return stream_->Size();
  }
  // A packed file's size is in the archive index, so no open is needed for
  // it; a loose file has to be opened to be measured.
  if (SearchPaths::Resolve(name_).has_value()) {
    const std::unique_ptr<ByteStream> stream =
        OpenStream(name_, FileAccess::kRead);
    return stream != nullptr ? stream->Size() : 0;
  }
  const std::optional<MixArchive::FileLocation> location =
      MixArchive::Offset(name_);
  return location ? location->size : 0;
}
