// File: the LZO, LCW and LZW compressors behind BlockCodec.

#ifndef CNC_RED_ALERT_TECH_BLOCK_BACKENDS_H_
#define CNC_RED_ALERT_TECH_BLOCK_BACKENDS_H_

#include <cstddef>
#include <span>
#include <vector>

#include "tech/block_codec.h"

// LZO1X-1, which saved games use.
class LzoBackend {
 public:
  // The decoder is given room for twice a block.
  static int Capacity(int block_size) { return 2 * block_size; }

  int Compress(std::span<const std::byte> input, std::span<std::byte> output);
  static int Decompress(std::span<const std::byte> input,
                        std::span<std::byte> output);

 private:
  // The compressor's dictionary, allocated on first use.
  std::vector<std::byte> work_;
};

// Westwood's LCW, which map and overlay packs use.
class LcwBackend {
 public:
  // Room for an incompressible block plus the header the old straw stored in
  // front of it; the header check depends on the exact size.
  static int Capacity(int block_size);

  static int Compress(std::span<const std::byte> input,
                      std::span<std::byte> output);
  static int Decompress(std::span<const std::byte> input,
                        std::span<std::byte> output);
};

// Westwood's LZW.
class LzwBackend {
 public:
  // Room for an incompressible block plus the header the old straw stored in
  // front of it; the header check depends on the exact size.
  static int Capacity(int block_size);

  static int Compress(std::span<const std::byte> input,
                      std::span<std::byte> output);
  static int Decompress(std::span<const std::byte> input,
                        std::span<std::byte> output);
};

using LzoCodec = BlockCodec<LzoBackend>;
using LcwCodec = BlockCodec<LcwBackend>;
using LzwCodec = BlockCodec<LzwBackend>;

#endif  // CNC_RED_ALERT_TECH_BLOCK_BACKENDS_H_
