// File: LcwSource, a source that LCW-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LCW_SOURCE_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LCW_SOURCE_H_

#include "engine/codec/block_backends.h"
#include "engine/stream/transform_source.h"

// Compresses or decompresses the bytes read through it with LCW; see
// BlockCodec.
//
// Example:
//   LcwSource decompressor(CodecMode::kDecompress, file_source, 4096);
using LcwSource = TransformSource<LcwCodec>;

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LCW_SOURCE_H_
