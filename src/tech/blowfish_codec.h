// File: BlowfishCodec, Blowfish encryption and decryption of a byte stream
// in whole 8-byte blocks.
//
// Replaces Westwood's BlowPipe and BlowStraw (Joe L. Bostic, 1996) and
// produces the same bytes: saved games depend on it.

#ifndef CNC_RED_ALERT_TECH_BLOWFISH_CODEC_H_
#define CNC_RED_ALERT_TECH_BLOWFISH_CODEC_H_

#include <array>
#include <cstddef>
#include <optional>
#include <span>

#include "base/types.h"
#include "tech/blowfish.h"
#include "tech/byte_sink.h"

// A ByteCodec that encrypts or decrypts whole 8-byte blocks. A tail shorter
// than a block passes through unchanged when flushed, and so does everything
// until Key() has been called.
class BlowfishCodec {
 public:
  explicit BlowfishCodec(CipherMode mode) : mode_(mode) {}

  // Keys the cipher with length bytes of key; at most
  // BlowfishEngine::kMaxKeyLength are used.
  void Key(const void* key, int length);

  bool Process(std::span<const std::byte> in, ByteSink& out);
  bool Flush(ByteSink& out);
  // A cipher never meets undecodable input.
  [[nodiscard]] static bool ok() { return true; }
  [[nodiscard]] base::ssize BytesWanted(base::ssize output_needed) const;

 private:
  static constexpr base::ssize kBlockSize = 8;

  CipherMode mode_;
  std::optional<BlowfishEngine> engine_;  // Empty until a key is submitted.
  std::array<std::byte, kBlockSize> block_{};
  base::ssize count_ = 0;  // Bytes of block_ filled so far.
};

#endif  // CNC_RED_ALERT_TECH_BLOWFISH_CODEC_H_
