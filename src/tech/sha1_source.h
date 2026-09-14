// File: Sha1Source, a source that computes the SHA-1 digest of what it reads.

#ifndef CNC_RED_ALERT_TECH_SHA1_SOURCE_H_
#define CNC_RED_ALERT_TECH_SHA1_SOURCE_H_

#include "tech/sha.h"
#include "tech/sha1_codec.h"
#include "tech/transform_source.h"

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

  // Returns the digest of the bytes read so far.
  [[nodiscard]] Sha1Digest digest() const { return codec().digest(); }
};

#endif  // CNC_RED_ALERT_TECH_SHA1_SOURCE_H_
