// File: the LZO, LCW and LZW compressors behind BlockCodec.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_BLOCK_BACKENDS_H_
#define CNC_RED_ALERT_ENGINE_CODEC_BLOCK_BACKENDS_H_

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "base/types.h"
#include "engine/codec/block_codec.h"

// LZO1X-1, which saved games use.
class LzoBackend {
 public:
  // The decoder is given room for twice a block.
  static int Capacity(int block_size) { return 2 * block_size; }

  std::vector<std::byte> Compress(std::span<const std::byte> input);
  static std::optional<std::vector<std::byte>> Decompress(
      std::span<const std::byte> input, base::ssize max_size);

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

  static std::vector<std::byte> Compress(std::span<const std::byte> input);
  static std::optional<std::vector<std::byte>> Decompress(
      std::span<const std::byte> input, base::ssize max_size);
};

// Westwood's LZW.
class LzwBackend {
 public:
  // Room for an incompressible block plus the header the old straw stored in
  // front of it; the header check depends on the exact size.
  static int Capacity(int block_size);

  static std::vector<std::byte> Compress(std::span<const std::byte> input);
  static std::optional<std::vector<std::byte>> Decompress(
      std::span<const std::byte> input, base::ssize max_size);
};

using LzoCodec = BlockCodec<LzoBackend>;
using LcwCodec = BlockCodec<LcwBackend>;
using LzwCodec = BlockCodec<LzwBackend>;

#endif  // CNC_RED_ALERT_ENGINE_CODEC_BLOCK_BACKENDS_H_
