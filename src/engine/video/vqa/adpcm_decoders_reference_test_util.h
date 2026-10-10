// The C++ VQA sound decoder that engine_rs's adpcm module replaced
// (src/rust/src/adpcm.rs), kept so that a differential test and kernel_bench
// can hold the Rust one to it.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_ADPCM_DECODERS_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_ADPCM_DECODERS_REFERENCE_TEST_UTIL_H_

#include <cstdint>
#include <span>

namespace adpcm_decoders_reference {

// As ::ImaAdpcmDecoder (engine/video/vqa/adpcm_decoders.h).
class ImaAdpcmDecoder {
 public:
  bool Decode(std::span<const uint8_t> source, std::span<uint8_t> dest);

 private:
  int32_t predicted_ = 0;
  int16_t step_index_ = 0;
};

}  // namespace adpcm_decoders_reference

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_ADPCM_DECODERS_REFERENCE_TEST_UTIL_H_
