// Tests for the tagged-field packets sent to the Westwood statistics server.

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>

#include "gtest/gtest.h"
#include "tech/field.h"
#include "tech/packet.h"

namespace {

// Serializes `packet` and returns the wire bytes.
std::unique_ptr<char[]> Serialize(PacketClass& packet, int& size) {
  return std::unique_ptr<char[]>(packet.Create_Comms_Packet(size));
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
  std::unique_ptr<char[]> wire = Serialize(packet, size);
  PacketClass parsed(wire.get());

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
  ASSERT_TRUE(parsed.Get_Field("STRG", text.data(), text.size()));

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
  std::unique_ptr<char[]> wire = Serialize(packet, size);

  // Packet header (size, id), field header (id, type, size), then the data.
  // The original protocol sends TYPE_LONG as 4 bytes; an 8-byte host `long`
  // used to inflate the field to 20 bytes on LP64.
  constexpr std::array<unsigned char, 16> kExpected = {
      0x00, 0x10, 0x01, 0x02,                         // packet size, id
      'A',  'B',  'C',  'D',  0x00, TYPE_LONG, 0x00,  // field id, type
      0x04,                                           // field size
      0x01, 0x02, 0x03, 0x04};                        // value
  ASSERT_EQ(size, static_cast<int>(kExpected.size()));
  EXPECT_EQ(std::memcmp(wire.get(), kExpected.data(), kExpected.size()), 0);
}

}  // namespace
