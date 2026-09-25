// File: LcwSource, a source that LCW-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_TECH_LCW_SOURCE_H_
#define CNC_RED_ALERT_TECH_LCW_SOURCE_H_

#include "engine/stream/transform_source.h"
#include "tech/block_backends.h"

// Compresses or decompresses the bytes read through it with LCW; see
// BlockCodec.
//
// Example:
//   LcwSource decompressor(CodecMode::kDecompress, file_source, 4096);
using LcwSource = TransformSource<LcwCodec>;

#endif  // CNC_RED_ALERT_TECH_LCW_SOURCE_H_
