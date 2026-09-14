#include "tech/tee_sink.h"

#include <array>
#include <span>

#include "gtest/gtest.h"
#include "tech/span_sink.h"

namespace {

TEST(TeePipeTest, CopiesAllInputAcrossWrites) {
  std::array<char, 6> main_bytes{};
  std::array<char, 6> copy_bytes{};
  SpanSink main_sink(std::as_writable_bytes(std::span(main_bytes).first(6)));
  SpanSink copy_sink(std::as_writable_bytes(std::span(copy_bytes).first(6)));
  TeeSink tee(main_sink, &copy_sink);
  EXPECT_TRUE(tee.Write(std::as_bytes(std::span("FRAM", 4))));
  EXPECT_TRUE(tee.Write(std::as_bytes(std::span("\0\1", 2))));
  EXPECT_TRUE(tee.copy_ok());
  EXPECT_EQ(main_bytes, copy_bytes);
  EXPECT_EQ(main_bytes, (std::array<char, 6>{'F', 'R', 'A', 'M', '\0', '\1'}));
}

TEST(TeePipeTest, CopyFailureDoesNotInterruptMainStream) {
  std::array<char, 6> main_bytes{};
  std::array<char, 2> copy_bytes{};
  SpanSink main_sink(std::as_writable_bytes(std::span(main_bytes).first(6)));
  SpanSink copy_sink(std::as_writable_bytes(std::span(copy_bytes).first(2)));
  TeeSink tee(main_sink, &copy_sink);
  EXPECT_TRUE(tee.Write(std::as_bytes(std::span("FRAM", 4))));
  EXPECT_FALSE(tee.copy_ok());
  EXPECT_TRUE(tee.Write(std::as_bytes(std::span("12", 2))));
  EXPECT_FALSE(tee.copy_ok());
  EXPECT_EQ(main_bytes, (std::array<char, 6>{'F', 'R', 'A', 'M', '1', '2'}));
}

TEST(TeePipeTest, DisabledCopyPreservesMainResult) {
  std::array<char, 2> main_bytes{};
  SpanSink main_sink(std::as_writable_bytes(std::span(main_bytes).first(2)));
  TeeSink tee(main_sink, nullptr);
  EXPECT_FALSE(tee.Write(std::as_bytes(std::span("FRAM", 4))));
  EXPECT_FALSE(tee.ok());
  EXPECT_TRUE(tee.copy_ok());
  EXPECT_EQ(main_bytes, (std::array<char, 2>{'F', 'R'}));
}

}  // namespace
