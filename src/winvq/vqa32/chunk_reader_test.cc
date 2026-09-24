#include "winvq/vqa32/chunk_reader.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "gtest/gtest.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

TEST(ChunkReaderTest, ReadsIdAndBigEndianSize) {
  FakeVqaIo io;
  AppendChunk(io.data, "CPL0", std::vector<uint8_t>(0x123));
  ChunkReader reader(io);

  const auto chunk = reader.Next();
  ASSERT_TRUE(chunk.has_value());
  EXPECT_EQ(chunk->id, kChunkCpl0);
  EXPECT_EQ(chunk->size, 0x123);
  EXPECT_EQ(chunk->padded_size(), 0x124);
  EXPECT_EQ(io.pos, 8);
}

TEST(ChunkReaderTest, ReportsEndOfFileOnAShortHeader) {
  FakeVqaIo io;
  AppendBytes(io.data, "CPL0");
  ChunkReader reader(io);

  EXPECT_EQ(reader.Next().error(), ChunkError::kEndOfFile);
}

TEST(ChunkReaderTest, RejectsSizesOf2GiBAndMore) {
  for (const uint32_t size : {0x80000000U, 0xFFFFFFF8U, 0x7FFFFFFFU}) {
    FakeVqaIo io;
    AppendChunk(io.data, "XXXX", size, {});
    ChunkReader reader(io);

    EXPECT_EQ(reader.Next().error(), ChunkError::kBadSize) << size;
  }
}

TEST(ChunkReaderTest, SkipsThePayloadAndThePadByte) {
  FakeVqaIo io;
  AppendChunk(io.data, "ODD0", std::vector<uint8_t>(3));
  AppendChunk(io.data, "NEXT", std::vector<uint8_t>(2));
  ChunkReader reader(io);

  const auto odd = reader.Next();
  ASSERT_TRUE(odd.has_value());
  ASSERT_TRUE(reader.Skip(*odd));
  const auto next = reader.Next();
  ASSERT_TRUE(next.has_value());
  EXPECT_EQ(next->size, 2);
}

TEST(ChunkReaderTest, ReadsThePayloadWithItsPadByte) {
  FakeVqaIo io;
  AppendChunk(io.data, "ODD0", {1, 2, 3});
  ChunkReader reader(io);
  const auto chunk = reader.Next();
  ASSERT_TRUE(chunk.has_value());

  std::array<uint8_t, 4> dest{};
  ASSERT_TRUE(reader.ReadPayload(*chunk, std::span(dest)));
  EXPECT_EQ(dest, (std::array<uint8_t, 4>{1, 2, 3, 0}));
}

TEST(ChunkReaderTest, RefusesAPayloadLargerThanItsDestination) {
  FakeVqaIo io;
  AppendChunk(io.data, "ODD0", {1, 2, 3});
  ChunkReader reader(io);
  const auto chunk = reader.Next();
  ASSERT_TRUE(chunk.has_value());

  // Three bytes of payload fit, but not the pad byte.
  std::array<uint8_t, 3> dest{};
  EXPECT_FALSE(reader.ReadPayload(*chunk, std::span(dest)));
  EXPECT_EQ(io.pos, 8);
}

TEST(ChunkReaderTest, ReadsABareId) {
  FakeVqaIo io;
  AppendBytes(io.data, "WVQA");
  ChunkReader reader(io);

  EXPECT_EQ(reader.ReadId(), kFormWvqa);
  EXPECT_EQ(reader.ReadId(), std::nullopt);
}

}  // namespace
