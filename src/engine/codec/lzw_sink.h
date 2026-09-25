// File: LzwSink, a sink that LZW-compresses or decompresses in blocks.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LZW_SINK_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LZW_SINK_H_

#include "engine/codec/block_backends.h"
#include "engine/stream/transform_sink.h"

// Compresses or decompresses the bytes written to it with LZW; see BlockCodec.
//
// Example:
//   LzwSink compressor(CodecMode::kCompress, file_sink, 4096);
using LzwSink = TransformSink<LzwCodec>;

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LZW_SINK_H_
