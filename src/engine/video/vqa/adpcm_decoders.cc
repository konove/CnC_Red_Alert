#include "engine/video/vqa/adpcm_decoders.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/log/check.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

bool ImaAdpcmDecoder::Supports(const int channels, const int bits_per_sample) {
  return channels == 1 && bits_per_sample == 16;
}

// The decoder is engine_rs's adpcm module (src/rust/src/adpcm.rs).
bool ImaAdpcmDecoder::Decode(const std::span<const uint8_t> source,
                             const std::span<uint8_t> dest) {
  const auto source_bytes = std::as_bytes(source);
  const auto dest_bytes = std::as_writable_bytes(dest);
  engine_rs::ImaAdpcmState state{
      .predictor = static_cast<int16_t>(predicted_),
      .step_index = static_cast<uint8_t>(step_index_)};
  bool decoded = false;
  if (!Overlaps(source_bytes, dest_bytes)) {
    decoded = engine_rs::ima_adpcm_decode_bytes(
        state, AsRustSlice(source_bytes), AsRustSlice(dest_bytes));
  } else {
    // Rust may not see one buffer as a shared and a mutable slice at once, so
    // the whole of it crosses once.
    const auto dest_begin = std::bit_cast<uintptr_t>(dest.data());
    const auto source_begin = std::bit_cast<uintptr_t>(source.data());
    CHECK_GE(source_begin, dest_begin)
        << "ADPCM stream overlaps the front of its destination";
    const std::size_t source_start = source_begin - dest_begin;
    const std::size_t buffer_size =
        std::max(dest.size(), source_start + source.size());
    // Both spans lie in one array, which runs at least this far.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<std::byte> buffer(dest_bytes.data(), buffer_size);
    decoded = engine_rs::ima_adpcm_decode_bytes_in_place(
        state, AsRustSlice(buffer), source_start, source.size(), dest.size());
  }
  predicted_ = state.predictor;
  step_index_ = state.step_index;
  return decoded;
}
