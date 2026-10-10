#include "engine/codec/aud_decoder.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The decoders are engine_rs's adpcm and westwood_audio modules
// (src/rust/src/adpcm.rs, westwood_audio.rs).

std::vector<int16_t> DecodeAdpcmBlock(AdpcmState& state,
                                      const std::span<const std::byte> block) {
  // Rust fills the vector this returns; one it returned would need copying.
  std::vector<int16_t> samples(block.size() * 2);
  // Rust reads an index outside the step table as its last entry, so a
  // negative one, which no decode leaves behind, is harmless.
  engine_rs::ImaAdpcmState rust_state{
      .predictor = state.predictor,
      .step_index = static_cast<uint8_t>(state.step_index)};
  engine_rs::ima_adpcm_decode_samples(rust_state, AsRustSlice(block),
                                      {samples.data(), samples.size()});
  state.predictor = rust_state.predictor;
  state.step_index = static_cast<int8_t>(rust_state.step_index);
  return samples;
}

std::optional<std::vector<uint8_t>> DecodeWestwoodBlock(
    const std::span<const std::byte> block) {
  const auto rust_block = AsRustSlice(block);
  // A first pass over the commands sizes the vector, so Rust decodes straight
  // into it.
  const base::ssize size = engine_rs::westwood_audio_decoded_len(rust_block);
  if (size < 0) {
    return std::nullopt;
  }
  std::vector<uint8_t> samples(base::ToSize(size));
  if (!engine_rs::westwood_audio_decode(
          rust_block,
          AsRustSlice(std::as_writable_bytes(std::span(samples))))) {
    return std::nullopt;
  }
  return samples;
}
