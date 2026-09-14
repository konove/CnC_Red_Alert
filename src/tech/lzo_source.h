// File: LzoSource, a source that LZO-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_TECH_LZO_SOURCE_H_
#define CNC_RED_ALERT_TECH_LZO_SOURCE_H_

#include "tech/block_backends.h"
#include "tech/transform_source.h"

// Compresses or decompresses the bytes read through it with LZO; see
// BlockCodec.
//
// Example:
//   LzoSource decompressor(CodecMode::kDecompress, file_source, 4096);
using LzoSource = TransformSource<LzoCodec>;

#endif  // CNC_RED_ALERT_TECH_LZO_SOURCE_H_
