#include "sdllib/aud_decoder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "base/array.h"

namespace {

constexpr int8_t kImaIndexTable[16] = {-1, -1, -1, -1, 2, 4, 6, 8,
                                       -1, -1, -1, -1, 2, 4, 6, 8};

constexpr int16_t kImaStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

constexpr int8_t kWestwood2BitDeltas[4] = {-2, -1, 0, 1};
constexpr int8_t kWestwood4BitDeltas[16] = {-9, -8, -6, -5, -4, -3, -2, -1,
                                            0,  1,  2,  3,  4,  5,  6,  8};

// Decodes one 4-bit IMA code: bit 3 is the sign, bits 0-2 the magnitude in
// eighths of the current step.
int16_t DecodeAdpcmNibble(AdpcmState& state, const unsigned nibble) {
  const int step = base::At(kImaStepTable, state.step_index);
  state.step_index = static_cast<int8_t>(
      std::clamp(state.step_index + base::At(kImaIndexTable, nibble), 0, 88));

  const int magnitude = static_cast<int>((((nibble & 7U) * 2) + 1)) * step / 8;
  const int diff = (nibble & 8U) != 0 ? -magnitude : magnitude;
  state.predictor =
      static_cast<int16_t>(std::clamp(state.predictor + diff, -32768, 32767));
  return state.predictor;
}

// Adds `delta` to the running 8-bit sample, saturating, and emits it.
void PutDelta(int& sample, const int delta, std::vector<uint8_t>& samples) {
  sample = std::clamp(sample + delta, 0, 0xFF);
  samples.push_back(static_cast<uint8_t>(sample));
}

}  // namespace

std::vector<int16_t> DecodeAdpcmBlock(AdpcmState& state,
                                      const std::span<const std::byte> block) {
  std::vector<int16_t> samples;
  samples.reserve(block.size() * 2);
  for (const std::byte packed : block) {
    const auto codes = std::to_integer<unsigned>(packed);
    samples.push_back(DecodeAdpcmNibble(state, codes & 0xFU));
    samples.push_back(DecodeAdpcmNibble(state, codes >> 4U));
  }
  return samples;
}

std::optional<std::vector<uint8_t>> DecodeWestwoodBlock(
    std::span<const std::byte> block) {
  std::vector<uint8_t> samples;
  // What 2-bit deltas come to; only long runs of one sample exceed it.
  samples.reserve(block.size() * 4);
  int sample = 0x80;
  while (!block.empty()) {
    // The top two bits of a command byte select what follows, the low six
    // are a count, stored less one, or for command 2 possibly a delta.
    const auto command_byte = std::to_integer<unsigned>(block.front());
    block = block.subspan(1);
    const unsigned command = command_byte >> 6U;
    const unsigned low_bits = command_byte & 0x3FU;
    const std::size_t count = low_bits + 1;

    switch (command) {
      case 0:  // `count` bytes of four 2-bit deltas, lowest bits first.
      case 1:  // `count` bytes of two 4-bit deltas, low nibble first.
      {
        if (count > block.size()) {
          return std::nullopt;
        }
        for (const std::byte packed : block.first(count)) {
          auto codes = std::to_integer<unsigned>(packed);
          if (command == 0) {
            for (int i = 0; i < 4; i++, codes >>= 2U) {
              PutDelta(sample, base::At(kWestwood2BitDeltas, codes & 3U),
                       samples);
            }
          } else {
            for (int i = 0; i < 2; i++, codes >>= 4U) {
              PutDelta(sample, base::At(kWestwood4BitDeltas, codes & 0xFU),
                       samples);
            }
          }
        }
        block = block.subspan(count);
        break;
      }
      case 2:
        if ((low_bits & 0x20U) != 0) {
          // The low five bits are one signed delta. Unlike the packed deltas
          // it wraps rather than saturates.
          const int delta = (low_bits & 0x10U) != 0
                                ? static_cast<int>(low_bits & 0x1FU) - 32
                                : static_cast<int>(low_bits & 0xFU);
          sample = static_cast<uint8_t>(sample + delta);
          samples.push_back(static_cast<uint8_t>(sample));
        } else {
          // `count` raw samples follow.
          if (count > block.size()) {
            return std::nullopt;
          }
          for (const std::byte raw : block.first(count)) {
            samples.push_back(std::to_integer<uint8_t>(raw));
          }
          sample = samples.back();
          block = block.subspan(count);
        }
        break;
      default:  // 3: the current sample repeated `count` times.
        samples.insert(samples.end(), count, static_cast<uint8_t>(sample));
        break;
    }
  }
  return samples;
}
