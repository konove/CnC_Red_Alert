#include "td/vector.h"
// Exercise native event formats at deliberately unaligned packet addresses.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
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
  int Send(std::span<const std::byte> /*buf*/, int /*buflen*/) override {
    return 1;
  }
};

TEST_F(QueueAlignmentTest, ReceivesPacketHeaderAtOddAddress) {
  TestConnection connection;
  alignas(CommHeaderType) std::array<uint8_t, sizeof(CommHeaderType) + 4>
      bytes{};
  CommHeaderType header{};
  header.MagicNumber = 0x1234;
  header.Code = static_cast<unsigned char>(ConnectionClass::PACKET_DATA_NOACK);
  header.PacketID = 7;
  port::WriteUnaligned(std::as_writable_bytes(std::span(bytes)).subspan(1),
                       header);
  bytes.at(1 + sizeof(header)) = 0x6b;
  EXPECT_TRUE(connection.Receive_Packet(
      std::as_writable_bytes(std::span(bytes)).subspan(1), sizeof(header) + 1));
  std::array<uint8_t, 4> payload{};
  int length = 0;
  EXPECT_TRUE(connection.Get_Packet(std::as_writable_bytes(std::span(payload)),
                                    &length));
  EXPECT_EQ(length, 1);
  EXPECT_EQ(payload.at(0), 0x6b);
  EXPECT_FALSE(connection.Receive_Packet(
      std::as_writable_bytes(std::span(bytes)).subspan(1), sizeof(header) - 1));
}

TEST_F(QueueAlignmentTest, ExtractsCompressedFrameAndPayloadFromOddAddress) {
  alignas(EventClass) std::array<uint8_t, 512> bytes{};
  const auto packet = std::as_writable_bytes(std::span(bytes)).subspan(1);
  EventClass frame;
  frame.Type = EventClass::FRAMEINFO;
  frame.Frame = 123;
  frame.ID = static_cast<unsigned>(HOUSE_GOOD);
  frame.MPlayerID = 7;
  frame.Data.FrameInfo.Delay = 4;
  const auto header_size =
      offsetof(EventClass, Data) + sizeof(frame.Data.FrameInfo);
  base::CopyBytes(packet, base::ObjectBytes(frame), header_size);
  const auto payload = packet.subspan(header_size);
  port::WriteUnaligned(payload, EventClass::RESPONSE_TIME);
  decltype(frame.Data.FrameInfo.Delay) const delay = 9;
  port::WriteUnaligned(payload.subspan(sizeof(EventClass::EventType)), delay);
  const int size = static_cast<int>(
      header_size + sizeof(EventClass::EventType) + sizeof(delay));
  EXPECT_EQ(Extract_Compressed_Events(packet, size), 2);
  ASSERT_EQ(DoList.Count(), 2);
  EXPECT_EQ(DoList.at(0).Type, EventClass::FRAMEINFO);
  EXPECT_EQ(DoList.at(1).Type, EventClass::RESPONSE_TIME);
  EXPECT_EQ(DoList.at(1).Frame, 123);
  EXPECT_EQ(DoList.at(1).ID, static_cast<unsigned>(HOUSE_GOOD));
  EXPECT_EQ(DoList.at(1).MPlayerID, 7);
  EXPECT_EQ(DoList.at(1).Data.FrameInfo.Delay, 9);
}

TEST_F(QueueAlignmentTest, ExtractsUncompressedEventWithoutMutatingPacket) {
  alignas(EventClass) std::array<uint8_t, sizeof(EventClass) + 1> bytes{};
  EventClass event;
  event.Type = EventClass::RESPONSE_TIME;
  event.Frame = 321;
  event.IsExecuted = true;
  event.Data.FrameInfo.Delay = 11;
  port::WriteUnaligned(std::as_writable_bytes(std::span(bytes)).subspan(1),
                       event);
  const auto before = bytes;
  EXPECT_EQ(
      Extract_Uncompressed_Events(
          std::as_writable_bytes(std::span(bytes)).subspan(1), sizeof(event)),
      1);
  ASSERT_EQ(DoList.Count(), 1);
  EXPECT_EQ(DoList.at(0).Frame, 321);
  EXPECT_EQ(DoList.at(0).Data.FrameInfo.Delay, 11);
  EXPECT_FALSE(DoList.at(0).IsExecuted);
  EXPECT_EQ(bytes, before);
}

TEST_F(QueueAlignmentTest, RejectsTruncatedCompressedType) {
  std::array<uint8_t, 3> bytes{};
  EXPECT_EQ(
      Extract_Compressed_Events(std::as_bytes(std::span(bytes)), bytes.size()),
      0);
  EXPECT_EQ(DoList.Count(), 0);
}
TEST_F(QueueAlignmentTest, RejectsClaimedExtentBeyondStorage) {
  std::array<std::byte, sizeof(EventClass)> bytes{};
  EXPECT_EQ(Extract_Uncompressed_Events(bytes, sizeof(EventClass) + 1), 0);
  EXPECT_EQ(Extract_Compressed_Events(bytes, -1), 0);
  EXPECT_EQ(DoList.Count(), 0);
}

TEST_F(QueueAlignmentTest, RejectsTruncatedMissionRun) {
  std::array<std::byte, 256> bytes{};
  EventClass frame;
  frame.Type = EventClass::FRAMEINFO;
  const auto header_size =
      offsetof(EventClass, Data) + sizeof(frame.Data.FrameInfo);
  base::CopyBytes(bytes, base::ObjectBytes(frame), header_size);
  const auto mission = std::span(bytes).subspan(header_size);
  port::WriteUnaligned(mission, EventClass::MEGAMISSION);
  base::At(mission, sizeof(EventClass::EventType)) = std::byte{2};
  const int size =
      static_cast<int>(header_size + sizeof(EventClass::EventType) + 1 +
                       sizeof(frame.Data.MegaMission));
  EXPECT_EQ(Extract_Compressed_Events(bytes, size), 1);
  EXPECT_EQ(DoList.Count(), 1);
}

}  // namespace

namespace {

TEST(TdCheckedContainerTest, VectorAccessPreservesConstnessAndCapacity) {
  VectorClass<int> values(2);
  values.at(1) = 42;
  const VectorClass<int>& view = values;
  EXPECT_EQ(view.at(1), 42);
  EXPECT_EQ(&view.at(1), &values.at(1));
}

TEST(TdCheckedContainerTest, QueueAccessFollowsWrappedLogicalOrder) {
  QueueClass<int, 3> queue;
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  ASSERT_TRUE(queue.Add(30));
  ASSERT_TRUE(queue.Add(40));
  EXPECT_EQ(queue.at(0), 20);
  EXPECT_EQ(queue.at(1), 30);
  EXPECT_EQ(queue.at(2), 40);
  queue.at(1) = 31;
  EXPECT_EQ(queue.at(1), 31);
}

TEST(TdCheckedContainerDeathTest, RejectsInvalidVectorAndQueueIndices) {
  VectorClass<int> values(2);
  const VectorClass<int>& view = values;
  QueueClass<int, 3> queue;
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)values.at(-1), "Check failed");
  EXPECT_DEATH((void)view.at(2), "Check failed");
  EXPECT_DEATH((void)queue.at(0), "Check failed");
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  // Both indices formerly wrapped to valid storage outside the active queue.
  EXPECT_DEATH((void)queue.at(-1), "Check failed");
  EXPECT_DEATH((void)queue.at(1), "Check failed");
  EXPECT_DEATH((void)queue.at(4), "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

}  // namespace
