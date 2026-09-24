// LCW, Westwood's LZ77-style compression, as used by shapes, key frames,
// animations and compressed pictures.

#ifndef CNC_RED_ALERT_SDLLIB_LCW_UNCOMPRESS_H_
#define CNC_RED_ALERT_SDLLIB_LCW_UNCOMPRESS_H_

#include <cstddef>
#include <cstdint>
#include <span>

// Decodes an LCW stream from `source` into `dest` without reading or writing
// outside either span. Stops at the end marker, when `dest` is full, or at the
// first malformed command, and returns the number of bytes written so far.
int32_t LCW_Uncompress(std::span<const std::byte> source,
                       std::span<std::byte> dest);
int32_t LCW_Uncompress(std::span<const unsigned char> source,
                       std::span<unsigned char> dest);

#endif  // CNC_RED_ALERT_SDLLIB_LCW_UNCOMPRESS_H_
