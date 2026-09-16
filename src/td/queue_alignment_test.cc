// Exercise native event formats at deliberately unaligned packet addresses.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "td/connect.h"
#include "td/defines.h"
#include "td/event.h"
#include "td/noseqcon.h"
#include "td/queue.h"

extern QueueClass<EventClass, MAX_EVENTS * 8> DoList;

namespace {
class QueueAlignmentTest : public testing::Test {
 protected:
  void SetUp() override { DoList.Init(); }
  void TearDown() override { DoList.Init(); }
};

// Queue-only connection: these tests do not send through a network device.
class TestConnection : public NonSequencedConnClass {
 public:
  TestConnection() : NonSequencedConnClass(4, 4, 64, 0x1234, 1, 3, 60) {
    Init();
  }

 protected:
  int Send(void* /*buf*/, int /*buflen*/) override { return 1; }
};

TEST_F(QueueAlignmentTest, ReceivesPacketHeaderAtOddAddress) {
  TestConnection connection;
  alignas(CommHeaderType) std::array<uint8_t, sizeof(CommHeaderType) + 4>
      bytes{};
  CommHeaderType header{};
  header.MagicNumber = 0x1234;
  header.Code = static_cast<unsigned char>(ConnectionClass::PACKET_DATA_NOACK);
  header.PacketID = 7;
  port::WriteUnaligned(bytes.data() + 1, header);
  bytes[1 + sizeof(header)] = 0x6b;
  EXPECT_TRUE(connection.Receive_Packet(bytes.data() + 1, sizeof(header) + 1));
  std::array<uint8_t, 4> payload{};
  int length = 0;
  EXPECT_TRUE(connection.Get_Packet(payload.data(), &length));
  EXPECT_EQ(length, 1);
  EXPECT_EQ(payload[0], 0x6b);
  EXPECT_FALSE(connection.Receive_Packet(bytes.data() + 1, sizeof(header) - 1));
}

TEST_F(QueueAlignmentTest, ExtractsCompressedFrameAndPayloadFromOddAddress) {
  alignas(EventClass) std::array<uint8_t, 512> bytes{};
  auto* packet = bytes.data() + 1;
  EventClass frame;
  frame.Type = EventClass::FRAMEINFO;
  frame.Frame = 123;
  frame.ID = static_cast<unsigned>(HOUSE_GOOD);
  frame.MPlayerID = 7;
  frame.Data.FrameInfo.Delay = 4;
  const auto header_size =
      offsetof(EventClass, Data) + sizeof(frame.Data.FrameInfo);
  std::memcpy(packet, &frame, header_size);
  auto* payload = packet + header_size;
  port::WriteUnaligned(payload, EventClass::RESPONSE_TIME);
  decltype(frame.Data.FrameInfo.Delay) const delay = 9;
  port::WriteUnaligned(payload + sizeof(EventClass::EventType), delay);
  const int size = static_cast<int>(
      header_size + sizeof(EventClass::EventType) + sizeof(delay));
  EXPECT_EQ(Extract_Compressed_Events(packet, size), 2);
  ASSERT_EQ(DoList.Count(), 2);
  EXPECT_EQ(DoList[0].Type, EventClass::FRAMEINFO);
  EXPECT_EQ(DoList[1].Type, EventClass::RESPONSE_TIME);
  EXPECT_EQ(DoList[1].Frame, 123);
  EXPECT_EQ(DoList[1].ID, static_cast<unsigned>(HOUSE_GOOD));
  EXPECT_EQ(DoList[1].MPlayerID, 7);
  EXPECT_EQ(DoList[1].Data.FrameInfo.Delay, 9);
}

TEST_F(QueueAlignmentTest, ExtractsUncompressedEventWithoutMutatingPacket) {
  alignas(EventClass) std::array<uint8_t, sizeof(EventClass) + 1> bytes{};
  EventClass event;
  event.Type = EventClass::RESPONSE_TIME;
  event.Frame = 321;
  event.IsExecuted = true;
  event.Data.FrameInfo.Delay = 11;
  port::WriteUnaligned(bytes.data() + 1, event);
  const auto before = bytes;
  EXPECT_EQ(Extract_Uncompressed_Events(bytes.data() + 1, sizeof(event)), 1);
  ASSERT_EQ(DoList.Count(), 1);
  EXPECT_EQ(DoList[0].Frame, 321);
  EXPECT_EQ(DoList[0].Data.FrameInfo.Delay, 11);
  EXPECT_FALSE(DoList[0].IsExecuted);
  EXPECT_EQ(bytes, before);
}

TEST_F(QueueAlignmentTest, RejectsTruncatedCompressedType) {
  std::array<uint8_t, 3> bytes{};
  EXPECT_EQ(Extract_Compressed_Events(bytes.data(), bytes.size()), 0);
  EXPECT_EQ(DoList.Count(), 0);
}
}  // namespace
