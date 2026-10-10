// The C++ VQ frame decoder that engine_rs's vq module replaced
// (src/rust/src/vq.rs), kept so that a differential test and kernel_bench can
// hold the Rust one to it.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQ_DECODER_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQ_DECODER_REFERENCE_TEST_UTIL_H_

#include <span>

#include "engine/video/vqa/vq_decoder.h"

namespace vq_decoder_reference {

// As ::DecodeVqFrame() (engine/video/vqa/vq_decoder.h).
void DecodeVqFrame(BlockShape shape, std::span<const unsigned char> codebook,
                   std::span<const unsigned char> pointers,
                   std::span<unsigned char> buffer, int blocks_per_row,
                   int block_rows, int stride);

}  // namespace vq_decoder_reference

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQ_DECODER_REFERENCE_TEST_UTIL_H_
