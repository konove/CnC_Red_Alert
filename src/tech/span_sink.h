// File: SpanSink, a byte sink that stores into memory the caller owns.

#ifndef CNC_RED_ALERT_TECH_SPAN_SINK_H_
#define CNC_RED_ALERT_TECH_SPAN_SINK_H_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>

#include "base/numeric.h"
#include "base/types.h"
#include "tech/byte_sink.h"

// A chain terminator that stores into a caller-owned buffer.
//
// Example:
//   std::array<char, 64> storage{};
//   SpanSink sink(std::as_writable_bytes(std::span(storage)));
//   LcwSink compressor(CodecMode::kCompress, sink);
class SpanSink : public ByteSink {
 public:
  // Stores into buffer, which must outlive the sink. clang suggests
  // lifetimebound here, but its lifetimebound-violation check cannot verify
  // it.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
  explicit SpanSink(std::span<std::byte> buffer) : buffer_(buffer) {}

  // Stores what fits, failing if that is not every byte.
  bool Write(std::span<const std::byte> bytes) override {
    if (!ok()) {
      return false;
    }
    const base::ssize count =
        std::min(std::ssize(bytes), std::ssize(buffer_) - index_);
    if (count > 0) {
      std::memmove(buffer_.data() + index_, bytes.data(), base::ToSize(count));
      index_ += count;
    }
    if (count < std::ssize(bytes)) {
      Fail();
    }
    return ok();
  }

  // Returns the number of bytes stored so far.
  [[nodiscard]] base::ssize bytes_written() const { return index_; }

 private:
  std::span<std::byte> buffer_;
  base::ssize index_ = 0;  // Bytes stored so far.
};

#endif  // CNC_RED_ALERT_TECH_SPAN_SINK_H_
