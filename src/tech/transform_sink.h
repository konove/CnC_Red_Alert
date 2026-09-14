// File: TransformSink, the push adapter that runs a ByteCodec over the bytes
// written to it.

#ifndef CNC_RED_ALERT_TECH_TRANSFORM_SINK_H_
#define CNC_RED_ALERT_TECH_TRANSFORM_SINK_H_

#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "tech/byte_codec.h"
#include "tech/byte_sink.h"

// A chain link that transforms the bytes written to it with codec C and
// writes the result to the next sink. Flush emits what the codec buffers.
//
// Example:
//   using LzoSink = TransformSink<LzoCodec>;
//   LzoSink compressor(CodecMode::kCompress, file_sink, 4096);
template <ByteCodec C>
class TransformSink : public ChainedSink {
 public:
  // Constructs the codec from mode and codec_args. next must outlive this
  // sink.
  template <class Mode, class... CodecArgs>
    requires std::is_enum_v<Mode> &&
                 std::constructible_from<C, Mode, CodecArgs...>
  TransformSink(Mode mode, ByteSink& next, CodecArgs&&... codec_args)
      : ChainedSink(next),
        codec_(mode, std::forward<CodecArgs>(codec_args)...) {}

  // For a codec without a mode. next must outlive this sink.
  explicit TransformSink(ByteSink& next)
    requires std::default_initializable<C>
      : ChainedSink(next) {}

  bool Write(std::span<const std::byte> bytes) override {
    if (!ok()) {
      return false;
    }
    if (!codec_.Process(bytes, next())) {
      Fail();
    }
    return ok();
  }

  bool Flush() override {
    if (!codec_.Flush(next())) {
      Fail();
    }
    return ChainedSink::Flush();
  }

  [[nodiscard]] bool ok() const override {
    return codec_.ok() && ChainedSink::ok();
  }

  [[nodiscard]] C& codec() ABSL_ATTRIBUTE_LIFETIME_BOUND { return codec_; }
  [[nodiscard]] const C& codec() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return codec_;
  }

 private:
  C codec_;
};

#endif  // CNC_RED_ALERT_TECH_TRANSFORM_SINK_H_
