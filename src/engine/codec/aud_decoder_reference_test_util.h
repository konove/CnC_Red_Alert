// The C++ .AUD block decoders that engine_rs's adpcm and westwood_audio
// modules replaced (src/rust/src), kept so that a differential test and
// kernel_bench can hold the Rust ones to them.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_AUD_DECODER_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CODEC_AUD_DECODER_REFERENCE_TEST_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "engine/codec/aud_decoder.h"

namespace aud_decoder_reference {

// As ::DecodeAdpcmBlock() and ::DecodeWestwoodBlock()
// (engine/codec/aud_decoder.h).
std::vector<int16_t> DecodeAdpcmBlock(AdpcmState& state,
                                      std::span<const std::byte> block);
std::optional<std::vector<uint8_t>> DecodeWestwoodBlock(
    std::span<const std::byte> block);

}  // namespace aud_decoder_reference

#endif  // CNC_RED_ALERT_ENGINE_CODEC_AUD_DECODER_REFERENCE_TEST_UTIL_H_
