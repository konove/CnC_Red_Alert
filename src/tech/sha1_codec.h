// File: Sha1Codec, a SHA-1 digest of the bytes passing a stream.

#ifndef CNC_RED_ALERT_TECH_SHA1_CODEC_H_
#define CNC_RED_ALERT_TECH_SHA1_CODEC_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/types.h"
#include "tech/byte_sink.h"
#include "tech/sha.h"

// A ByteCodec that passes bytes through unchanged while hashing them.
class Sha1Codec {
 public:
  bool Process(std::span<const std::byte> in, ByteSink& out) {
    engine_.Hash(in);
    return out.Write(in);
  }
  // Nothing is ever buffered.
  static bool Flush(ByteSink& /*out*/) { return true; }
  [[nodiscard]] static bool ok() { return true; }
  // Bytes pass straight through, so take no more than the reader asked for.
  [[nodiscard]] static base::ssize BytesWanted(base::ssize output_needed) {
    return output_needed;
  }

  // Returns the digest of the bytes that have passed so far.
  [[nodiscard]] Sha1Digest digest() const { return engine_.Digest(); }

 private:
  SHAEngine engine_;
};

#endif  // CNC_RED_ALERT_TECH_SHA1_CODEC_H_
