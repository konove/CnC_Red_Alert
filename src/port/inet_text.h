// File: Dotted-decimal text for an IPv4 address.
//
// Replaces inet_ntoa, which formats into one static buffer, with inet_ntop
// into a string the caller owns.
//
// Example:
//   in_addr address{};
//   address.s_addr = user_ip;
//   pNetUtil->RequestPing(port::Ipv4Text(address).c_str(), 1000, &unused);

#ifndef CNC_RED_ALERT_PORT_INET_TEXT_H_
#define CNC_RED_ALERT_PORT_INET_TEXT_H_

#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace port {

// Returns `address` as "a.b.c.d".
[[nodiscard]] std::string Ipv4Text(const in_addr& address);

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_INET_TEXT_H_
