// LCW, Westwood's LZ77-style compression, as used by shapes, key frames,
// animations and compressed pictures.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LCW_UNCOMPRESS_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LCW_UNCOMPRESS_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"

// Decodes the LCW stream in `source` and returns its output. Stops at the end
// marker, after `max_size` bytes, or at the first malformed command, and
// returns what it decoded up to there: a damaged stream gives a short result,
// not an error. `max_size` bytes are allocated up front, so bound a size read
// from a file before passing it.
std::vector<uint8_t> LcwUncompress(std::span<const std::byte> source,
                                   base::ssize max_size);

// Decodes an LCW stream that lies in its destination's own storage, at or
// after `dest`'s start, as WSA animations and VQA movies load it: the output
// grows from the front of `dest`, behind the input still to be read. Stops as
// LcwUncompress() does, at `dest`'s size, and returns the number of bytes
// written. A stream stored elsewhere decodes into `dest` just the same.
int32_t LcwUncompressInPlace(std::span<const std::byte> source,
                             std::span<std::byte> dest);
int32_t LcwUncompressInPlace(std::span<const unsigned char> source,
                             std::span<unsigned char> dest);

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LCW_UNCOMPRESS_H_
