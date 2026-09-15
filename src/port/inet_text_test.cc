#include "port/inet_text.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include "gtest/gtest.h"

namespace {

TEST(InetTextTest, FormatsDottedDecimal) {
  in_addr address{};
  address.s_addr = htonl(0x7f000001);
  EXPECT_EQ(port::Ipv4Text(address), "127.0.0.1");
  address.s_addr = htonl(0xc0a80a0b);
  EXPECT_EQ(port::Ipv4Text(address), "192.168.10.11");
}

}  // namespace
