// File: TcpTransport, the DatagramTransport Tiberian Dawn's IPX manager sends
// through.

#ifndef CNC_RED_ALERT_TD_NET_TCP_TRANSPORT_H_
#define CNC_RED_ALERT_TD_NET_TCP_TRANSPORT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "engine/net/datagram_transport.h"
#include "engine/net/ipxaddr.h"

namespace td {

class TcpipManagerClass;

// Carries every packet over the one connection `tcp` holds to the other
// player, whatever its address: there is no IPX driver, so this works only
// in an Internet game once `tcp` is connected, and does nothing before.
// Every packet read comes from the all-ones placeholder address.
class TcpTransport final : public engine::net::DatagramTransport {
 public:
  explicit TcpTransport(TcpipManagerClass& tcp ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : tcp_(tcp) {}

  // Succeeds only when the connection is up; opens nothing.
  bool Start_Listening(uint16_t socket) override;
  void Stop_Listening() override {}
  int Read(std::span<std::byte> buffer,
           engine::net::IPXAddressClass& from) override;
  void Write_To(std::span<const std::byte> packet,
                const engine::net::IPXAddressClass& to) override;
  void Broadcast(std::span<const std::byte> packet) override;

 private:
  TcpipManagerClass& tcp_;
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_TCP_TRANSPORT_H_
