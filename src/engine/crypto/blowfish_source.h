// File: BlowfishSource, a source that encrypts or decrypts with Blowfish.

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_SOURCE_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_SOURCE_H_

#include "engine/crypto/blowfish_codec.h"
#include "engine/stream/transform_source.h"

// Encrypts or decrypts the bytes read through it in whole 8-byte blocks; a
// shorter tail at the end of the source passes through unchanged.
//
// Example:
//   BlowfishSource cipher(CipherMode::kDecrypt, file_source);
//   cipher.Key(key, BlowfishEngine::kMaxKeyLength);
class BlowfishSource : public TransformSource<BlowfishCodec> {
 public:
  using TransformSource::TransformSource;
  ~BlowfishSource() override;
  BlowfishSource(const BlowfishSource&) = delete;
  BlowfishSource& operator=(const BlowfishSource&) = delete;
  BlowfishSource(BlowfishSource&&) = delete;
  BlowfishSource& operator=(BlowfishSource&&) = delete;

  // Keys the cipher. Until then bytes pass through unchanged.
  void Key(std::span<const std::byte> key) { codec().Key(key); }
};

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_BLOWFISH_SOURCE_H_
