// File: DatagramTransport, the link an IPX manager's packets travel over.

#ifndef CNC_RED_ALERT_ENGINE_NET_DATAGRAM_TRANSPORT_H_
#define CNC_RED_ALERT_ENGINE_NET_DATAGRAM_TRANSPORT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/net/ipxaddr.h"

namespace engine::net {

// Sends and receives whole datagrams addressed by IPXAddressClass. Red Alert
// implements it over UDP, Tiberian Dawn over its one TCP connection, the
// tests over a loopback queue.
//
// Example:
//   std::array<std::byte, 1024> buffer;
//   IPXAddressClass from;
//   while (const int length = transport.Read(buffer, from)) { ... }
class DatagramTransport {
 public:
  DatagramTransport() = default;
  virtual ~DatagramTransport();
  DatagramTransport(const DatagramTransport&) = delete;
  DatagramTransport& operator=(const DatagramTransport&) = delete;
  DatagramTransport(DatagramTransport&&) = delete;
  DatagramTransport& operator=(DatagramTransport&&) = delete;

  // Opens `socket` (in network byte order) and starts listening on it.
  // Returns false, with nothing left open, if either step fails.
  virtual bool Start_Listening(uint16_t socket) = 0;

  // Stops listening.
  virtual void Stop_Listening() = 0;

  // Reads the next waiting datagram into `buffer` and its sender into
  // `from`. Returns the datagram's length, or 0 when none is waiting.
  virtual int Read(std::span<std::byte> buffer, IPXAddressClass& from) = 0;

  // Sends `packet` to `to`.
  virtual void Write_To(std::span<const std::byte> packet,
                        const IPXAddressClass& to) = 0;

  // Sends `packet` to every listener the transport can reach.
  virtual void Broadcast(std::span<const std::byte> packet) = 0;
};

}  // namespace engine::net

#endif  // CNC_RED_ALERT_ENGINE_NET_DATAGRAM_TRANSPORT_H_
