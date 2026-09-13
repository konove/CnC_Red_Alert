// Round-trip tests for the LCW encoder, through the bounded decoder and
// through the LCW pipe and straw, including the map and overlay pack layout.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "tech/lcw.h"
#include "tech/lcwpipe.h"
#include "tech/lcwstraw.h"
#include "tech/pipe.h"
#include "tech/straw.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

namespace {

class ByteSink : public Pipe {
 public:
  std::vector<uint8_t> bytes;
  int Put(const void* data, int length) override {
    const auto* first = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), first, first + length);
    return length;
  }
};

std::vector<uint8_t> Drain(Straw& straw) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> chunk(999);
  for (int count = straw.Get(chunk.data(), static_cast<int>(chunk.size()));
       count != 0;
       count = straw.Get(chunk.data(), static_cast<int>(chunk.size()))) {
    result.insert(result.end(), chunk.begin(), chunk.begin() + count);
  }
  return result;
}

// Deterministic pseudo-random bytes; incompressible for LCW.
std::vector<uint8_t> Random(int size, uint32_t seed) {
  std::vector<uint8_t> bytes;
  for (int i = 0; i < size; ++i) {
    seed = (seed * 1664525U) + 1013904223U;
    bytes.push_back(static_cast<uint8_t>(seed >> 24));
  }
  return bytes;
}

// Runs of one value, lengths cycling from 1 to 300.
std::vector<uint8_t> Runs(int size) {
  std::vector<uint8_t> bytes;
  for (int run = 1; std::ssize(bytes) < size; run = (run % 300) + 1) {
    bytes.insert(bytes.end(), static_cast<std::size_t>(run),
                 static_cast<uint8_t>(run * 7));
  }
  bytes.resize(static_cast<std::size_t>(size));
  return bytes;
}

// A 3000-byte random pattern repeated with one byte changed per copy, so the
// encoder needs references at many distances.
std::vector<uint8_t> Repeats(int size) {
  const std::vector<uint8_t> pattern = Random(3000, 7);
  std::vector<uint8_t> bytes;
  for (int copy = 0; std::ssize(bytes) < size; ++copy) {
    bytes.insert(bytes.end(), pattern.begin(), pattern.end());
    bytes[bytes.size() - 1 - static_cast<std::size_t>(copy % 3000)] ^= 0x5a;
  }
  bytes.resize(static_cast<std::size_t>(size));
  return bytes;
}

// Compresses with LCW_Comp and checks the bound, then decodes with both
// decoders.
void ExpectRoundTrip(const std::vector<uint8_t>& plain) {
  const int size = static_cast<int>(plain.size());
  std::vector<uint8_t> packed(static_cast<std::size_t>(LcwWorstCaseSize(size)));
  const int packed_size = LCW_Comp(plain.data(), packed.data(), size);
  ASSERT_GT(packed_size, 0);
  ASSERT_LE(packed_size, LcwWorstCaseSize(size));

  std::vector<uint8_t> unpacked(plain.size());
  EXPECT_EQ(LcwUncompBounded(
                std::as_bytes(std::span(packed.data(),
                                        static_cast<std::size_t>(packed_size))),
                std::as_writable_bytes(std::span(unpacked))),
            size);
  EXPECT_EQ(unpacked, plain);

  std::vector<uint8_t> legacy(plain.size() + 1);
  EXPECT_EQ(LCW_Uncomp(packed.data(), legacy.data(), size), size);
  legacy.pop_back();
  EXPECT_EQ(legacy, plain);
}

TEST(LcwCompTest, EmptyInputIsJustTheEndMarker) {
  std::vector<uint8_t> packed(1);
  EXPECT_EQ(LCW_Comp(nullptr, packed.data(), 0), 1);
  EXPECT_EQ(packed[0], 0x80);
}

TEST(LcwCompTest, RoundTripsSingleByte) { ExpectRoundTrip({42}); }

TEST(LcwCompTest, RoundTripsRandomData) {
  ExpectRoundTrip(Random(8192, 1));
  ExpectRoundTrip(Random(1, 2));
  ExpectRoundTrip(Random(64, 3));
}

TEST(LcwCompTest, RoundTripsRuns) { ExpectRoundTrip(Runs(8192)); }

TEST(LcwCompTest, RoundTripsRepeats) { ExpectRoundTrip(Repeats(8192)); }

TEST(LcwCompTest, RoundTripsText) {
  constexpr std::string_view kText =
      "The quick brown fox jumps over the lazy dog. The quick brown fox jumps "
      "over the lazy dog again, and again, and again.";
  ExpectRoundTrip({kText.begin(), kText.end()});
}

TEST(LcwCompTest, RoundTripsMaximumSixteenBitBlock) {
  ExpectRoundTrip(Random(65535, 4));
  ExpectRoundTrip(Repeats(65535));
}

TEST(LcwCompTest, RoundTripsPastSixteenBitPositions) {
  // Absolute references cannot reach past 65535; relative ones still can.
  ExpectRoundTrip(Repeats(70000));
  std::vector<uint8_t> mixed = Random(66000, 5);
  const std::vector<uint8_t> tail = Runs(6000);
  mixed.insert(mixed.end(), tail.begin(), tail.end());
  ExpectRoundTrip(mixed);
}

TEST(LcwCompTest, CompressesRedundantData) {
  const std::vector<uint8_t> zeros(8192, 0);
  std::vector<uint8_t> packed(static_cast<std::size_t>(LcwWorstCaseSize(8192)));
  EXPECT_LE(LCW_Comp(zeros.data(), packed.data(), 8192), 8);
  ExpectRoundTrip(zeros);
}

// Compresses `plain` with the pipe and decompresses it with the straw, and the
// other way round, feeding the pipes in uneven pieces.
void ExpectPipeStrawRoundTrip(const std::vector<uint8_t>& plain,
                              int block_size) {
  ByteSink encoded;
  LCWPipe compressor(LCWPipe::COMPRESS, block_size);
  compressor.SetSink(encoded);
  for (std::size_t at = 0; at < plain.size(); at += 1234) {
    const std::size_t piece = std::min<std::size_t>(1234, plain.size() - at);
    compressor.Put(plain.data() + at, static_cast<int>(piece));
  }
  compressor.Flush();
  BufferStraw compressed(encoded.bytes.data(),
                         static_cast<int>(encoded.bytes.size()));
  LCWStraw decompressor(LCWStraw::DECOMPRESS, block_size);
  decompressor.SetSource(compressed);
  EXPECT_EQ(Drain(decompressor), plain);

  BufferStraw source(plain.data(), static_cast<int>(plain.size()));
  LCWStraw compressing_straw(LCWStraw::COMPRESS, block_size);
  compressing_straw.SetSource(source);
  const std::vector<uint8_t> straw_encoded = Drain(compressing_straw);
  EXPECT_EQ(straw_encoded, encoded.bytes);
  ByteSink decoded;
  LCWPipe decompressing_pipe(LCWPipe::DECOMPRESS, block_size);
  decompressing_pipe.SetSink(decoded);
  for (std::size_t at = 0; at < straw_encoded.size(); at += 777) {
    const std::size_t piece =
        std::min<std::size_t>(777, straw_encoded.size() - at);
    decompressing_pipe.Put(straw_encoded.data() + at, static_cast<int>(piece));
  }
  decompressing_pipe.Flush();
  EXPECT_EQ(decoded.bytes, plain);
}

TEST(LcwCompTest, PipeAndStrawRoundTripIncompressibleBlocks) {
  ExpectPipeStrawRoundTrip(Random(3 * 8192, 6), 8192);
  ExpectPipeStrawRoundTrip(Random(1000, 8), 128);
}

TEST(LcwCompTest, PipeAndStrawRoundTripMixedData) {
  std::vector<uint8_t> mixed = Runs(20000);
  const std::vector<uint8_t> repeats = Repeats(20000);
  mixed.insert(mixed.end(), repeats.begin(), repeats.end());
  ExpectPipeStrawRoundTrip(mixed, 8192);
}

// Mirrors MapClass::Write_Binary/Read_Binary and OverlayClass::Write_INI/
// Read_INI in ra: per-cell template types (uint16_t), then icons (uint8_t),
// then overlays (int8_t), each through a default LCW pipe into the 32000-byte
// staging buffer, and back through a default LCW straw.
TEST(LcwCompTest, MapAndOverlayPacksRoundTripThroughStagingBuffer) {
  constexpr int kCells = 128 * 128;  // MAP_CELL_TOTAL.
  const std::vector<uint8_t> noise = Random(kCells, 9);
  std::vector<uint16_t> types(kCells, 0xffff);  // TEMPLATE_NONE.
  std::vector<uint8_t> icons(kCells, 0);
  std::vector<int8_t> overlays(kCells, -1);  // OVERLAY_NONE.
  for (int cell = 0; cell < kCells; ++cell) {
    const auto index = static_cast<std::size_t>(cell);
    if ((cell / 128) % 16 < 5 && (cell % 128) % 20 < 9) {
      types[index] = static_cast<uint16_t>(noise[index] % 40);
      icons[index] = static_cast<uint8_t>(noise[index] % 16);
    }
    if (cell % 311 == 0) {
      overlays[index] = static_cast<int8_t>(noise[index] % 20);
    }
  }

  std::vector<char> staging(32000);
  BufferPipe map_sink(staging.data(), static_cast<int>(staging.size()));
  int map_total = 0;
  {
    LCWPipe comp(LCWPipe::COMPRESS);
    comp.SetSink(&map_sink);
    for (const uint16_t& type : types) {
      map_total += comp.Put(&type, sizeof(type));
    }
    for (const uint8_t& icon : icons) {
      map_total += comp.Put(&icon, sizeof(icon));
    }
    map_total += comp.Flush();
  }
  ASSERT_GT(map_total, 0);
  ASSERT_LT(map_total, 32000);  // Nothing was clipped by the staging buffer.

  BufferStraw map_source(staging.data(), map_total);
  LCWStraw decomp(LCWStraw::DECOMPRESS);
  decomp.SetSource(&map_source);
  std::vector<uint16_t> read_types(kCells);
  std::vector<uint8_t> read_icons(kCells);
  for (uint16_t& type : read_types) {
    ASSERT_EQ(decomp.Get(&type, sizeof(type)), sizeof(type));
  }
  for (uint8_t& icon : read_icons) {
    ASSERT_EQ(decomp.Get(&icon, sizeof(icon)), sizeof(icon));
  }
  EXPECT_EQ(read_types, types);
  EXPECT_EQ(read_icons, icons);

  BufferPipe overlay_sink(staging.data(), static_cast<int>(staging.size()));
  int overlay_total = 0;
  {
    LCWPipe comppipe(LCWPipe::COMPRESS);
    comppipe.SetSink(&overlay_sink);
    for (const int8_t& overlay : overlays) {
      overlay_total += comppipe.Put(&overlay, sizeof(overlay));
    }
    overlay_total += comppipe.Flush();
  }
  ASSERT_GT(overlay_total, 0);
  ASSERT_LT(overlay_total, 32000);

  BufferStraw overlay_source(staging.data(), overlay_total);
  LCWStraw uncomp(LCWStraw::DECOMPRESS);
  uncomp.SetSource(&overlay_source);
  std::vector<int8_t> read_overlays(kCells);
  for (int8_t& overlay : read_overlays) {
    ASSERT_EQ(uncomp.Get(&overlay, sizeof(overlay)), sizeof(overlay));
  }
  EXPECT_EQ(read_overlays, overlays);
}

}  // namespace
