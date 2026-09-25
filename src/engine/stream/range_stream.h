#ifndef CNC_RED_ALERT_ENGINE_STREAM_RANGE_STREAM_H_
#define CNC_RED_ALERT_ENGINE_STREAM_RANGE_STREAM_H_

// File: RangeStream, a read-only ByteStream over a window of another stream.

#include <cstddef>
#include <memory>
#include <span>

#include "base/types.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"

// A read-only window of size bytes starting offset bytes into another
// stream, which it owns. Reads never leave the window, and positions are
// relative to its start, so the window behaves as a whole stream of its own.
class RangeStream final : public ByteStream {
 public:
  // The window is clipped to what inner actually holds.
  RangeStream(std::unique_ptr<ByteStream> inner, base::ssize offset,
              base::ssize size);

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override;
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }

  // The position is clamped to [0, Size()].
  base::ssize Seek(base::ssize offset,
                   SeekOrigin origin = SeekOrigin::kCurrent) override;
  base::ssize Size() override { return size_; }
  [[nodiscard]] bool ok() const override { return !failed_ && inner_->ok(); }

 private:
  std::unique_ptr<ByteStream> inner_;

  // Set when inner_ could not be positioned for a read.
  bool failed_ = false;

  // Where the window starts in inner_.
  base::ssize offset_;

  // Length of the window.
  base::ssize size_;

  // Current position within the window.
  base::ssize position_ = 0;

  // Where inner_ was left by our last read, or -1 when that is unknown --
  // before the first read, and after any read that did not land where it was
  // asked to. A read only repositions inner_ when it disagrees with this.
  base::ssize inner_position_ = -1;
};

#endif  // CNC_RED_ALERT_ENGINE_STREAM_RANGE_STREAM_H_
