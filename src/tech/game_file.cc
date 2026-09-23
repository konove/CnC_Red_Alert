// File: OpenGameFile, GameFileExists, GameFileSize and DeleteGameFile,
// the free functions that resolve and open the game's data by name.
//
// Originally CCFILE.CPP by Joe L. Bostic, started August 8, 1994.

#include "tech/game_file.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "base/types.h"
#include "tech/byte_stream.h"
#include "tech/disk_file.h"
#include "tech/disk_stream.h"
#include "tech/file_access.h"
#include "tech/memory_stream.h"
#include "tech/mix_archive.h"
#include "tech/range_stream.h"
#include "tech/search_paths.h"

std::unique_ptr<ByteStream> OpenGameFile(const std::string_view name,
                                         const FileAccess access) {
  if (name.empty()) {
    return nullptr;
  }

  // Writes never search the search paths or the archives: they target the
  // name as given, so a loose file is created next to the executable rather
  // than on the CD. OpenDiskFile still prefers an existing lowercase twin of
  // that name (e.g. conquer.ini) over creating a new upper-case file, the
  // same as every other write site.
  if (HasAccess(access, FileAccess::kWrite)) {
    return OpenDiskFile(name, access);
  }

  // A loose file on disk wins over the packed copy, so patches work.
  if (const std::optional<std::string> path = SearchPaths::Resolve(name)) {
    return DiskStream::Open(*path, access);
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
      OpenGameFile(location->mixfile->Filename(), FileAccess::kRead);
  if (archive == nullptr) {
    return nullptr;
  }
  return std::make_unique<RangeStream>(std::move(archive), location->offset,
                                       location->size);
}

bool GameFileExists(const std::string_view name) {
  // The archive index is in memory, so it is checked before the disk.
  return MixArchive::Offset(name).has_value() ||
         SearchPaths::Resolve(name).has_value();
}

base::ssize GameFileSize(const std::string_view name) {
  // A packed file's size is in the archive index, so no open is needed for
  // it; a loose file has to be opened to be measured.
  if (SearchPaths::Resolve(name).has_value()) {
    const std::unique_ptr<ByteStream> stream =
        OpenGameFile(name, FileAccess::kRead);
    return stream != nullptr ? stream->Size() : 0;
  }
  const std::optional<MixArchive::FileLocation> location =
      MixArchive::Offset(name);
  return location ? location->size : 0;
}

bool DeleteGameFile(const std::string_view name) {
  const std::optional<std::string> path = SearchPaths::Resolve(name);
  if (!path.has_value()) {
    return false;
  }
  std::error_code error;
  return std::filesystem::remove(*path, error);
}
