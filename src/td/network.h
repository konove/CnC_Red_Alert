// File: Network, the links Tiberian Dawn plays a multiplayer game over.

#ifndef CNC_RED_ALERT_TD_NETWORK_H_
#define CNC_RED_ALERT_TD_NETWORK_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/base/installed.h"
#include "td/defines.h"
#include "td/event.h"
#include "td/ipxaddr.h"
#include "td/ipxmgr.h"
#include "td/nodename.h"
#include "td/nullmgr.h"
#include "td/queue.h"
#include "td/tcpip.h"
#include "td/vector.h"

class ModemRegistryEntryClass;
class PhoneEntryClass;

// The transports a multiplayer game runs over -- the null modem and the IPX
// manager -- together with the two event queues their traffic is, the
// phone book the serial dialog dials from, the lists the network dialog
// builds, and the values that identify the game to Westwood Chat.
//
// The queues are here rather than with the simulation because they are what
// crosses the wire: OutList holds the local player's commands until they
// are sent, and DoList holds every machine's commands until the frame they
// run on. Their contents decide the simulation, so nothing here may change
// the order events are queued or executed in.
//
// Game owns the one Network; everything else reaches it through
// TheNetwork(). Constructing one opens no socket and no port.
//
// Example:
//   TheNetwork().out_list().Add(event);
class Network {
 public:
  // The size of the buffer the server or peer address is kept in, which is
  // what IP_ADDRESS_MAX has always been.
  static constexpr int kAddressLength = 40;

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
  QueueClass<EventClass, MAX_EVENTS>& out_list() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return out_list_;
  }

  // The events from every machine, waiting for the frame they execute on.
  QueueClass<EventClass, MAX_EVENTS * 8>& do_list()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return do_list_;
  }

  // The serial and modem link, and whether the main loop is allowed to
  // service it; the dialogs switch that off while they drive it directly.
  NullModemClass& null_modem() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return null_modem_;
  }
  bool& modem_service() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return modem_service_;
  }

  // The IPX link, which carries both the global channel the lobby uses and
  // one private channel per connected player.
  IPXManagerClass& ipx() ABSL_ATTRIBUTE_LIFETIME_BOUND { return ipx_; }

  // The numbers the serial dialog can dial, which one is selected, the
  // modem init strings to choose from, the port settings read from the INI
  // file, and which end of a modem game this machine is.
  DynamicVectorClass<PhoneEntryClass*>& phone_book()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return phone_book_;
  }
  int& current_phone_index() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return current_phone_index_;
  }
  DynamicVectorClass<char*>& init_strings() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return init_strings_;
  }
  SerialSettingsType& serial_defaults() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return serial_defaults_;
  }
  ModemGameType& modem_game_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return modem_game_type_;
  }

  // The address of a game owner on the far side of a bridge, and whether
  // to use it. Only the first four numbers are taken from it; the rest are
  // set to broadcast.
  int& is_bridge() ABSL_ATTRIBUTE_LIFETIME_BOUND { return is_bridge_; }
  IPXAddressClass& bridge_net() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bridge_net_;
  }

  // Whether this game hides from machines that are just starting up,
  // whether messages from players outside it are refused, and whether it
  // is still accepting players.
  bool& stealth() ABSL_ATTRIBUTE_LIFETIME_BOUND { return stealth_; }
  bool& protect() ABSL_ATTRIBUTE_LIFETIME_BOUND { return protect_; }
  bool& is_open() ABSL_ATTRIBUTE_LIFETIME_BOUND { return is_open_; }

  // The global channel's scratch space: the packet just received, how long
  // it was, who sent it and which product they are running.
  GlobalPacketType& global_packet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return global_packet_;
  }
  int& global_packet_length() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return global_packet_length_;
  }
  IPXAddressClass& global_address() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return global_address_;
  }
  uint16_t& product_id() ABSL_ATTRIBUTE_LIFETIME_BOUND { return product_id_; }

  // A batch of events sent as one packet, sized to IPX's 546-byte limit
  // rounded down to whole events.
  std::vector<std::byte>& meta_packet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return meta_packet_;
  }

  // The games the network dialog has heard of, with the address of each
  // one's owner, and the players in the game this machine is joining. The
  // second list is what connections are formed from, and it is filled both
  // from replies to this machine's query and from other machines' queries,
  // so anyone who knows about this machine is known to it in turn.
  DynamicVectorClass<NodeNameType*>& games() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return games_;
  }
  DynamicVectorClass<NodeNameType*>& players() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return players_;
  }

  // The TCP/IP transport, whether this machine is the server rather than a
  // client, whether the games are routed through Westwood's subnet server,
  // and how many players an internet game may hold.
  TcpipManagerClass& winsock() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return winsock_;
  }
  bool& is_server() ABSL_ATTRIBUTE_LIFETIME_BOUND { return is_server_; }
  bool& use_subnet_server() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return use_subnet_server_;
  }
  int& internet_max_players() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return internet_max_players_;
  }

  // The modems the registry lists, as a linked list, or null before the
  // serial dialog has read it.
  ModemRegistryEntryClass*& modem_registry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return modem_registry_;
  }

  // A packet that arrived before the game was ready for it, kept until it
  // is. Null when there is none.
  void*& packet_later() ABSL_ATTRIBUTE_LIFETIME_BOUND { return packet_later_; }

  // The first and last frame of the window in which a raised MaxAhead is
  // not yet in force everywhere. An event scheduled inside it would reach
  // the other machines too late, so it is pushed past the end.
  int64_t& new_max_ahead_frame1() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return new_max_ahead_frame1_;
  }
  int64_t& new_max_ahead_frame2() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return new_max_ahead_frame2_;
  }

  // Westwood Chat's record of the game: where the statistics go, whether
  // this machine sets the options, and the id and start time that identify
  // the game to the server.
  auto& westwood_address() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return westwood_address_;
  }
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

  // What Westwood Chat asks the game to use in place of the values the
  // dialogs would have set.
  int& chat_max_ahead() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return chat_max_ahead_;
  }
  int& chat_send_rate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return chat_send_rate_;
  }

  // Whether the game statistics have gone out already, so they are not
  // sent twice, and whether the link to the other player has dropped.
  bool& statistics_sent() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return statistics_sent_;
  }
  bool& connection_lost() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return connection_lost_;
  }

 private:
  QueueClass<EventClass, MAX_EVENTS> out_list_;
  QueueClass<EventClass, MAX_EVENTS * 8> do_list_;

  // The magic number must have each digit unique and different from the
  // queue's own.
  NullModemClass null_modem_{
      16,  // number of send entries
      64,  // number of receive entries
      (200 / sizeof(EventClass) * sizeof(EventClass)) + sizeof(CommHeaderType),
      0x1234};
  bool modem_service_ = true;

  IPXManagerClass ipx_{
      sizeof(GlobalPacketType),
      (546 - sizeof(CommHeaderType)) / sizeof(EventClass) * sizeof(EventClass),
      10,             // # entries in Global Queue
      8,              // # entries in Private Queues
      VIRGIN_SOCKET,  // Socket ID #
      IPXGlobalConnClass::kCommandAndConquer};  // Product ID #

  DynamicVectorClass<PhoneEntryClass*> phone_book_;
  int current_phone_index_ = 0;
  DynamicVectorClass<char*> init_strings_;
  SerialSettingsType serial_defaults_{};
  ModemGameType modem_game_type_{};

  int is_bridge_ = 0;
  IPXAddressClass bridge_net_;

  bool stealth_ = false;
  bool protect_ = true;
  bool is_open_ = false;

  GlobalPacketType global_packet_{};
  int global_packet_length_ = 0;
  IPXAddressClass global_address_;
  uint16_t product_id_ = 0;

  std::vector<std::byte> meta_packet_;

  DynamicVectorClass<NodeNameType*> games_;
  DynamicVectorClass<NodeNameType*> players_;

  TcpipManagerClass winsock_;
  bool is_server_ = false;
  bool use_subnet_server_ = false;
  int internet_max_players_ = 0;

  ModemRegistryEntryClass* modem_registry_ = nullptr;

  void* packet_later_ = nullptr;

  int64_t new_max_ahead_frame1_ = 0;
  int64_t new_max_ahead_frame2_ = 0;

  char westwood_address_[kAddressLength] = "206.154.108.87";
  int32_t westwood_port_ = 1234;
  bool westwood_is_host_ = false;
  uint32_t westwood_game_id_ = 0;
  uint32_t westwood_start_time_ = 0;

  int chat_max_ahead_ = 0;
  int chat_send_rate_ = 0;

  bool statistics_sent_ = false;
  bool connection_lost_ = false;
};

// Returns the Network that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Network& TheNetwork() { return base::Installed<Network>::Get(); }

#endif  // CNC_RED_ALERT_TD_NETWORK_H_
