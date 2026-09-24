#include "winvq/vqa32/lcw_buffer.h"

#include <cstdint>
#include <span>
#include <vector>

#include "base/array.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

// An LCW stream that decodes to "abc": a 3-byte literal run, then the end
// marker.
std::vector<uint8_t> CompressedAbc() { return {0x83, 'a', 'b', 'c', 0x80}; }

// Serves one chunk with the given payload and returns its header.
Chunk ServeChunk(FakeVqaIo& io, const std::vector<uint8_t>& payload) {
  AppendChunk(io.data, "DATA", payload);
  return ChunkReader(io).Next().value();
}

TEST(LcwBufferTest, LoadsRawDataAtTheStart) {
  FakeVqaIo io;
  const Chunk chunk = ServeChunk(io, {1, 2, 3});
  ChunkReader reader(io);
  LcwBuffer buffer(16);

  ASSERT_TRUE(buffer.LoadRaw(reader, chunk));
  EXPECT_FALSE(buffer.compressed());
  EXPECT_EQ(
      std::vector<uint8_t>(buffer.contents().begin(), buffer.contents().end()),
      (std::vector<uint8_t>{1, 2, 3}));
}

TEST(LcwBufferTest, LoadsCompressedDataAtTheEndAndDecompressesOnce) {
  FakeVqaIo io;
  const Chunk chunk = ServeChunk(io, CompressedAbc());
  ChunkReader reader(io);
  LcwBuffer buffer(16);

  ASSERT_TRUE(buffer.LoadCompressed(reader, chunk));
  EXPECT_TRUE(buffer.compressed());
  EXPECT_TRUE(buffer.contents().empty());
  EXPECT_EQ(buffer.size(), 5);
  // Five bytes and the pad byte, flush with the end.
  EXPECT_EQ(base::At(buffer.data(), 10), 0x83);

  buffer.Decompress();
  EXPECT_FALSE(buffer.compressed());
  EXPECT_EQ(
      std::vector<uint8_t>(buffer.contents().begin(), buffer.contents().end()),
      (std::vector<uint8_t>{'a', 'b', 'c'}));

  // Already decompressed: nothing changes.
  buffer.Decompress();
  EXPECT_EQ(buffer.size(), 3);
}

TEST(LcwBufferTest, RefusesAPayloadLargerThanTheBuffer) {
  FakeVqaIo io;
  const Chunk chunk = ServeChunk(io, std::vector<uint8_t>(17));
  ChunkReader reader(io);
  LcwBuffer buffer(16);

  EXPECT_FALSE(buffer.LoadRaw(reader, chunk));
  EXPECT_FALSE(buffer.LoadCompressed(reader, chunk));
  EXPECT_EQ(buffer.size(), 0);
}

TEST(LcwBufferTest, AssemblesPiecesAtOffsets) {
  FakeVqaIo io;
  AppendChunk(io.data, "PART", {0x83, 'a'});
  AppendChunk(io.data, "PART", {'b', 'c', 0x80});
  ChunkReader reader(io);
  LcwBuffer buffer(16);

  const Chunk first = reader.Next().value();
  ASSERT_TRUE(buffer.ReadAt(reader, first, 8));
  const Chunk second = reader.Next().value();
  // The second piece overwrites the first's (absent) pad byte.
  ASSERT_TRUE(buffer.ReadAt(reader, second, 8 + first.size));
  buffer.SetCompressed(8, first.size + second.size);

  buffer.Decompress();
  EXPECT_EQ(
      std::vector<uint8_t>(buffer.contents().begin(), buffer.contents().end()),
      (std::vector<uint8_t>{'a', 'b', 'c'}));
}

TEST(LcwBufferTest, RefusesAPieceRunningOffTheEnd) {
  FakeVqaIo io;
  const Chunk chunk = ServeChunk(io, std::vector<uint8_t>(4));
  ChunkReader reader(io);
  LcwBuffer buffer(16);

  EXPECT_FALSE(buffer.ReadAt(reader, chunk, 13));
  EXPECT_FALSE(buffer.ReadAt(reader, chunk, -1));
  EXPECT_TRUE(buffer.ReadAt(reader, chunk, 12));
}

}  // namespace
