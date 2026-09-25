#ifndef CNC_RED_ALERT_ENGINE_STREAM_MEMORY_STREAM_H_
#define CNC_RED_ALERT_ENGINE_STREAM_MEMORY_STREAM_H_

// File: MemoryStream, a read-only ByteStream over bytes someone else owns.

#include <cstddef>
#include <iterator>
#include <span>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"

// A read-only view of bytes that someone else owns and keeps alive for as
// long as the stream is used, such as a file inside a cached mixfile.
class MemoryStream final : public ByteStream {
 public:
  explicit MemoryStream(
      std::span<const std::byte> bytes ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : bytes_(bytes) {}

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }

  // The position is clamped to [0, Size()].
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override { return std::ssize(bytes_); }

 private:
  std::span<const std::byte> bytes_;
  base::ssize position_ = 0;
};

#endif  // CNC_RED_ALERT_ENGINE_STREAM_MEMORY_STREAM_H_
