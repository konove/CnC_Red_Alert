// Tests for the File interface helpers, using MemoryFile as the concrete file.


#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "base/seek_origin.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"
#include "tech/memory_file.h"

namespace {

struct Header {
  int32_t magic;
  int16_t version;
  int16_t flags;
};

TEST(FileTest, ObjectsRoundTripThroughWriteObjectAndReadObject) {
  char storage[64] = {};
  MemoryFile file(storage, sizeof(storage));
  ASSERT_TRUE(file.Open(FileAccess::kReadWrite));

  const Header written{.magic = 0x41424344, .version = 3, .flags = 7};
  EXPECT_TRUE(file.WriteObject(written));
  EXPECT_TRUE(file.WriteObject(int32_t{-1}));
  EXPECT_EQ(file.Seek(0, SeekOrigin::kBegin), 0);

  Header read{};
  int32_t trailer = 0;
  EXPECT_TRUE(file.ReadObject(read));
  EXPECT_TRUE(file.ReadObject(trailer));
  EXPECT_EQ(read.magic, written.magic);
  EXPECT_EQ(read.version, written.version);
  EXPECT_EQ(read.flags, written.flags);
  EXPECT_EQ(trailer, -1);
}

TEST(FileTest, ReadObjectFailsOnShortRead) {
  char storage[2] = {'a', 'b'};
  MemoryFile file(storage, sizeof(storage));
  int32_t value = 0;
  EXPECT_FALSE(file.ReadObject(value));
}

TEST(FileTest, ArraysAreObjectsToo) {
  char storage[8] = {};
  MemoryFile file(storage, sizeof(storage));
  ASSERT_TRUE(file.Open(FileAccess::kReadWrite));
  const int16_t written[3] = {1, 2, 3};
  EXPECT_TRUE(file.WriteObject(written));
  file.Seek(0, SeekOrigin::kBegin);
  int16_t read[3] = {};
  EXPECT_TRUE(file.ReadObject(read));
  EXPECT_EQ(read[2], 3);
}

TEST(FileTest, ReadBytesAndReadStringStopAtEndOfFile) {
  char storage[] = {'h', 'e', 'l', 'l', 'o'};
  MemoryFile file(storage, sizeof(storage));
  ASSERT_TRUE(file.Open());
  EXPECT_EQ(file.ReadString(2), "he");
  const std::vector<std::byte> rest = file.ReadBytes(10);
  ASSERT_EQ(rest.size(), 3U);
  EXPECT_EQ(static_cast<char>(rest[0]), 'l');
  EXPECT_TRUE(file.ReadBytes(4).empty());
}

TEST(FileTest, SpanAndRawPointerReadsAgree) {
  char storage[] = {'x', 'y', 'z'};
  MemoryFile file(storage, sizeof(storage));
  ASSERT_TRUE(file.Open());
  std::byte first[2];
  EXPECT_EQ(file.Read(std::span(first)), 2);
  EXPECT_EQ(static_cast<char>(first[1]), 'y');
  char last = 0;
  EXPECT_EQ(file.Read(&last, 1), 1);
  EXPECT_EQ(last, 'z');
}

}  // namespace
