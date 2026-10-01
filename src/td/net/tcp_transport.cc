#include "td/net/tcp_transport.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/net/ipxaddr.h"
#include "td/net/tcpip.h"

namespace td {

namespace {

// The sender of every packet: TCP packets carry no IPX header.
constexpr engine::net::NetNumType kSenderNet = {1, 1, 1, 1};
constexpr engine::net::NetNodeType kSenderNode = {1, 1, 1, 1, 1, 1};

}  // namespace

bool TcpTransport::Start_Listening(uint16_t /*socket*/) {
  return tcp_.Get_Connected();
}

int TcpTransport::Read(std::span<std::byte> buffer,
                       engine::net::IPXAddressClass& from) {
  if (!tcp_.Get_Connected()) {
    return 0;
  }
  const int length = tcp_.Read(buffer, static_cast<int>(buffer.size()));
  if (length != 0) {
    from.Set_Address(kSenderNet, kSenderNode);
  }
  return length;
}

void TcpTransport::Write_To(std::span<const std::byte> packet,
                            const engine::net::IPXAddressClass& /*to*/) {
  Broadcast(packet);
}

void TcpTransport::Broadcast(std::span<const std::byte> packet) {
  if (tcp_.Get_Connected()) {
    tcp_.Write(packet, static_cast<int>(packet.size()));
  }
}

}  // namespace td
