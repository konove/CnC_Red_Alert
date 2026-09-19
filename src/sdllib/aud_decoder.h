#ifndef CNC_RED_ALERT_SDLLIB_AUD_DECODER_H_
#define CNC_RED_ALERT_SDLLIB_AUD_DECODER_H_

// File: Decoders for the two block compressions of Westwood .AUD sound files.
//
// An .AUD file is an AudHeader followed by blocks, each an AudBlockHeader and
// a compressed payload. The functions decode one payload; they know nothing
// of files or of the mixer.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

// Bits of AudHeader::flags.
constexpr uint8_t kAudFlagStereo = 1;
constexpr uint8_t kAudFlag16Bit = 2;

// PWG 3-14-95: This structure used to have bit fields defined for Stereo
//   and Bits.  These were removed because watcom packs them into a 32 bit
//   flag entry even though they could have fit in a 8 bit entry.
#pragma pack(push, 1)
struct AudHeader {
  uint16_t sample_rate;        // Playback rate (hertz).
  int32_t compressed_bytes;    // Size of the data that follows the header.
  int32_t uncompressed_bytes;  // Size of the data once decoded.
  uint8_t flags;               // kAudFlagStereo, kAudFlag16Bit
  uint8_t compression;         // An AudCompression.
};

// Precedes each block of compressed data after the AudHeader.
struct AudBlockHeader {
  uint16_t compressed_bytes;  // Size of the block that follows.
  uint16_t decoded_bytes;     // Equal to compressed_bytes for a raw block.
  uint32_t magic;             // 0x0000DEAF; not checked.
};
#pragma pack(pop)

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
