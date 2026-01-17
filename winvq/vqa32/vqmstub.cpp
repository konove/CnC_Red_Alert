// don't need much from VQM32, and it's all asm
#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/soscomp.h"

static constexpr int8_t kImaAdpcmIndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8,
                                                 -1, -1, -1, -1, 2, 4, 6, 8};

static constexpr int16_t kImaAdpcmStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

long AudioUnzap(void* /*source*/, void* /*dest*/, long) {
  printf("%s\n", __func__);
  return 0;
}

// oh look, another ADPCM decoder...
void VQA_sosCODECInitStream(SosCompressInfo* info) {
  info->predicted = info->predicted2 = 0;
  info->step_index = info->step_index2 = 0;
}

// Returns true on success, false on failure.
bool DecompressVqaSosData(SosCompressInfo* info, std::size_t uncomp_size) {
  // Sanity check: This decoder only supports Mono 16-bit.
  if (info->channels != 1 || info->bit_size != 16) {
    std::fprintf(stderr, "%s (%d/%d)\n", __func__, info->channels,
                 info->bit_size);
    return false;
  }

  // Cast void*/uint8_t* to the specific types needed for processing.
  // source is read as bytes (uint8_t), dest is written as shorts (int16_t).
  auto* in_ptr = info->source;
  auto* out_ptr = reinterpret_cast<std::int16_t*>(info->dest);

  // Loop processes 4 output bytes (2 samples) per iteration.
  // We use >= 4 to prevent underflow if uncomp_size is not a multiple of 4.
  while (uncomp_size >= 4) {
    const std::uint8_t raw_byte = *in_ptr++;

    // A single byte contains two 4-bit ADPCM samples (nibbles).
    // We iterate 0 (low nibble) then 1 (high nibble).
    for (int i = 0; i < 2; ++i) {
      // 1. Extract Nibble
      // Low nibble (bits 0-3) first, then High nibble (bits 4-7)
      const int nibble = (i == 0) ? (raw_byte & 0x0F) : (raw_byte >> 4);

      // 2. Get current step size
      const int step = kImaAdpcmStepTable[info->step_index];

      // 3. Update Step Index for the NEXT sample
      const int index_delta = kImaAdpcmIndexTable[nibble];
      info->step_index = std::clamp(info->step_index + index_delta, 0, 88);

      // 4. Calculate Difference
      // Formula: ( (nibble * 2 + 1) * step ) / 8
      // Note: The logic (nibble & 7) masks out the sign bit for calculation
      const int sign = (nibble & 8) ? -1 : 1;
      const int diff = ((((nibble & 7) * 2 + 1) * step) >> 3) * sign;

      // 5. Update and Clamp Predicted Value
      info->predicted = std::clamp(info->predicted + diff, -32768, 32767);

      // 6. Output Sample
      *out_ptr++ = static_cast<std::int16_t>(info->predicted);
    }

    uncomp_size -= 4;
  }

  // Update the struct pointers to reflect the new position
  // (Optional: depends if the caller expects the struct pointers to move)
  info->source = in_ptr;
  info->dest = reinterpret_cast<std::uint8_t*>(out_ptr);

  return true;
}
