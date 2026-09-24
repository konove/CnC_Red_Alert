// File: C++ stand-ins for the VQM32 sound decoders the VQA loader calls,
// which were all assembly: the IMA ADPCM decoder for SND2 sound, and a stub
// where the ZAP decoder for SND1 sound would be.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <span>
#include <utility>

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
// the SDL port does not build. Writing nothing, the stub cannot overrun dest;
// a real decoder must stop at its end.
int32_t AudioUnzap(std::span<const unsigned char> /*source*/,
                   std::span<unsigned char> /*dest*/) {
  absl::PrintF("%s\n", __func__);
  return 0;
}

void ResetAdpcmStream(AdpcmStream* stream) {
  stream->predicted = 0;
  stream->step_index = 0;
}

bool DecodeAdpcmSound(AdpcmStream* stream, int32_t output_bytes) {
  // The only format the movies use; see soscomp.h.
  if (stream->channels != 1 || stream->bits_per_sample != 16) {
    absl::FPrintF(stderr, "%s (%d/%d)\n", __func__, stream->channels,
                  stream->bits_per_sample);
    return false;
  }

  auto input = stream->source;
  auto output = stream->dest;
  if (output_bytes < 0 || std::cmp_greater(output_bytes, output.size()) ||
      std::cmp_greater(output_bytes / 4, input.size())) {
    return false;
  }

  // One input byte is two samples, 4 output bytes. A trailing part of that
  // (output_bytes not a multiple of 4) is left undecoded.
  while (output_bytes >= 4) {
    const uint8_t code_pair = input.front();
    input = input.subspan(1);

    // A byte holds two 4-bit codes, the low nibble first.
    for (int i = 0; i < 2; ++i) {
      const auto code =
          static_cast<uint8_t>(i == 0 ? code_pair & 0x0F : code_pair >> 4);

      // This sample uses the step size before the update below.
      const int step = base::At(kImaAdpcmStepTable, stream->step_index);

      const int index_delta = base::At(kImaAdpcmIndexTable, code);
      stream->step_index = static_cast<int16_t>(
          std::clamp(stream->step_index + index_delta, 0, 88));

      // The difference is (magnitude + 1/2) * step / 4, the magnitude being
      // the code's low 3 bits and bit 3 its sign. The IMA reference adds the
      // shifted steps bit by bit and truncates each; this truncates once, so
      // it can come out up to 3 higher.
      const int sign = code & 8 ? -1 : 1;
      const int difference = (((((code & 7) * 2) + 1) * step) / 8) * sign;

      // The prediction saturates at the 16-bit range.
      stream->predicted =
          std::clamp(stream->predicted + difference, -32768, 32767);

      const auto sample = static_cast<int16_t>(stream->predicted);
      base::CopyBytes(std::as_writable_bytes(output), base::ObjectBytes(sample),
                      sizeof(sample));
      output = output.subspan(sizeof(sample));
    }

    output_bytes -= 4;
  }

  // The next chunk carries on from here.
  stream->source = input;
  stream->dest = output;

  return true;
}
