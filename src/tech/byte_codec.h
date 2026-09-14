// File: ByteCodec, the interface of a byte transform that TransformSink
// pushes bytes through and TransformSource pulls bytes through, so that each
// transform is written once for both directions.

#ifndef CNC_RED_ALERT_TECH_BYTE_CODEC_H_
#define CNC_RED_ALERT_TECH_BYTE_CODEC_H_

#include <concepts>
#include <cstddef>
#include <span>

#include "base/types.h"
#include "tech/byte_sink.h"

// A byte transform, such as a compressor or a cipher.
//
// - Process(in, out) consumes all of in, buffering what it cannot transform
//   yet and writing each complete piece of output to out. Returns false if
//   the input cannot be transformed or out failed.
// - Flush(out) writes whatever is still buffered: a partial block, a cipher
//   tail, padding. The codec stays usable. Returns false if what is buffered
//   cannot be transformed (a truncated compressed block) or out failed.
// - ok() is false once the input has turned out to be undecodable.
// - BytesWanted(output_needed) is how many input bytes to give the codec
//   next; always at least one. A pull adapter reads exactly that many, so it
//   never takes more from its source than the codec can use: a mixfile
//   header decode leaves the file positioned for the next reader.
//   output_needed is how many output bytes the reader still wants, which a
//   codec that passes data through unchanged returns as is.
template <class C>
concept ByteCodec =
    requires(C codec, const C& const_codec, std::span<const std::byte> in,
             ByteSink& out, base::ssize output_needed) {
      { codec.Process(in, out) } -> std::same_as<bool>;
      { codec.Flush(out) } -> std::same_as<bool>;
      { const_codec.ok() } -> std::same_as<bool>;
      { const_codec.BytesWanted(output_needed) } -> std::same_as<base::ssize>;
    };

#endif  // CNC_RED_ALERT_TECH_BYTE_CODEC_H_
