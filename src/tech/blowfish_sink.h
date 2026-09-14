// File: BlowfishSink, a sink that encrypts or decrypts with Blowfish.

#ifndef CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_
#define CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_

#include "tech/blowfish_codec.h"
#include "tech/transform_sink.h"

// Encrypts or decrypts the bytes written to it in whole 8-byte blocks; a
// shorter tail passes through unchanged at Flush.
//
// Example:
//   BlowfishSink cipher(CipherMode::kEncrypt, file_sink);
//   cipher.Key(key, BlowfishEngine::MAX_KEY_LENGTH);
class BlowfishSink : public TransformSink<BlowfishCodec> {
 public:
  using TransformSink::TransformSink;

  // Keys the cipher. Until then bytes pass through unchanged.
  void Key(const void* key, int length) { codec().Key(key, length); }
};

#endif  // CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_
