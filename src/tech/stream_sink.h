#ifndef CNC_RED_ALERT_TECH_STREAM_SINK_H_
#define CNC_RED_ALERT_TECH_STREAM_SINK_H_

// File: StreamSink, a ByteSink over an already-open ByteStream.

#include <cstddef>
#include <iterator>
#include <span>

#include "absl/base/attributes.h"
#include "tech/byte_sink.h"
#include "tech/byte_stream.h"

// A sink that writes an open stream at its current position. It neither
// opens nor closes the stream; whoever owns the stream does.
//
// Example:
//   const std::unique_ptr<ByteStream> file =
//       OpenGameFile("SAVEGAME.001", FileAccess::kWrite);
//   StreamSink sink(*file);
class StreamSink : public ByteSink {
 public:
  // stream must outlive this sink.
  explicit StreamSink(ByteStream& stream ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : stream_(stream) {}

  bool Write(std::span<const std::byte> bytes) override {
    if (!ok()) {
      return false;
    }
    if (stream_.Write(bytes) != std::ssize(bytes)) {
      Fail();
    }
    return ok();
  }

  bool Flush() override {
    if (!ok()) {
      return false;
    }
    if (!stream_.Flush()) {
      Fail();
    }
    return ok();
  }

 private:
  ByteStream& stream_;
};

#endif  // CNC_RED_ALERT_TECH_STREAM_SINK_H_
