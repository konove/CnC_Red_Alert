#ifndef CNC_RED_ALERT_SDLLIB_AUD_DECODER_H_
#define CNC_RED_ALERT_SDLLIB_AUD_DECODER_H_

// File: Decoders for the two block compressions of Westwood .AUD sound files.
//
// An .AUD file is an AudHeader followed by blocks, each an 8-byte block header
// and a compressed payload. These functions decode one payload; they know
// nothing of files or of the mixer.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

// What AudHeader::compression says about the blocks.
enum class AudCompression : uint8_t {
  SCOMP_NONE = 0,      // No compression -- raw data.
  SCOMP_WESTWOOD = 1,  // Sliding delta compression of 8-bit samples.
  SCOMP_SONARC = 33,   // Sonarc frame compression; never decoded here.
  SCOMP_SOS = 99       // IMA ADPCM, 4 bits per 16-bit sample.
};
using enum AudCompression;

// The running state of an IMA ADPCM stream. It carries over from one block to
// the next and starts from zero with each sound.
struct AdpcmState {
  int16_t predictor = 0;  // the last sample decoded
  int8_t step_index = 0;  // index into the IMA step table, 0..88
};

// Decodes an SCOMP_SOS block into two 16-bit samples for every byte of
// `block` (low nibble first).
std::vector<int16_t> DecodeAdpcmBlock(AdpcmState& state,
                                      std::span<const std::byte> block);

// Decodes an SCOMP_WESTWOOD block into unsigned 8-bit samples. Each block is
// decoded on its own, starting from the midpoint 0x80. Returns nullopt if a
// command runs past the end of `block`.
std::optional<std::vector<uint8_t>> DecodeWestwoodBlock(
    std::span<const std::byte> block);

#endif  // CNC_RED_ALERT_SDLLIB_AUD_DECODER_H_
