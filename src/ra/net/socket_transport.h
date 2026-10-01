// File: SocketTransport, the DatagramTransport Red Alert's IPX manager sends
// through.

#ifndef CNC_RED_ALERT_RA_NET_SOCKET_TRANSPORT_H_
#define CNC_RED_ALERT_RA_NET_SOCKET_TRANSPORT_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "engine/net/datagram_transport.h"
#include "engine/net/ipxaddr.h"

namespace ra {

class WinsockInterfaceClass;

// Forwards to whichever WinsockInterfaceClass the protocol setup code has put
// in `winsock` (UDP today), looked up on every call because that code
// replaces it. With none there, nothing is read or sent and listening fails.
class SocketTransport final : public engine::net::DatagramTransport {
 public:
  explicit SocketTransport(std::unique_ptr<WinsockInterfaceClass>& winsock
                               ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : winsock_(winsock) {}

  bool Start_Listening(uint16_t socket) override;
  void Stop_Listening() override;
  int Read(std::span<std::byte> buffer,
           engine::net::IPXAddressClass& from) override;
  void Write_To(std::span<const std::byte> packet,
                const engine::net::IPXAddressClass& to) override;
  void Broadcast(std::span<const std::byte> packet) override;

 private:
  std::unique_ptr<WinsockInterfaceClass>& winsock_;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_SOCKET_TRANSPORT_H_
