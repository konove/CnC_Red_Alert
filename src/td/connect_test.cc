// Tests for Tiberian Dawn's non-sequenced connection and send-queue search.
#include "td/connect.h"

#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "td/combuf.h"
#include "td/noseqcon.h"

namespace {

constexpr uint16_t kMagic = 0x1234;

// A tick count past 2^32. ConnectionClass::Time() counts 60ths of a second
// from steady_clock, which on Linux starts at boot, so it reaches this after
// about 828 days of uptime.
constexpr int64_t kLongUptimeTicks = int64_t{1} << 33;

// A connection that counts sends instead of touching a network device.
class TestConnection : public NonSequencedConnClass {
 public:
  TestConnection() : NonSequencedConnClass(4, 8, 64, kMagic, 1, 3, 60) {
    Init();
  }

  [[nodiscard]] int sent_count() const { return sent_count_; }

 protected:
  int Send(void* /*buf*/, int /*buflen*/) override {
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
  port::WriteUnaligned(packet.data(), header);
  packet.back() = payload;
  return packet;
}

// Queues a header-only packet for sending and returns its queue entry.
SendQueueType* QueueHeader(CommBufferClass& queue,
                           ConnectionClass::ConnectionEnum code,
                           int64_t first_time) {
  CommHeaderType header{};
  header.MagicNumber = kMagic;
  header.Code = static_cast<unsigned char>(code);
  if (queue.Queue_Send(&header, static_cast<int>(sizeof(header))) == 0) {
    return nullptr;
  }
  SendQueueType* entry = queue.Get_Send(queue.Num_Send() - 1);
  entry->FirstTime = first_time;
  return entry;
}

// Packet IDs are 32 bits on the wire, as in the original game and in Red
// Alert, so the "no packet yet" sentinel 0xffffffff wraps to ID 0.
TEST(NonSequencedConnTest, PacketIdIs32Bits) {
  EXPECT_EQ(sizeof(CommHeaderType{}.PacketID), sizeof(uint32_t));
}

TEST(NonSequencedConnTest, DeliversAckRequiredPacketsInIdOrder) {
  TestConnection connection;
  Packet second = MakePacket(ConnectionClass::PACKET_DATA_ACK, 1, 'b');
  Packet first = MakePacket(ConnectionClass::PACKET_DATA_ACK, 0, 'a');
  EXPECT_TRUE(connection.Receive_Packet(second.data(),
                                        static_cast<int>(second.size())));
  EXPECT_TRUE(
      connection.Receive_Packet(first.data(), static_cast<int>(first.size())));
  EXPECT_EQ(connection.sent_count(), 2);

  char payload = 0;
  int length = 0;
  ASSERT_TRUE(connection.Get_Packet(&payload, &length));
  EXPECT_EQ(length, 1);
  EXPECT_EQ(payload, 'a');
  ASSERT_TRUE(connection.Get_Packet(&payload, &length));
  EXPECT_EQ(payload, 'b');
  EXPECT_FALSE(connection.Get_Packet(&payload, &length));
}

TEST(NonSequencedConnTest, DropsResendOfDeliveredPacket) {
  TestConnection connection;
  Packet packet = MakePacket(ConnectionClass::PACKET_DATA_ACK, 0, 'a');
  EXPECT_TRUE(
      connection.Receive_Packet(packet.data(), static_cast<int>(packet.size())));

  char payload = 0;
  int length = 0;
  ASSERT_TRUE(connection.Get_Packet(&payload, &length));
  EXPECT_EQ(payload, 'a');

  // The resend is still ACKed, but must not be delivered a second time.
  EXPECT_TRUE(
      connection.Receive_Packet(packet.data(), static_cast<int>(packet.size())));
  EXPECT_EQ(connection.sent_count(), 2);
  EXPECT_FALSE(connection.Get_Packet(&payload, &length));
}

TEST(NonSequencedConnTest, OldestUnackedSendFindsPacketsAfterLongUptime) {
  CommBufferClass newer(4, 4, 64);
  CommBufferClass older(4, 4, 64);
  QueueHeader(newer, ConnectionClass::PACKET_DATA_ACK, kLongUptimeTicks + 50);
  SendQueueType* expected =
      QueueHeader(older, ConnectionClass::PACKET_DATA_ACK, kLongUptimeTicks);
  ASSERT_NE(expected, nullptr);

  std::array<CommBufferClass*, 2> queues{&newer, &older};
  EXPECT_EQ(ConnectionClass::OldestUnackedSend(queues), expected);
}

TEST(NonSequencedConnTest, OldestUnackedSendSkipsAckedAndNoAckPackets) {
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

TEST(NonSequencedConnTest, OldestUnackedSendReturnsNullWhenNothingPending) {
  CommBufferClass empty(4, 4, 64);
  std::array<CommBufferClass*, 2> queues{nullptr, &empty};
  EXPECT_EQ(ConnectionClass::OldestUnackedSend(queues), nullptr);
}

}  // namespace
