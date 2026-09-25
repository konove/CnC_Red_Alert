// File: LzoSource, a source that LZO-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LZO_SOURCE_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LZO_SOURCE_H_

#include "engine/codec/block_backends.h"
#include "engine/stream/transform_source.h"

// Compresses or decompresses the bytes read through it with LZO; see
// BlockCodec.
//
// Example:
//   LzoSource decompressor(CodecMode::kDecompress, file_source, 4096);
using LzoSource = TransformSource<LzoCodec>;

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LZO_SOURCE_H_
