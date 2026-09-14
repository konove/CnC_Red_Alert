// File: GameFileVqaIo implementation.

#include "tech/game_file_vqa_io.h"

#include <cstddef>
#include <iterator>
#include <span>
#include <string_view>

#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file_access.h"
#include "tech/game_file.h"

bool GameFileVqaIo::Open(const std::string_view name) {
  stream_ = GameFile::OpenStream(name, FileAccess::kRead);
  return stream_ != nullptr;
}

bool GameFileVqaIo::Read(const std::span<std::byte> buffer) {
  return stream_ != nullptr && stream_->Read(buffer) == std::ssize(buffer);
}

bool GameFileVqaIo::Seek(const base::ssize offset, const SeekOrigin origin) {
  if (stream_ == nullptr) {
    return false;
  }
  // The streams clamp instead of failing, so the request is checked against
  // where the position actually landed.
  base::ssize target = offset;
  switch (origin) {
    case SeekOrigin::kCurrent:
      target += stream_->Tell();
      break;
    case SeekOrigin::kEnd:
      target += stream_->Size();
      break;
    case SeekOrigin::kBegin:
    default:
      break;
  }
  return stream_->Seek(target, SeekOrigin::kBegin) == target;
}
