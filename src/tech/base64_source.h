// File: Base64Source, a source that Base64-encodes or decodes.

#ifndef CNC_RED_ALERT_TECH_BASE64_SOURCE_H_
#define CNC_RED_ALERT_TECH_BASE64_SOURCE_H_

#include "engine/stream/transform_source.h"
#include "tech/base64_codec.h"

// Encodes or decodes the bytes read through it; see Base64Codec.
//
// Example:
//   Base64Source encoder(Base64Mode::kEncode, span_source);
using Base64Source = TransformSource<Base64Codec>;

#endif  // CNC_RED_ALERT_TECH_BASE64_SOURCE_H_
