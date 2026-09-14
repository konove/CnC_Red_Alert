// File: Sha1Sink, a sink that computes the SHA-1 digest of what it passes on.

#ifndef CNC_RED_ALERT_TECH_SHA1_SINK_H_
#define CNC_RED_ALERT_TECH_SHA1_SINK_H_

#include "tech/sha.h"
#include "tech/sha1_codec.h"
#include "tech/transform_sink.h"

// Passes bytes on unchanged while hashing them.
//
// Example:
//   Sha1Sink hash(file_sink);
//   ...
//   const Sha1Digest digest = hash.digest();
class Sha1Sink : public TransformSink<Sha1Codec> {
 public:
  using TransformSink::TransformSink;

  // Returns the digest of the bytes written so far.
  [[nodiscard]] Sha1Digest digest() const { return codec().digest(); }
};

#endif  // CNC_RED_ALERT_TECH_SHA1_SINK_H_
