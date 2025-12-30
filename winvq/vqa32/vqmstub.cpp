#include <array>
#include <cstdio>
#include <cstdint>

#include "vqm32/compress.h"
#include "vqm32/soscomp.h"

static constexpr std::array<int8_t, 16> ima_adpcm_index_table = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

static constexpr std::array<int16_t, 89> ima_adpcm_step_table = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

long AudioUnzap(void * /*source*/, void * /*dest*/, long /*uncomp_size*/) {
  printf("%s\n", __func__);
  return 0;
}

// oh look, another ADPCM decoder...
void VQA_sosCODECInitStream(_SOS_COMPRESS_INFO *info) {
  info->dwPredicted = info->dwPredicted2 = 0;
  info->wStep = info->wStep2 = 0;
}

uint64_t VQA_sosCODECDecompressData(_SOS_COMPRESS_INFO *info,
                                    uint64_t uncomp_size) {
  if (info->wChannels != 1 || info->wBitSize != 16) {
    printf("%s (%i/%i)\n", __func__, info->wChannels, info->wBitSize);
    return 0;
  }

  auto clamp = [](int32_t v, int32_t min, int32_t max) {
    return v < min ? min : (v > max ? max : v);
  };

  auto *in_ptr = reinterpret_cast<uint8_t *>(info->lpSource);
  auto *out_ptr = reinterpret_cast<int64_t *>(info->lpDest);

  uint64_t in_index = 0;
  uint64_t out_index = 0;

  for (; uncomp_size != 0; uncomp_size -= 4) {
    const auto adpcm_byte = in_ptr[in_index++];

    uint32_t nibble = adpcm_byte & 0xF;
    int32_t step = ima_adpcm_step_table[info->wStep];
    info->wStep = clamp(info->wStep + ima_adpcm_index_table[nibble], 0, 88);

    int32_t diff =
        ((((nibble & 7u) * 2 + 1) * step) >> 3) * (nibble & 8 ? -1 : 1);
    info->dwPredicted = clamp(info->dwPredicted + diff, -32768, 32767);

    out_ptr[out_index++] = info->dwPredicted;

    nibble = adpcm_byte >> 4;
    step = ima_adpcm_step_table[info->wStep];
    info->wStep = clamp(info->wStep + ima_adpcm_index_table[nibble], 0, 88);

    diff = ((((nibble & 7) * 2 + 1) * step) >> 3) * (nibble & 8 ? -1 : 1);
    info->dwPredicted = clamp(info->dwPredicted + diff, -32768, 32767);

    out_ptr[out_index++] = info->dwPredicted;
  }

  return 0;
}
