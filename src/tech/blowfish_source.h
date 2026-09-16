// File: BlowfishSource, a source that encrypts or decrypts with Blowfish.

#ifndef CNC_RED_ALERT_TECH_BLOWFISH_SOURCE_H_
#define CNC_RED_ALERT_TECH_BLOWFISH_SOURCE_H_

#include "tech/blowfish_codec.h"
#include "tech/transform_source.h"

// Encrypts or decrypts the bytes read through it in whole 8-byte blocks; a
// shorter tail at the end of the source passes through unchanged.
//
// Example:
//   BlowfishSource cipher(CipherMode::kDecrypt, file_source);
//   cipher.Key(key, BlowfishEngine::kMaxKeyLength);
class BlowfishSource : public TransformSource<BlowfishCodec> {
 public:
  using TransformSource::TransformSource;

  // Keys the cipher. Until then bytes pass through unchanged.
  void Key(const void* key, int length) { codec().Key(key, length); }
};

#endif  // CNC_RED_ALERT_TECH_BLOWFISH_SOURCE_H_
