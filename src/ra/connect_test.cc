// Tests for Red Alert's ConnectionClass sequencing and send-queue search.
#include "ra/connect.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/buffer.h"
#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "ra/combuf.h"

namespace {

constexpr uint16_t kMagic = 0x1234;

// A tick count past 2^32. ConnectionClass::Time() counts 60ths of a second
// from steady_clock, which on Linux starts at boot, so it reaches this after
// about 828 days of uptime.
constexpr int64_t kLongUptimeTicks = 8'589'934'592;  // 2^33

// A connection that counts sends instead of touching a network device.
class TestConnection : public ConnectionClass {
 public:
  TestConnection() : ConnectionClass(4, 8, 64, kMagic, 1, 3, 60) { Init(); }

  [[nodiscard]] int sent_count() const { return sent_count_; }

 protected:
  int Send(std::span<const std::byte> /*buf*/, int /*buflen*/,
           std::span<const std::byte> /*extrabuf*/, int /*extralen*/) override {
    ++sent_count_;
    return 1;
  }

 private:
  int sent_count_ = 0;
};

using Packet = std::array<char, sizeof(CommHeaderType) + 1>;

// Builds a received packet: a header followed by one payload byte.
Packet MakePacket(ConnectionClass::ConnectionEnum code, uint32_t id,
                  char payload) {
  CommHeaderType header{};
  header.MagicNumber = kMagic;
  header.Code = static_cast<unsigned char>(code);
  header.PacketID = id;
  Packet packet{};
  port::WriteUnaligned(base::ObjectBytes(packet), header);
  packet.back() = payload;
  return packet;
}

// Queues a header-only packet for sending and returns its queue entry.
SendQueueType* QueueHeader(CommBufferClass& queue ABSL_ATTRIBUTE_LIFETIME_BOUND,
                           ConnectionClass::ConnectionEnum code,
                           int64_t first_time) {
  CommHeaderType header{};
  header.MagicNumber = kMagic;
  header.Code = static_cast<unsigned char>(code);
  if (queue.Queue_Send(base::ObjectBytes(header),
                       static_cast<int>(sizeof(header))) == 0) {
    return nullptr;
  }
  SendQueueType* entry = queue.Get_Send(queue.Num_Send() - 1);
  entry->FirstTime = first_time;
  return entry;
}

TEST(ConnectionTest, DeliversAckRequiredPacketsInIdOrder) {
  TestConnection connection;
  Packet second = MakePacket(ConnectionClass::PACKET_DATA_ACK, 1, 'b');
  Packet first = MakePacket(ConnectionClass::PACKET_DATA_ACK, 0, 'a');
  EXPECT_TRUE(connection.Receive_Packet(base::ObjectBytes(second),
                                        static_cast<int>(second.size())));
  EXPECT_TRUE(connection.Receive_Packet(base::ObjectBytes(first),
                                        static_cast<int>(first.size())));
  EXPECT_EQ(connection.sent_count(), 2);

  char payload = 0;
  int length = 0;
  ASSERT_TRUE(connection.Get_Packet(base::ObjectBytes(payload), &length));
  EXPECT_EQ(length, 1);
  EXPECT_EQ(payload, 'a');
  ASSERT_TRUE(connection.Get_Packet(base::ObjectBytes(payload), &length));
  EXPECT_EQ(payload, 'b');
  EXPECT_FALSE(connection.Get_Packet(base::ObjectBytes(payload), &length));
}

TEST(ConnectionTest, OldestUnackedSendFindsPacketsAfterLongUptime) {
  CommBufferClass newer(4, 4, 64);
  CommBufferClass older(4, 4, 64);
  QueueHeader(newer, ConnectionClass::PACKET_DATA_ACK, kLongUptimeTicks + 50);
  SendQueueType* expected =
      QueueHeader(older, ConnectionClass::PACKET_DATA_ACK, kLongUptimeTicks);
  ASSERT_NE(expected, nullptr);

  std::array<CommBufferClass*, 2> queues{&newer, &older};
  EXPECT_EQ(ConnectionClass::OldestUnackedSend(queues), expected);
}

TEST(ConnectionTest, OldestUnackedSendSkipsAckedAndNoAckPackets) {
  CommBufferClass queue(4, 4, 64);
  QueueHeader(queue, ConnectionClass::PACKET_DATA_NOACK, 1);
  SendQueueType* acked =
      QueueHeader(queue, ConnectionClass::PACKET_DATA_ACK, 2);
  ASSERT_NE(acked, nullptr);
  acked->IsACK = 1;
  SendQueueType* expected =
      QueueHeader(queue, ConnectionClass::PACKET_DATA_ACK, kLongUptimeTicks);

  std::array<CommBufferClass*, 2> queues{nullptr, &queue};
  EXPECT_EQ(ConnectionClass::OldestUnackedSend(queues), expected);
}

TEST(ConnectionTest, OldestUnackedSendReturnsNullWhenNothingPending) {
  CommBufferClass empty(4, 4, 64);
  std::array<CommBufferClass*, 2> queues{nullptr, &empty};
  EXPECT_EQ(ConnectionClass::OldestUnackedSend(queues), nullptr);
}

TEST(ConnectionTest, RejectsLengthsBeyondTheSuppliedStorage) {
  TestConnection connection;
  Packet packet = MakePacket(ConnectionClass::PACKET_DATA_NOACK, 0, 'x');
  EXPECT_FALSE(connection.Receive_Packet(base::ObjectBytes(packet).first(1),
                                         static_cast<int>(packet.size())));
  EXPECT_FALSE(connection.Send_Packet(base::ObjectBytes(packet), -1, 0));
  EXPECT_FALSE(connection.Send_Packet(base::ObjectBytes(packet),
                                      static_cast<int>(packet.size()) + 1, 0));
  EXPECT_EQ(connection.Queue->Num_Receive(), 0);
  EXPECT_EQ(connection.Queue->Num_Send(), 0);
}

TEST(ConnectionTest, ShortDestinationDoesNotConsumeReceivedPacket) {
  TestConnection connection;
  Packet packet = MakePacket(ConnectionClass::PACKET_DATA_ACK, 0, 'x');
  ASSERT_TRUE(connection.Receive_Packet(base::ObjectBytes(packet),
                                        static_cast<int>(packet.size())));
  int length = 0;
  EXPECT_FALSE(connection.Get_Packet({}, &length));
  char payload = 0;
  ASSERT_TRUE(connection.Get_Packet(base::ObjectBytes(payload), &length));
  EXPECT_EQ(payload, 'x');
  EXPECT_EQ(length, 1);
}

TEST(CommBufferTest, RejectsInvalidInputAndQueueIndices) {
  CommBufferClass queue(2, 2, 8);
  const uint32_t value = 0x12345678;
  EXPECT_FALSE(queue.Queue_Send(base::ObjectBytes(value), -1));
  EXPECT_FALSE(queue.Queue_Send(base::ObjectBytes(value), 5));
  EXPECT_EQ(queue.Get_Send(-1), nullptr);
  EXPECT_EQ(queue.Get_Receive(0), nullptr);
  EXPECT_FALSE(queue.UnQueue_Send({}, nullptr, 2));
  EXPECT_EQ(queue.Num_Send(), 0);
}

TEST(CommBufferTest, ShortDestinationLeavesThePacketQueued) {
  CommBufferClass queue(2, 2, 8);
  const uint32_t value = 0x12345678;
  ASSERT_TRUE(queue.Queue_Send(base::ObjectBytes(value), sizeof(value)));
  uint16_t small = 0;
  int length = 0;
  EXPECT_FALSE(queue.UnQueue_Send(base::ObjectBytes(small), &length, 0));
  EXPECT_EQ(queue.Num_Send(), 1);
  uint32_t output = 0;
  ASSERT_TRUE(queue.UnQueue_Send(base::ObjectBytes(output), &length, 0));
  EXPECT_EQ(output, value);
  EXPECT_EQ(length, sizeof(value));
  EXPECT_EQ(queue.Num_Send(), 0);
}

}  // namespace
