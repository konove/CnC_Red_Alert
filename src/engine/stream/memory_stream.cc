// File: the MemoryStream implementation.

#include "engine/stream/memory_stream.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

#include "engine/base/buffer.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"

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
