// File: GlobalConnClass, the interface IPXManagerClass drives each game's
// global channel through.

#ifndef CNC_RED_ALERT_ENGINE_NET_GLOBAL_CONN_H_
#define CNC_RED_ALERT_ENGINE_NET_GLOBAL_CONN_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/net/ipxaddr.h"
#include "engine/net/ipxconn.h"

namespace engine::net {

// The global channel the lobby talks over: packets to and from any address,
// each tagged with a product ID. Its header layout and ACK rules are part of
// each game's wire format, so each game implements it.
class GlobalConnClass : public IPXConnClass {
 public:
  using IPXConnClass::IPXConnClass;
  ~GlobalConnClass() override;
  GlobalConnClass(const GlobalConnClass&) = delete;
  GlobalConnClass& operator=(const GlobalConnClass&) = delete;
  GlobalConnClass(GlobalConnClass&&) = delete;
  GlobalConnClass& operator=(GlobalConnClass&&) = delete;

  // These deliberately hide the inherited point-to-point routines rather than
  // overriding them: a global packet carries a header with a product ID and
  // an address, which the base versions do not write.

  // Queues `buf` for `address`, or for everyone when `address` is null. Only
  // an addressed packet can ask for an ACK. Returns 0 if the queue is full
  // or the packet is too long.
  // NOLINTNEXTLINE(clang-diagnostic-overloaded-virtual)
  virtual int Send_Packet(std::span<const std::byte> buf, int buflen,
                          IPXAddressClass* address, int ack_req) = 0;

  // Takes a packet that arrived from `address`. Returns 0 if it was not a
  // global packet or could not be queued.
  // NOLINTNEXTLINE(clang-diagnostic-overloaded-virtual)
  virtual int Receive_Packet(std::span<std::byte> buf, int buflen,
                             IPXAddressClass* address) = 0;

  // Copies the next unread packet into `buf`, its length into `buflen`, its
  // sender into `address` and its product ID into `product_id`. Returns 0
  // when none is waiting.
  // NOLINTNEXTLINE(clang-diagnostic-overloaded-virtual)
  virtual int Get_Packet(std::span<std::byte> buf, int* buflen,
                         IPXAddressClass* address, uint16_t* product_id) = 0;
};

}  // namespace engine::net

#endif  // CNC_RED_ALERT_ENGINE_NET_GLOBAL_CONN_H_
