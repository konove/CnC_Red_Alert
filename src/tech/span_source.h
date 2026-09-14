// File: SpanSource, a byte source that reads memory the caller owns.

#ifndef CNC_RED_ALERT_TECH_SPAN_SOURCE_H_
#define CNC_RED_ALERT_TECH_SPAN_SOURCE_H_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>

#include "absl/base/attributes.h"
#include "base/numeric.h"
#include "base/types.h"
#include "tech/byte_source.h"

// A chain source that hands out a caller-owned buffer until it runs out.
//
// Example:
//   SpanSource source(std::as_bytes(std::span(packed)));
//   LcwSource decompressor(CodecMode::kDecompress, source);
class SpanSource : public ByteSource {
 public:
  // Reads from buffer, which must outlive the source.
  explicit SpanSource(
      std::span<const std::byte> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : buffer_(buffer) {}

  base::ssize Read(std::span<std::byte> buffer) override {
    const base::ssize count =
        std::min(std::ssize(buffer), std::ssize(buffer_) - index_);
    if (count > 0) {
      std::memmove(buffer.data(), buffer_.data() + index_, base::ToSize(count));
      index_ += count;
    }
    return count;
  }

  // Returns the number of bytes not yet handed out.
  [[nodiscard]] base::ssize bytes_remaining() const {
    return std::ssize(buffer_) - index_;
  }

 private:
  std::span<const std::byte> buffer_;
  base::ssize index_ = 0;  // Bytes handed out so far.
};

#endif  // CNC_RED_ALERT_TECH_SPAN_SOURCE_H_
