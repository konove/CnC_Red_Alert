// File: LzoSink, a sink that LZO-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_TECH_LZO_SINK_H_
#define CNC_RED_ALERT_TECH_LZO_SINK_H_

#include "tech/block_backends.h"
#include "tech/transform_sink.h"

// Compresses or decompresses the bytes written to it with LZO; see BlockCodec.
//
// Example:
//   LzoSink compressor(CodecMode::kCompress, file_sink, 4096);
using LzoSink = TransformSink<LzoCodec>;

#endif  // CNC_RED_ALERT_TECH_LZO_SINK_H_
