// File: Sha1Source, a source that computes the SHA-1 digest of what it reads.

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_SOURCE_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_SOURCE_H_

#include "engine/crypto/sha.h"
#include "engine/crypto/sha1_codec.h"
#include "engine/stream/transform_source.h"

// Hands bytes out unchanged while hashing them. It reads no more from its
// source than it is asked for, so the digest covers exactly what was read.
//
// Example:
//   Sha1Source hash(file_source);
//   ...
//   const Sha1Digest digest = hash.digest();
class Sha1Source : public TransformSource<Sha1Codec> {
 public:
  using TransformSource::TransformSource;
  ~Sha1Source() override;
  Sha1Source(const Sha1Source&) = delete;
  Sha1Source& operator=(const Sha1Source&) = delete;
  Sha1Source(Sha1Source&&) = delete;
  Sha1Source& operator=(Sha1Source&&) = delete;

  // Returns the digest of the bytes read so far.
  [[nodiscard]] Sha1Digest digest() const { return codec().digest(); }
};

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_SOURCE_H_
