// File: Base64Sink, a sink that Base64-encodes or decodes.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_BASE64_SINK_H_
#define CNC_RED_ALERT_ENGINE_CODEC_BASE64_SINK_H_

#include "engine/codec/base64_codec.h"
#include "engine/stream/transform_sink.h"

// Encodes or decodes the bytes written to it; see Base64Codec.
//
// Example:
//   Base64Sink decoder(Base64Mode::kDecode, span_sink);
using Base64Sink = TransformSink<Base64Codec>;

#endif  // CNC_RED_ALERT_ENGINE_CODEC_BASE64_SINK_H_
