// File: LcwSink, a sink that LCW-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_TECH_LCW_SINK_H_
#define CNC_RED_ALERT_TECH_LCW_SINK_H_

#include "engine/stream/transform_sink.h"
#include "tech/block_backends.h"

// Compresses or decompresses the bytes written to it with LCW; see BlockCodec.
//
// Example:
//   LcwSink compressor(CodecMode::kCompress, file_sink, 4096);
using LcwSink = TransformSink<LcwCodec>;

#endif  // CNC_RED_ALERT_TECH_LCW_SINK_H_
