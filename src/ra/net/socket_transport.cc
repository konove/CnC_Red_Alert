#include "ra/net/socket_transport.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "base/unaligned.h"
#include "engine/net/ipxaddr.h"
#include "ra/net/wsproto.h"

namespace ra {

bool SocketTransport::Start_Listening(const uint16_t socket) {
  if (!winsock_ || !winsock_->Open_Socket(socket)) {
    return false;
  }
  if (winsock_->Start_Listening()) {
    return true;
  }
  winsock_->Close_Socket();
  return false;
}

void SocketTransport::Stop_Listening() {
  if (winsock_) {
    winsock_->Stop_Listening();
  }
}

int SocketTransport::Read(std::span<std::byte> buffer,
                          engine::net::IPXAddressClass& from) {
  if (!winsock_) {
    return 0;
  }
  // The interface writes the sender as the bytes of an IPXAddressClass.
  char address[128];
  int buffer_len = static_cast<int>(buffer.size());
  int address_len = sizeof(address);
  const int length = winsock_->Read(buffer, buffer_len,
                                    base::ObjectBytes(address), address_len);
  if (length != 0) {
    from = base::ReadUnaligned<engine::net::IPXAddressClass>(
        base::ObjectBytes(address));
  }
  return length;
}

void SocketTransport::Write_To(std::span<const std::byte> packet,
                               const engine::net::IPXAddressClass& to) {
  if (winsock_) {
    winsock_->WriteTo(packet, static_cast<int>(packet.size()), to);
  }
}

void SocketTransport::Broadcast(std::span<const std::byte> packet) {
  if (winsock_) {
    winsock_->Broadcast(packet, static_cast<int>(packet.size()));
  }
}

}  // namespace ra
