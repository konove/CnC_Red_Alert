// File: the ByteStream helpers and ClampedSeek.

#include "engine/stream/byte_stream.h"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/seek_origin.h"

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
