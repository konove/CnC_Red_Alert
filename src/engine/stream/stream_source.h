#ifndef CNC_RED_ALERT_ENGINE_STREAM_STREAM_SOURCE_H_
#define CNC_RED_ALERT_ENGINE_STREAM_STREAM_SOURCE_H_

// File: StreamSource, a ByteSource over an already-open ByteStream.

#include <cstddef>
#include <span>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/stream/byte_source.h"
#include "engine/stream/byte_stream.h"

// A source that reads an open stream from its current position. It neither
// opens nor closes the stream; whoever owns the stream does.
//
// Example:
//   const std::unique_ptr<ByteStream> file = OpenGameFile("RULES.INI");
//   StreamSource source(*file);
class StreamSource : public ByteSource {
 public:
  // stream must outlive this source.
  explicit StreamSource(ByteStream& stream ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : stream_(stream) {}

  base::ssize Read(std::span<std::byte> buffer) override {
    if (!ok()) {
      return 0;
    }
    const base::ssize count = stream_.Read(buffer);
    if (!stream_.ok()) {
      Fail();
    }
    return count;
  }

 private:
  ByteStream& stream_;
};

#endif  // CNC_RED_ALERT_ENGINE_STREAM_STREAM_SOURCE_H_
