#include "port/socket_bytes.h"

#include <cstring>
#include <type_traits>

#include "gtest/gtest.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace {

TEST(SocketBytes, ScalarAddressesTheObject) {
  int value = 0x01020304;
  char* bytes = SocketBytes(value);
  EXPECT_EQ(static_cast<void*>(bytes), static_cast<void*>(&value));

  int copy = 0;
  std::memcpy(&copy, bytes, sizeof(copy));
  EXPECT_EQ(copy, value);
}

TEST(SocketBytes, ConstScalarGivesConstBytes) {
  const int value = 7;
  static_assert(std::is_same_v<decltype(SocketBytes(value)), const char*>);
  EXPECT_EQ(static_cast<const void*>(SocketBytes(value)),
            static_cast<const void*>(&value));
}

TEST(SocketBytes, ArrayAddressesFirstElement) {
  unsigned char buffer[16] = {0xAB};
  EXPECT_EQ(static_cast<void*>(SocketBytes(buffer)),
            static_cast<void*>(&buffer[0]));
  EXPECT_EQ(static_cast<unsigned char>(*SocketBytes(buffer)), 0xAB);
}

TEST(SocketBytes, PointerAddressesPointeeNotPointer) {
  unsigned char buffer[4] = {1, 2, 3, 4};
  unsigned char* pointer = buffer;
  EXPECT_EQ(static_cast<void*>(SocketBytes(pointer)),
            static_cast<void*>(buffer));

  const unsigned char* const_pointer = buffer;
  static_assert(
      std::is_same_v<decltype(SocketBytes(const_pointer)), const char*>);
  EXPECT_EQ(static_cast<const void*>(SocketBytes(const_pointer)),
            static_cast<const void*>(buffer));
}

TEST(SocketAddress, AddressesTheObjectAsSockaddr) {
  sockaddr_in address{};
  address.sin_family = AF_INET;
  sockaddr* generic = SocketAddress(address);
  EXPECT_EQ(static_cast<void*>(generic), static_cast<void*>(&address));
  EXPECT_EQ(generic->sa_family, AF_INET);

  const sockaddr_in& const_address = address;
  static_assert(
      std::is_same_v<decltype(SocketAddress(const_address)), const sockaddr*>);
  EXPECT_EQ(static_cast<const void*>(SocketAddress(const_address)),
            static_cast<const void*>(&address));
}

}  // namespace
