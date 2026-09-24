// File: C++ stand-ins for the VQM32 sound decoders the VQA loader calls,
// which were all assembly: the IMA ADPCM decoder for SND2 sound, and a stub
// where the ZAP decoder for SND1 sound would be.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <span>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/soscomp.h"

// IMA ADPCM step index change for each 4-bit code: small codes step the
// quantizer down, large ones up. The sign bit (8) does not matter.
static constexpr int kImaAdpcmIndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8,
                                              -1, -1, -1, -1, 2, 4, 6, 8};

// IMA ADPCM quantizer step sizes, roughly 10% apart, for step indexes 0-88.
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

// The only ZAP decoder is the original assembly (vqm32/audunzap.asm), which
// the SDL port does not build. Writing nothing, the stub cannot exceed the
// destination size the loader passes; a real decoder must stop after size
// bytes.
int32_t AudioUnzap(void* /*source*/, void* /*dest*/, int32_t /*size*/) {
  absl::PrintF("%s\n", __func__);
  return 0;
}

void VQA_sosCODECInitStream(SosCompressInfo* info) {
  info->predicted = info->predicted2 = 0;
  info->step_index = info->step_index2 = 0;
}

bool DecompressVqaSosData(SosCompressInfo* info, int32_t uncomp_size) {
  // The only format the movies use; see soscomp.h.
  if (info->channels != 1 || info->bit_size != 16) {
    absl::FPrintF(stderr, "%s (%d/%d)\n", __func__, info->channels,
                  info->bit_size);
    return false;
  }

  auto in_ptr = info->source;
  auto out_ptr = info->dest;
  if (uncomp_size < 0 || static_cast<size_t>(uncomp_size) > out_ptr.size() ||
      static_cast<size_t>(uncomp_size / 4) > in_ptr.size()) {
    return false;
  }

  // One input byte is two samples, 4 output bytes. A trailing part of that
  // (uncomp_size not a multiple of 4) is left undecoded.
  while (uncomp_size >= 4) {
    const uint8_t raw_byte = in_ptr.front();
    in_ptr = in_ptr.subspan(1);

    // A byte holds two 4-bit codes, the low nibble first.
    for (int i = 0; i < 2; ++i) {
      const auto nibble =
          static_cast<std::uint8_t>(i == 0 ? raw_byte & 0x0F : raw_byte >> 4);

      // This sample uses the step size before the update below.
      const int step = base::At(kImaAdpcmStepTable, info->step_index);

      const int index_delta = base::At(kImaAdpcmIndexTable, nibble);
      info->step_index = static_cast<std::int16_t>(std::clamp(info->step_index + index_delta, 0, 88));

      // The difference is (magnitude + 1/2) * step / 4, the magnitude being
      // the code's low 3 bits and bit 3 its sign. The IMA reference adds the
      // shifted steps bit by bit and truncates each; this truncates once, so
      // it can come out up to 3 higher.
      const int sign = nibble & 8 ? -1 : 1;
      const int diff = (((((nibble & 7) * 2) + 1) * step) / 8) * sign;

      // The prediction saturates at the 16-bit range.
      info->predicted = std::clamp(info->predicted + diff, -32768, 32767);

      const auto sample = static_cast<std::int16_t>(info->predicted);
      base::CopyBytes(std::as_writable_bytes(out_ptr),
                      base::ObjectBytes(sample), sizeof(sample));
      out_ptr = out_ptr.subspan(sizeof(sample));
    }

    uncomp_size -= 4;
  }

  // The next chunk carries on from here.
  info->source = in_ptr;
  info->dest = out_ptr;

  return true;
}
