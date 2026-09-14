#include <array>
#include <cstdint>
#include <span>

#include "gtest/gtest.h"
#include "td/savepipe.h"
#include "tech/archive.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

namespace {

TEST(TdSavePipeTest, ArchiveBodyRoundTripsWithoutSeeking) {
  std::array<char, 32> bytes{};
  BufferPipe sink(std::as_writable_bytes(std::span(bytes)));
  SaveGamePipe checked(sink);
  ArchiveWriter writer(checked);
  int32_t cell_count = 2;
  char raw_payload[] = "raw checkpoint";
  writer(cell_count);
  writer.Bytes(std::as_bytes(std::span(raw_payload)));
  EXPECT_TRUE(checked.ok());

  BufferStraw source(
      std::as_bytes(std::span(bytes).first(4 + sizeof(raw_payload))));
  ArchiveReader reader(source);
  int32_t loaded_count = 0;
  std::array<char, sizeof(raw_payload)> loaded{};
  reader(loaded_count);
  reader.Bytes(std::as_writable_bytes(std::span(loaded)));
  EXPECT_TRUE(reader.ok());
  EXPECT_EQ(loaded_count, cell_count);
  EXPECT_STREQ(loaded.data(), raw_payload);
  reader(loaded_count);
  EXPECT_FALSE(reader.ok());
}

TEST(TdSavePipeTest, ShortWriteIsSticky) {
  std::array<char, 2> bytes{};
  BufferPipe sink(std::as_writable_bytes(std::span(bytes)));
  SaveGamePipe checked(sink);
  ArchiveWriter writer(checked);
  int32_t value = 123;
  writer(value);
  EXPECT_FALSE(checked.ok());
  EXPECT_EQ(checked.Put(std::as_bytes(std::span("x", 1))), 0);
  EXPECT_FALSE(checked.ok());
}

}  // namespace
