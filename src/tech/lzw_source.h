// File: LzwSource, a source that LZW-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_TECH_LZW_SOURCE_H_
#define CNC_RED_ALERT_TECH_LZW_SOURCE_H_

#include "tech/block_backends.h"
#include "tech/transform_source.h"

// Compresses or decompresses the bytes read through it with LZW; see
// BlockCodec.
//
// Example:
//   LzwSource decompressor(CodecMode::kDecompress, file_source, 4096);
using LzwSource = TransformSource<LzwCodec>;

#endif  // CNC_RED_ALERT_TECH_LZW_SOURCE_H_
