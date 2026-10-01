#include "engine/net/nullconn.h"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "base/unaligned.h"
#include "engine/net/connect.h"
#include "gtest/gtest.h"

namespace engine::net {
namespace {

std::vector<std::vector<std::byte>>& Diverted() {
  static std::vector<std::vector<std::byte>> diverted;
  return diverted;
}

bool Capture(std::span<const std::byte> packet) {
  Diverted().emplace_back(packet.begin(), packet.end());
  return true;
}

TEST(NullModemConnTest, FramesAPacketForTheLine) {
  Diverted().clear();
  NullModemConnClass connection(4, 4, 32, 0x1234, &Capture);
  int port = 0;  // any non-null handle; the diverter takes every packet
  connection.Init(&port);

  constexpr std::array<std::byte, 2> kPayload = {std::byte{7}, std::byte{9}};
  ASSERT_EQ(connection.Send_Packet(kPayload, 2, 0), 1);
  connection.Service();

  ASSERT_EQ(Diverted().size(), 1U);
  const std::vector<std::byte>& frame = Diverted().front();
  const auto header = base::ReadUnaligned<SerialHeaderType>(frame);
  EXPECT_EQ(header.MagicNumber, PACKET_SERIAL_START);
  EXPECT_EQ(header.MagicNumber2, PACKET_SERIAL_VERIFY);
  const int length = header.Length;
  EXPECT_EQ(length, static_cast<int>(sizeof(CommHeaderType)) + 2);
  ASSERT_EQ(frame.size(), sizeof(SerialHeaderType) +
                              static_cast<std::size_t>(length) + sizeof(int) +
                              1);

  const std::span<const std::byte> body = std::span(frame).subspan(
      sizeof(SerialHeaderType), static_cast<std::size_t>(length));
  const auto crc = base::ReadUnaligned<int>(
      std::span(frame).subspan(sizeof(SerialHeaderType) + body.size()));
  EXPECT_EQ(crc, NullModemConnClass::Compute_CRC(body, length));
  EXPECT_EQ(frame.back(), std::byte{'\r'});
}

TEST(NullModemConnTest, SendsNothingBeforeInit) {
  Diverted().clear();
  NullModemConnClass connection(4, 4, 32, 0x1234, &Capture);
  constexpr std::array<std::byte, 1> kPayload = {std::byte{1}};
  connection.Send_Packet(kPayload, 1, 0);
  connection.Service();
  EXPECT_TRUE(Diverted().empty());
}

TEST(NullModemConnTest, CrcRotatesTheHighBitIn) {
  constexpr std::array<std::byte, 2> kBytes = {std::byte{1}, std::byte{2}};
  // ((0 << 1) + 1) << 1) + 2
  EXPECT_EQ(NullModemConnClass::Compute_CRC(kBytes, 2), 4);
}

}  // namespace
}  // namespace engine::net
