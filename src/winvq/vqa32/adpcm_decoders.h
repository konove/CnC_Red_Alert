// File: the decoder for a VQA movie's SND2 sound, IMA ADPCM. SND1 sound is
// Westwood's ZAP, which engine/codec/aud_decoder.h decodes for the .AUD
// files.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_ADPCM_DECODERS_H_
#define CNC_RED_ALERT_WINVQ_VQA32_ADPCM_DECODERS_H_

#include <cstdint>
#include <span>

// Decodes a stream of IMA ADPCM, 4 bits a sample, into 16-bit mono samples.
// The stream runs on from one sound chunk to the next, so one decoder serves
// a whole movie; a new one is at the stream's start.
//
// Example:
//   ImaAdpcmDecoder decoder;
//   for (each chunk) decoder.Decode(chunk_bytes, samples);
class ImaAdpcmDecoder {
 public:
  // Whether Decode() produces this format. The movies only use 16-bit mono.
  static bool Supports(int channels, int bits_per_sample);

  // Decodes source to fill dest, two samples (4 bytes) per source byte; a
  // trailing part of a sample in dest is left as it was. source and dest may
  // overlap, with source at the end. Returns false, decoding nothing, when
  // source is too short to fill dest.
  bool Decode(std::span<const uint8_t> source, std::span<uint8_t> dest);

 private:
  // The last sample, from which the next is predicted.
  int32_t predicted_ = 0;
  // Index in the step table of the quantizer step the next code is scaled by.
  int16_t step_index_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_ADPCM_DECODERS_H_
