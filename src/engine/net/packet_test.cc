// Tests for the tagged-field packets sent to the Westwood statistics server.

#include "engine/net/packet.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "engine/net/field.h"
#include "gtest/gtest.h"

namespace {

// Serializes `packet` and returns the wire bytes.
std::vector<char> Serialize(PacketClass& packet, int& size) {
  std::vector<char> wire = packet.Create_Comms_Packet();
  size = static_cast<int>(wire.size());
  return wire;
}

TEST(PacketClassTest, IntegerFieldsRoundTrip) {
  PacketClass packet(7);
  packet.Add_Field("CHAR", static_cast<char>(-3));
  packet.Add_Field("UCHR", static_cast<unsigned char>(200));
  packet.Add_Field("SHRT", static_cast<int16_t>(-12345));
  packet.Add_Field("USHT", static_cast<uint16_t>(54321));
  packet.Add_Field("LONG", int32_t{-123456789});
  packet.Add_Field("ULNG", uint32_t{0xDEADBEEF});
  packet.Add_Field("STRG", "hello");

  int size = 0;
  const std::vector<char> wire = Serialize(packet, size);
  const auto bytes = std::as_bytes(std::span(wire));
  PacketClass parsed(bytes);

  char c = 0;
  unsigned char uc = 0;
  int16_t s = 0;
  uint16_t us = 0;
  int32_t l = 0;
  uint32_t ul = 0;
  std::array<char, 16> text{};
  ASSERT_TRUE(parsed.Get_Field("CHAR", c));
  ASSERT_TRUE(parsed.Get_Field("UCHR", uc));
  ASSERT_TRUE(parsed.Get_Field("SHRT", s));
  ASSERT_TRUE(parsed.Get_Field("USHT", us));
  ASSERT_TRUE(parsed.Get_Field("LONG", l));
  ASSERT_TRUE(parsed.Get_Field("ULNG", ul));
  ASSERT_TRUE(parsed.Get_Field("STRG", text));

  EXPECT_EQ(c, -3);
  EXPECT_EQ(uc, 200);
  EXPECT_EQ(s, -12345);
  EXPECT_EQ(us, 54321);
  EXPECT_EQ(l, -123456789);
  EXPECT_EQ(ul, 0xDEADBEEF);
  EXPECT_STREQ(text.data(), "hello");
}

TEST(PacketClassTest, LongFieldIsFourBigEndianBytes) {
  PacketClass packet(0x0102);
  packet.Add_Field("ABCD", int32_t{0x01020304});

  int size = 0;
  const std::vector<char> wire = Serialize(packet, size);

  // Packet header (size, id), field header (id, type, size), then the data.
  // The original protocol sends TYPE_LONG as 4 bytes; an 8-byte host `long`
  // used to inflate the field to 20 bytes on LP64.
  constexpr std::array<unsigned char, 16> kExpected = {
      0x00, 0x10, 0x01, 0x02,                         // packet size, id
      'A',  'B',  'C',  'D',  0x00, TYPE_LONG, 0x00,  // field id, type
      0x04,                                           // field size
      0x01, 0x02, 0x03, 0x04};                        // value
  ASSERT_EQ(size, static_cast<int>(kExpected.size()));
  const auto bytes = std::as_bytes(std::span(wire));
  EXPECT_EQ(base::CompareBytes(bytes, base::ObjectBytes(kExpected), size), 0);
}

// The statistics server reads an array of counts as 32-bit big-endian values
// in one chunk field.
TEST(PacketClassTest, IntegerArrayFieldIsABigEndianChunk) {
  PacketClass packet(0x0102);
  constexpr std::array<int32_t, 2> kCounts = {0, 258};
  packet.Add_Field("UNB1", std::span<const int32_t>(kCounts));

  int size = 0;
  const std::vector<char> wire = Serialize(packet, size);

  constexpr std::array<unsigned char, 20> kExpected = {
      0x00, 0x14, 0x01, 0x02,                                 // packet size, id
      'U',  'N',  'B',  '1',  0x00, TYPE_CHUNK, 0x00,         // field id, type
      0x08,                                                   // field size
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       0x01, 0x02};  // counts
  ASSERT_EQ(size, static_cast<int>(kExpected.size()));
  const auto bytes = std::as_bytes(std::span(wire));
  EXPECT_EQ(base::CompareBytes(bytes, base::ObjectBytes(kExpected), size), 0);
}

TEST(PacketClassTest, RejectsCharacterFieldWithoutAValue) {
  constexpr std::array<unsigned char, 12> wire = {
      0, 12, 0, 1, 'C', 'H', 'A', 'R', 0, TYPE_CHAR, 0, 0};
  PacketClass parsed(std::as_bytes(std::span(wire)));
  char value = 'x';
  EXPECT_FALSE(parsed.Get_Field("CHAR", value));
  EXPECT_EQ(value, 'x');
}

}  // namespace
