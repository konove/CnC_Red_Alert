#include "engine/net/combuf.h"

#include <array>
#include <cstddef>

#include "gtest/gtest.h"

namespace engine::net {
namespace {

constexpr std::array<std::byte, 3> kPacket = {std::byte{1}, std::byte{2},
                                              std::byte{3}};
constexpr std::array<std::byte, 2> kExtra = {std::byte{9}, std::byte{8}};

TEST(CommBufferTest, SendQueueRoundTripsAPacket) {
  CommBufferClass buffer(2, 2, 8);
  ASSERT_EQ(buffer.Queue_Send(kPacket, 3), 1);
  EXPECT_EQ(buffer.Num_Send(), 1);

  std::array<std::byte, 8> out{};
  int len = 0;
  ASSERT_EQ(buffer.UnQueue_Send(out, &len, 0), 1);
  EXPECT_EQ(len, 3);
  EXPECT_EQ(out.at(2), std::byte{3});
  EXPECT_EQ(buffer.Num_Send(), 0);
}

TEST(CommBufferTest, FullOrOversizedSendIsRefused) {
  CommBufferClass buffer(1, 1, 2);
  EXPECT_EQ(buffer.Queue_Send(kPacket, 3), 0);  // longer than maxlen
  ASSERT_EQ(buffer.Queue_Send(kPacket, 2), 1);
  EXPECT_EQ(buffer.Queue_Send(kPacket, 2), 0);  // queue full
}

TEST(CommBufferTest, ExtraBytesTravelWithThePacket) {
  CommBufferClass buffer(1, 1, 8, 4);
  ASSERT_EQ(buffer.Queue_Receive(kPacket, 3, kExtra, 2), 1);

  std::array<std::byte, 8> out{};
  std::array<std::byte, 4> extra_out{};
  int len = 0;
  int extra_len = 0;
  ASSERT_EQ(buffer.UnQueue_Receive(out, &len, 0, extra_out, &extra_len), 1);
  EXPECT_EQ(len, 3);
  EXPECT_EQ(extra_len, 2);
  EXPECT_EQ(extra_out.at(1), std::byte{8});
}

TEST(CommBufferTest, ExtraBytesAreDroppedWithoutRoomForThem) {
  // Tiberian Dawn's buffers reserve no extra space.
  CommBufferClass buffer(1, 1, 8);
  ASSERT_EQ(buffer.Queue_Send(kPacket, 3, kExtra, 2), 1);
  EXPECT_EQ(buffer.Get_Send(0)->ExtraLen, 0);
}

}  // namespace
}  // namespace engine::net
