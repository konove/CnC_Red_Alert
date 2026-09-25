// File: the RangeStream implementation.

#include "tech/range_stream.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <span>
#include <utility>

#include "base/seek_origin.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "tech/byte_stream.h"

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
