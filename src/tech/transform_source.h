// File: TransformSource, the pull adapter that runs a ByteCodec over the
// bytes read from its source.

#ifndef CNC_RED_ALERT_TECH_TRANSFORM_SOURCE_H_
#define CNC_RED_ALERT_TECH_TRANSFORM_SOURCE_H_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "tech/byte_codec.h"
#include "tech/byte_source.h"
#include "tech/vector_sink.h"

// A chain link that reads from its source, transforms the bytes with codec C
// and hands out the result. When the source runs out, the codec is flushed
// once, so a partial final block still comes out (or fails the source).
//
// Example:
//   using LzoSource = TransformSource<LzoCodec>;
//   LzoSource decompressor(CodecMode::kDecompress, file_source, 4096);
template <ByteCodec C>
class TransformSource : public ChainedSource {
 public:
  // Constructs the codec from mode and codec_args. source must outlive this
  // source.
  template <class Mode, class... CodecArgs>
    requires std::is_enum_v<Mode> &&
                 std::constructible_from<C, Mode, CodecArgs...>
  TransformSource(Mode mode, ByteSource& source, CodecArgs&&... codec_args)
      : ChainedSource(source),
        codec_(mode, std::forward<CodecArgs>(codec_args)...) {}

  // For a codec without a mode. source must outlive this source.
  explicit TransformSource(ByteSource& source)
    requires std::default_initializable<C>
      : ChainedSource(source) {}

  // Output left over from the last transform is handed out first. Only then
  // is the source read again, and only for as many bytes as the codec wants.
  base::ssize Read(std::span<std::byte> buffer) override {
    base::ssize total = 0;
    while (total < std::ssize(buffer)) {
      if (cursor_ < std::ssize(pending_)) {
        const base::ssize count = std::min(std::ssize(buffer) - total,
                                           std::ssize(pending_) - cursor_);
        base::CopyBytes(buffer.subspan(base::ToSize(total)),
                        std::span(pending_).subspan(base::ToSize(cursor_)),
                        count);
        cursor_ += count;
        total += count;
        continue;
      }
      if (exhausted_ || !ok()) {
        break;
      }
      pending_.clear();
      cursor_ = 0;
      const base::ssize wanted = codec_.BytesWanted(std::ssize(buffer) - total);
      input_.resize(base::ToSize(wanted));
      const base::ssize got = ChainedSource::Read(input_);
      VectorSink output(pending_);
      if (got > 0 &&
          !codec_.Process(std::span(input_).first(base::ToSize(got)), output)) {
        Fail();
      }
      if (got < wanted) {
        if (!codec_.Flush(output)) {
          Fail();
        }
        exhausted_ = true;
      }
    }
    return total;
  }

  [[nodiscard]] bool ok() const override {
    return codec_.ok() && ChainedSource::ok();
  }

  [[nodiscard]] C& codec() ABSL_ATTRIBUTE_LIFETIME_BOUND { return codec_; }
  [[nodiscard]] const C& codec() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return codec_;
  }

 private:
  C codec_;
  std::vector<std::byte> pending_;  // Transformed bytes not yet handed out.
  base::ssize cursor_ = 0;          // Bytes of pending_ already handed out.
  std::vector<std::byte> input_;    // Scratch for reads from the source.
  bool exhausted_ = false;  // The source ran out and the codec was flushed.
};

#endif  // CNC_RED_ALERT_TECH_TRANSFORM_SOURCE_H_
