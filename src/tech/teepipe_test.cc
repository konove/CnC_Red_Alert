#include "tech/teepipe.h"

#include <array>
#include <span>

#include "gtest/gtest.h"
#include "tech/xpipe.h"

namespace {

TEST(TeePipeTest, CopiesAllInputAcrossWrites) {
  std::array<char, 6> main_bytes{};
  std::array<char, 6> copy_bytes{};
  BufferPipe main_sink(std::as_writable_bytes(std::span(main_bytes).first(6)));
  BufferPipe copy_sink(std::as_writable_bytes(std::span(copy_bytes).first(6)));
  TeePipe tee(main_sink, &copy_sink);
  EXPECT_EQ(tee.Put(std::as_bytes(std::span("FRAM", 4))), 4);
  EXPECT_EQ(tee.Put(std::as_bytes(std::span("\0\1", 2))), 2);
  EXPECT_TRUE(tee.copy_ok());
  EXPECT_EQ(main_bytes, copy_bytes);
  EXPECT_EQ(main_bytes, (std::array<char, 6>{'F', 'R', 'A', 'M', '\0', '\1'}));
}

TEST(TeePipeTest, CopyFailureDoesNotInterruptMainStream) {
  std::array<char, 6> main_bytes{};
  std::array<char, 2> copy_bytes{};
  BufferPipe main_sink(std::as_writable_bytes(std::span(main_bytes).first(6)));
  BufferPipe copy_sink(std::as_writable_bytes(std::span(copy_bytes).first(2)));
  TeePipe tee(main_sink, &copy_sink);
  EXPECT_EQ(tee.Put(std::as_bytes(std::span("FRAM", 4))), 4);
  EXPECT_FALSE(tee.copy_ok());
  EXPECT_EQ(tee.Put(std::as_bytes(std::span("12", 2))), 2);
  EXPECT_FALSE(tee.copy_ok());
  EXPECT_EQ(main_bytes, (std::array<char, 6>{'F', 'R', 'A', 'M', '1', '2'}));
}

TEST(TeePipeTest, DisabledCopyPreservesMainResult) {
  std::array<char, 2> main_bytes{};
  BufferPipe main_sink(std::as_writable_bytes(std::span(main_bytes).first(2)));
  TeePipe tee(main_sink, nullptr);
  EXPECT_EQ(tee.Put(std::as_bytes(std::span("FRAM", 4))), 2);
  EXPECT_TRUE(tee.copy_ok());
  EXPECT_EQ(main_bytes, (std::array<char, 2>{'F', 'R'}));
}

}  // namespace
