// File: Network, the links Red Alert plays a multiplayer game over.

#ifndef CNC_RED_ALERT_RA_NETWORK_H_
#define CNC_RED_ALERT_RA_NETWORK_H_

#include <cstdint>
#include <string>

#include "absl/base/attributes.h"
#include "engine/base/installed.h"
#include "ra/defines.h"
#include "ra/event.h"
#include "ra/ipxmgr.h"
#include "ra/nullmgr.h"
#include "ra/queue.h"

class ModemRegistryEntryClass;
class WinsockInterfaceClass;
class WolapiObject;

// The transports a multiplayer game runs over -- the null modem, the IPX
// manager and the Winsock interface -- together with the two event queues
// their traffic is, and the loose flags the connection dialogs keep.
//
// The queues are here rather than with the simulation because they are what
// crosses the wire: OutList holds the local player's commands until they are
// sent, and DoList holds every machine's commands until the frame they run
// on. Their contents decide the simulation, so nothing here may change the
// order events are queued or executed in.
//
// Game owns the one Network; everything else reaches it through
// TheNetwork(). Constructing one opens no socket and no port: the managers
// only allocate their queues, and Init_Network() and Init_Null_Modem() do
// the rest later.
//
// Example:
//   OutList.Add(event);
class Network {
 public:
  Network() = default;
  ~Network() = default;

  Network(const Network&) = delete;
  Network& operator=(const Network&) = delete;
  Network(Network&&) = delete;
  Network& operator=(Network&&) = delete;

  // Most of these hand out a reference: the connection code reads and
  // writes them all over, and a getter and setter pair for each would only
  // spell the same thing longer.

  // The events the local player has generated but not yet sent.
  QueueClass<EventClass, kMaxEvents>& out_list() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return out_list_;
  }

  // The events from every machine, waiting for the frame they execute on.
  QueueClass<EventClass, kMaxEvents * 64>& do_list()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return do_list_;
  }

  // The serial and modem link.
  NullModemClass& null_modem() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return null_modem_;
  }

  // The IPX link, which carries both the global channel the lobby uses and
  // one private channel per connected player.
  IPXManagerClass& ipx() ABSL_ATTRIBUTE_LIFETIME_BOUND { return ipx_; }

  // The Winsock interface the IPX manager sends through, or null when no
  // socket is open. Owned by the protocol code, not by Network.
  WinsockInterfaceClass*& packet_transport() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return packet_transport_;
  }

  // A packet that arrived before the game was ready for it, kept until it
  // is. Null when there is none.
  void*& packet_later() ABSL_ATTRIBUTE_LIFETIME_BOUND { return packet_later_; }

  // The Westwood Online session, or null when the player is not logged in.
  // Owned by the chat code, not by Network.
  WolapiObject*& wolapi() ABSL_ATTRIBUTE_LIFETIME_BOUND { return wolapi_; }

  // The modems the registry lists, as a linked list, or null before the
  // serial dialog has read it.
  ModemRegistryEntryClass*& modem_registry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return modem_registry_;
  }

  // The last line the modem sent back, shown in the serial dialog. Empty
  // when the modem has not answered since the dialog last cleared it.
  std::string& modem_response() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return modem_response_;
  }

  // The first and last frame of the window in which a raised MaxAhead is
  // not yet in force everywhere. An event scheduled inside it would reach
  // the other machines too late, so it is pushed past the end.
  int64_t& new_max_ahead_frame1() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return new_max_ahead_frame1_;
  }
  int64_t& new_max_ahead_frame2() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return new_max_ahead_frame2_;
  }

  // Westwood Online's record of the game in progress: which port the
  // statistics go to, whether this machine sets the options, and the id and
  // start time that identify the game to the server.
  int32_t& westwood_port() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return westwood_port_;
  }
  bool& westwood_is_host() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return westwood_is_host_;
  }
  uint32_t& westwood_game_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return westwood_game_id_;
  }
  uint32_t& westwood_start_time() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return westwood_start_time_;
  }

  // Whether the game statistics have gone out already, so they are not sent
  // twice, and whether the link to the other player has dropped.
  bool& statistics_sent() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return statistics_sent_;
  }
  bool& connection_lost() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return connection_lost_;
  }

  // Whether the player gave up on the reconnect dialog, which ends the game
  // rather than waiting any longer.
  bool& reconnect_cancelled() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return reconnect_cancelled_;
  }

 private:
  QueueClass<EventClass, kMaxEvents> out_list_;
  QueueClass<EventClass, kMaxEvents * 64> do_list_;

  // The magic number must have each digit unique and different from the
  // queue's own.
  NullModemClass null_modem_{
      16,  // number of send entries
      16,  // number of receive entries
      (MAX_SERIAL_PACKET_SIZE / sizeof(EventClass) * sizeof(EventClass)) +
          sizeof(CommHeaderType),
      0x1234};

  IPXManagerClass ipx_{
      std::max(sizeof(GlobalPacketType), sizeof(RemoteFileTransferType)),
      (546 - sizeof(CommHeaderType)) / sizeof(EventClass) * sizeof(EventClass),
      160,             // # entries in Global Queue
      32,              // # entries in Private Queues
      VIRGIN_SOCKET,   // Socket ID #
      IPXGlobalConnClass::kCommandAndConquer0};  // Product ID #

  WinsockInterfaceClass* packet_transport_ = nullptr;
  void* packet_later_ = nullptr;
  WolapiObject* wolapi_ = nullptr;
  ModemRegistryEntryClass* modem_registry_ = nullptr;
  std::string modem_response_;

  int64_t new_max_ahead_frame1_ = 0;
  int64_t new_max_ahead_frame2_ = 0;

  int32_t westwood_port_ = 1234;
  bool westwood_is_host_ = false;
  uint32_t westwood_game_id_ = 0;
  uint32_t westwood_start_time_ = 0;

  bool statistics_sent_ = false;
  bool connection_lost_ = false;
  bool reconnect_cancelled_ = false;
};

// Returns the Network that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Network& TheNetwork() { return base::Installed<Network>::Get(); }

#endif  // CNC_RED_ALERT_RA_NETWORK_H_
