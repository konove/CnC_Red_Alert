// File: Base64Codec, Base64 encoding and decoding of a byte stream.
//
// Replaces Westwood's Base64Pipe and Base64Straw (Joe L. Bostic, 1996); INI
// binary blocks depend on its bytes.

#ifndef CNC_RED_ALERT_TECH_BASE64_CODEC_H_
#define CNC_RED_ALERT_TECH_BASE64_CODEC_H_

#include <array>
#include <cstddef>
#include <span>

#include "base/types.h"
#include "tech/base64.h"
#include "tech/byte_sink.h"

// A ByteCodec that encodes three bytes at a time into four characters, or
// decodes four characters at a time into up to three bytes. Flush encodes a
// short final group with padding, or decodes whatever characters remain.
class Base64Codec {
 public:
  explicit Base64Codec(Base64Mode mode) : mode_(mode) {}

  bool Process(std::span<const std::byte> in, ByteSink& out);
  bool Flush(ByteSink& out);
  // Characters that are not Base64 are skipped, so nothing is undecodable.
  [[nodiscard]] static bool ok() { return true; }
  [[nodiscard]] base::ssize BytesWanted(base::ssize output_needed) const;

 private:
  // Bytes per input group: 3 plain bytes to encode, 4 characters to decode.
  [[nodiscard]] int GroupSize() const {
    return mode_ == Base64Mode::kEncode ? 3 : 4;
  }

  // Transforms the group filled so far and writes the result to out.
  bool EmitGroup(ByteSink& out);

  Base64Mode mode_;
  std::array<std::byte, 4> group_{};
  int count_ = 0;  // Bytes of group_ filled so far.
};

#endif  // CNC_RED_ALERT_TECH_BASE64_CODEC_H_
