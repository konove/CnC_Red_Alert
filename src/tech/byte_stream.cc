// File: the DiskStream, MemoryStream and RangeStream implementations.

#include "tech/byte_stream.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "port/bytes_of.h"
#include "sdllib/file_access.h"

// Moves position by offset from origin within [0, size] and returns it.
namespace {
base::ssize ClampedSeek(const base::ssize position, const base::ssize size,
                        const base::ssize offset, const SeekOrigin origin) {
  base::ssize base = position;
  switch (origin) {
    case SeekOrigin::kBegin:
      base = 0;
      break;
    case SeekOrigin::kEnd:
      base = size;
      break;
    case SeekOrigin::kCurrent:
    default:
      break;
  }
  return std::clamp<base::ssize>(base + offset, 0, size);
}

std::ios_base::seekdir SeekDir(const SeekOrigin origin) {
  switch (origin) {
    case SeekOrigin::kBegin:
      return std::ios_base::beg;
    case SeekOrigin::kEnd:
      return std::ios_base::end;
    case SeekOrigin::kCurrent:
    default:
      return std::ios_base::cur;
  }
}

base::ssize ToPosition(const std::streampos position) {
  return static_cast<base::ssize>(static_cast<std::streamoff>(position));
}
}  // namespace

std::vector<std::byte> ByteStream::ReadBytes(const base::ssize count) {
  std::vector<std::byte> bytes(base::ToSize(count));
  bytes.resize(base::ToSize(Read(bytes)));
  return bytes;
}

std::string ByteStream::ReadString(const base::ssize count) {
  std::string text(base::ToSize(count), '\0');
  text.resize(base::ToSize(Read(std::span(text))));
  return text;
}

std::unique_ptr<DiskStream> DiskStream::Open(const std::string_view path,
                                             const FileAccess access) {
  const std::filesystem::path file_path(path);
  // A directory opens as a file on POSIX; reading it then throws instead of
  // returning an error code. Refusing it here, rather than letting the first
  // read throw, is also what keeps IsAvailable()/FindExistingFile() from
  // treating a directory as though it were a file that merely can't be read.
  std::error_code error;
  if (std::filesystem::is_directory(file_path, error)) {
    return nullptr;
  }
  auto stream = std::unique_ptr<DiskStream>(new DiskStream);
  std::filebuf& file = stream->file_;
  constexpr std::ios_base::openmode kBinary = std::ios_base::binary;
  bool opened = false;
  switch (access) {
    case FileAccess::kRead:
      opened = file.open(file_path, std::ios_base::in | kBinary) != nullptr;
      break;
    case FileAccess::kWrite:
      opened = file.open(file_path, std::ios_base::out | std::ios_base::trunc |
                                        kBinary) != nullptr;
      break;
    case FileAccess::kReadWrite:
      // in|out keeps the contents (the record file appends to itself) but
      // needs the file to exist; create it only when there is nothing to keep.
      opened =
          file.open(file_path, std::ios_base::in | std::ios_base::out |
                                   kBinary) != nullptr ||
          file.open(file_path, std::ios_base::in | std::ios_base::out |
                                   std::ios_base::trunc | kBinary) != nullptr;
      break;
    default:
      break;
  }
  return opened ? std::move(stream) : nullptr;
}

base::ssize DiskStream::Read(const std::span<std::byte> buffer) {
  try {
    return file_.sgetn(port::CharBytes(buffer).data(), std::ssize(buffer));
  } catch (const std::ios_base::failure&) {
    // A real I/O error (EIO from a failing disk, a dropped network share)
    // throws instead of returning a short count; report it through ok(), as
    // the stdio-backed implementation did by checking ferror().
    failed_ = true;
    return 0;
  }
}

base::ssize DiskStream::Write(const std::span<const std::byte> buffer) {
  const base::ssize written =
      file_.sputn(port::CharBytes(buffer).data(), std::ssize(buffer));
  if (written != std::ssize(buffer)) {
    failed_ = true;
  }
  return written;
}

base::ssize DiskStream::Seek(const base::ssize offset,
                             const SeekOrigin origin) {
  const std::streampos moved = file_.pubseekoff(offset, SeekDir(origin));
  if (static_cast<std::streamoff>(moved) == -1) {
    // A seek to before the start fails and leaves the position alone, as
    // fseek did; report where the stream still is.
    return ToPosition(file_.pubseekoff(0, std::ios_base::cur));
  }
  return ToPosition(moved);
}

base::ssize DiskStream::Size() {
  const std::streampos here = file_.pubseekoff(0, std::ios_base::cur);
  const std::streampos end = file_.pubseekoff(0, std::ios_base::end);
  file_.pubseekpos(here);
  return ToPosition(end);
}

bool DiskStream::Flush() {
  try {
    if (file_.pubsync() != 0) {
      failed_ = true;
    }
  } catch (const std::ios_base::failure&) {
    // Same real I/O error case Read() catches, on the write-back path.
    failed_ = true;
  }
  return ok();
}

base::ssize MemoryStream::Read(const std::span<std::byte> buffer) {
  const base::ssize count =
      std::min(std::ssize(buffer), std::ssize(bytes_) - position_);
  if (count > 0) {
    base::CopyBytes(buffer, bytes_.subspan(base::ToSize(position_)), count);
    position_ += count;
  }
  return count;
}

base::ssize MemoryStream::Seek(const base::ssize offset,
                               const SeekOrigin origin) {
  position_ = ClampedSeek(position_, std::ssize(bytes_), offset, origin);
  return position_;
}

RangeStream::RangeStream(std::unique_ptr<ByteStream> inner,
                         const base::ssize offset, const base::ssize size)
    : inner_(std::move(inner)),
      offset_(std::clamp<base::ssize>(offset, 0, inner_->Size())),
      size_(std::clamp<base::ssize>(size, 0, inner_->Size() - offset_)) {}

base::ssize RangeStream::Read(const std::span<std::byte> buffer) {
  const base::ssize count = std::min(std::ssize(buffer), size_ - position_);
  if (count <= 0) {
    return 0;
  }
  // The inner stream may be positioned anywhere: a window over an archive is
  // opened once and read from many times, and a seek on this window moves only
  // position_. Reposition it when it is not already where this read starts --
  // which, for the sequential reads that decoding a packed file is made of, is
  // almost never, and each skipped seek is a filebuf pubseekoff (an lseek that
  // discards the buffer).
  const base::ssize start = offset_ + position_;
  if (inner_position_ != start) {
    if (inner_->Seek(start, SeekOrigin::kBegin) != start) {
      inner_position_ = -1;
      failed_ = true;
      return 0;
    }
    inner_position_ = start;
  }
  const base::ssize bytes_read =
      inner_->Read(buffer.first(base::ToSize(count)));
  inner_position_ += bytes_read;
  position_ += bytes_read;
  return bytes_read;
}

base::ssize RangeStream::Seek(const base::ssize offset,
                              const SeekOrigin origin) {
  position_ = ClampedSeek(position_, size_, offset, origin);
  return position_;
}
