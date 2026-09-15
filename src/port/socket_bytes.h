// File: The byte-pointer view of an object that the socket API takes.
//
// Winsock declares the option and packet buffers of setsockopt, getsockopt,
// send and recv as char*, where POSIX has void*. A char* converts to both, so
// call sites that go through SocketBytes need no platform branch.
//
// Example:
//   int yes = 1;
//   setsockopt(sock, SOL_SOCKET, SO_BROADCAST, SocketBytes(yes), sizeof(yes));
//   recvfrom(sock, SocketBytes(receive_buffer), sizeof(receive_buffer), ...);
//   bind(sock, SocketAddress(addr), sizeof(addr));

#ifndef CNC_RED_ALERT_PORT_SOCKET_BYTES_H_
#define CNC_RED_ALERT_PORT_SOCKET_BYTES_H_

#include "absl/base/attributes.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

// Viewing an object's representation through char* is the one thing
// reinterpret_cast is defined for, and keeping it here keeps it out of the
// call sites. The NOLINTs cover only that check.

// Returns the bytes of `object`, an option value or a packet buffer array.
template <typename T>
char* SocketBytes(T& object ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<char*>(&object);
}

template <typename T>
const char* SocketBytes(const T& object ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const char*>(&object);
}

// Overloads for byte buffers held by pointer rather than by array, so the
// pointer is not itself taken as the object.
inline char* SocketBytes(unsigned char* bytes ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<char*>(bytes);
}

inline const char* SocketBytes(
    const unsigned char* bytes ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const char*>(bytes);
}

// Returns `address` as the generic sockaddr that bind, accept, recvfrom and
// sendto take. sockaddr_in is laid out to begin with sockaddr's family field,
// which is what lets the socket API read it through the generic type.
inline sockaddr* SocketAddress(
    sockaddr_in& address ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<sockaddr*>(&address);
}

inline const sockaddr* SocketAddress(
    const sockaddr_in& address ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const sockaddr*>(&address);
}

#endif  // CNC_RED_ALERT_PORT_SOCKET_BYTES_H_
