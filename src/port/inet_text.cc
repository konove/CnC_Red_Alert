#include "port/inet_text.h"

#include <string>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace port {

std::string Ipv4Text(const in_addr& address) {
  char text[INET_ADDRSTRLEN] = {};
  // inet_ntop fails only for an unknown family or a short buffer, neither of
  // which can happen here; an empty string is the honest result if it did.
  if (inet_ntop(AF_INET, &address, text, sizeof(text)) == nullptr) {
    return {};
  }
  return {text};
}

}  // namespace port
