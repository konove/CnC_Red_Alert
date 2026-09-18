// File: the DiskStream, MemoryStream and RangeStream implementations.

#include "tech/byte_stream.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "sdllib/file.h"
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
}  // namespace

std::unique_ptr<DiskStream> DiskStream::Open(const std::string_view path,
                                             const FileAccess access) {
  // IO_Open_File wants a terminated string.
  void* const handle = IO_Open_File(std::string(path).c_str(), access);
  if (handle == nullptr) {
    return nullptr;
  }
  return std::unique_ptr<DiskStream>(new DiskStream(handle));
}

DiskStream::~DiskStream() { IO_Close_File(handle_); }

base::ssize DiskStream::Read(const std::span<std::byte> buffer) {
  size_t bytes_read = 0;
  if (!IO_Read_File(handle_, buffer, bytes_read)) {
    failed_ = true;
  }
  return base::ToSigned(bytes_read);
}

base::ssize DiskStream::Write(const std::span<const std::byte> buffer) {
  size_t bytes_written = 0;
  if (!IO_Write_File(handle_, buffer, bytes_written)) {
    failed_ = true;
  }
  return base::ToSigned(bytes_written);
}

base::ssize DiskStream::Seek(const base::ssize offset,
                             const SeekOrigin origin) {
  return static_cast<base::ssize>(
      IO_Seek_File(handle_, offset, StdioOrigin(origin)));
}

base::ssize DiskStream::Size() {
  return static_cast<base::ssize>(IO_Get_File_Size(handle_));
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
  // almost never, and each skipped seek is an fseek and an ftell.
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
