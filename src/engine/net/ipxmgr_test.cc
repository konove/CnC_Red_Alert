// Runs two IPX managers against each other over an in-memory network.
#include "engine/net/ipxmgr.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/unaligned.h"
#include "engine/net/combuf.h"
#include "engine/net/connect.h"
#include "engine/net/datagram_transport.h"
#include "engine/net/global_conn.h"
#include "engine/net/ipxaddr.h"
#include "gtest/gtest.h"

namespace engine::net {
namespace {

constexpr uint16_t kProductId = 0xbeef;
constexpr int kGlobalMagic = 0x4321;

IPXAddressClass Address(const unsigned char node) {
  const NetNumType net = {10, 0, 0, 1};
  const NetNodeType bytes = {0, 0, 0, 0, 0, node};
  return {net, bytes};
}

int Key(const IPXAddressClass& address) {
  NetNumType net = {};
  NetNodeType node = {};
  IPXAddressClass copy = address;
  copy.Get_Address(net, node);
  return node[5];
}

// Datagrams in flight between the endpoints of one test.
struct Datagram {
  std::vector<std::byte> bytes;
  IPXAddressClass from;
};
using Inboxes = std::map<int, std::deque<Datagram>>;

// One endpoint of the in-memory network: what it writes lands in the
// addressee's inbox, a broadcast in everyone else's.
class LoopbackTransport final : public DatagramTransport {
 public:
  LoopbackTransport(Inboxes& inboxes ABSL_ATTRIBUTE_LIFETIME_BOUND,
                    const IPXAddressClass& self)
      : inboxes_(inboxes), self_(self) {
    inboxes_[Key(self_)];
  }

  bool Start_Listening(uint16_t /*socket*/) override { return true; }
  void Stop_Listening() override {}

  int Read(std::span<std::byte> buffer, IPXAddressClass& from) override {
    std::deque<Datagram>& inbox = inboxes_[Key(self_)];
    if (inbox.empty()) {
      return 0;
    }
    Datagram datagram = std::move(inbox.front());
    inbox.pop_front();
    base::CopyBytes(buffer, datagram.bytes, datagram.bytes.size());
    from = datagram.from;
    return static_cast<int>(datagram.bytes.size());
  }

  void Write_To(std::span<const std::byte> packet,
                const IPXAddressClass& to) override {
    inboxes_[Key(to)].push_back({{packet.begin(), packet.end()}, self_});
  }

  void Broadcast(std::span<const std::byte> packet) override {
    for (auto& [key, inbox] : inboxes_) {
      if (key != Key(self_)) {
        inbox.push_back({{packet.begin(), packet.end()}, self_});
      }
    }
  }

 private:
  Inboxes& inboxes_;
  IPXAddressClass self_;
};

struct TestGlobalHeader {
  CommHeaderType Header;
  uint16_t ProductID;
};

// A global channel that only broadcasts and never ACKs: the least the
// manager needs. The games' channels are tested through the games.
class TestGlobalConn final : public GlobalConnClass {
 public:
  TestGlobalConn(DatagramTransport& transport, int num_packets, int maxlen,
                 uint16_t product_id)
      : GlobalConnClass(transport, num_packets, num_packets,
                        maxlen + static_cast<int>(sizeof(TestGlobalHeader)),
                        kGlobalMagic, nullptr, 0, "", sizeof(IPXAddressClass)),
        product_id_(product_id) {}

  static std::unique_ptr<GlobalConnClass> Make(DatagramTransport& transport,
                                               int num_packets, int maxlen,
                                               uint16_t product_id) {
    return std::make_unique<TestGlobalConn>(transport, num_packets, maxlen,
                                            product_id);
  }

  int Send_Packet(std::span<const std::byte> buf, int buflen,
                  IPXAddressClass* /*address*/, int /*ack_req*/) override {
    TestGlobalHeader header{};
    header.Header.MagicNumber = MagicNum;
    header.Header.Code = static_cast<unsigned char>(PACKET_DATA_NOACK);
    header.ProductID = product_id_;
    base::WriteUnaligned(std::span(PacketBuf), header);
    base::CopyBytes(std::span(PacketBuf).subspan(sizeof(header)), buf,
                    base::ToSize(buflen));
    const IPXAddressClass everyone;
    return Queue->Queue_Send(
        PacketBuf, buflen + static_cast<int>(sizeof(header)),
        base::ObjectBytes(everyone), sizeof(IPXAddressClass));
  }

  int Receive_Packet(std::span<std::byte> buf, int buflen,
                     IPXAddressClass* address) override {
    return Queue->Queue_Receive(buf, buflen, base::ObjectBytes(*address),
                                sizeof(IPXAddressClass));
  }

  int Get_Packet(std::span<std::byte> buf, int* buflen,
                 IPXAddressClass* address, uint16_t* product_id) override {
    if (Queue->Num_Receive() == 0) {
      return 0;
    }
    ReceiveQueueType* entry = Queue->Get_Receive(0);
    if (entry->IsRead) {
      return 0;
    }
    entry->IsRead = 1;
    const auto header = base::ReadUnaligned<TestGlobalHeader>(entry->Buffer);
    *buflen = entry->BufLen - static_cast<int>(sizeof(header));
    base::CopyBytes(buf, std::span(entry->Buffer).subspan(sizeof(header)),
                    base::ToSize(*buflen));
    *product_id = header.ProductID;
    *address = base::ReadUnaligned<IPXAddressClass>(entry->ExtraBuffer);
    return 1;
  }

 protected:
  int Send(std::span<const std::byte> buf, int buflen,
           std::span<const std::byte> /*extrabuf*/, int /*extralen*/) override {
    return Broadcast(buf, buflen) ? 1 : 0;
  }

  int Service_Receive_Queue() override {
    for (int i = 0; i < Queue->Num_Receive(); i++) {
      if (Queue->Get_Receive(i)->IsRead) {
        Queue->UnQueue_Receive({}, nullptr, i, {}, nullptr);
        i--;
      }
    }
    return 1;
  }

 private:
  uint16_t product_id_;
};

// Two managers that can see each other: `a` at node 1, `b` at node 2.
class IPXManagerTest : public testing::Test {
 protected:
  static constexpr int kMaxLen = 64;

  void Service(int rounds = 3) {
    for (int i = 0; i < rounds; i++) {
      a_.Service();
      b_.Service();
    }
  }

  Inboxes inboxes_;
  IPXAddressClass a_address_ = Address(1);
  IPXAddressClass b_address_ = Address(2);
  LoopbackTransport a_link_{inboxes_, a_address_};
  LoopbackTransport b_link_{inboxes_, b_address_};
  IPXManagerClass a_{
      a_link_, true, &TestGlobalConn::Make, 4,         kMaxLen, kMaxLen,
      4,       4,    kVirginSocket,         kProductId};
  IPXManagerClass b_{
      b_link_, true, &TestGlobalConn::Make, 4,         kMaxLen, kMaxLen,
      4,       4,    kVirginSocket,         kProductId};
};

TEST_F(IPXManagerTest, DeliversAPrivateMessageAndItsAck) {
  ASSERT_EQ(a_.Init(false), 1);
  ASSERT_EQ(b_.Init(false), 1);
  char b_name[] = "b";
  char a_name[] = "a";
  ASSERT_EQ(a_.Create_Connection(2, b_name, &b_address_), 1);
  ASSERT_EQ(b_.Create_Connection(1, a_name, &a_address_), 1);

  constexpr std::array<char, 5> kHello = {'h', 'e', 'l', 'l', 'o'};
  ASSERT_EQ(a_.Send_Private_Message(base::ObjectBytes(kHello), 5, 1, 2), 1);
  EXPECT_EQ(a_.Private_Num_Send(2), 1);
  Service();

  std::array<char, kMaxLen> received{};
  int length = 0;
  int from = 0;
  ASSERT_EQ(b_.Get_Private_Message(base::ObjectBytes(received), &length, &from),
            1);
  EXPECT_EQ(length, 5);
  EXPECT_EQ(from, 1);
  EXPECT_TRUE(
      std::ranges::equal(std::span(received).first(kHello.size()), kHello));

  // B's ACK clears A's send queue.
  EXPECT_EQ(a_.Private_Num_Send(2), 0);
}

TEST_F(IPXManagerTest, BroadcastsAGlobalMessageWithItsSender) {
  ASSERT_EQ(a_.Init(false), 1);
  ASSERT_EQ(b_.Init(false), 1);

  constexpr std::array<char, 3> kPing = {'p', 'i', 'n'};
  ASSERT_EQ(a_.Send_Global_Message(base::ObjectBytes(kPing), 3), 1);
  Service();

  std::array<char, kMaxLen> received{};
  int length = 0;
  IPXAddressClass sender;
  uint16_t product = 0;
  ASSERT_EQ(b_.Get_Global_Message(base::ObjectBytes(received), &length, &sender,
                                  &product),
            1);
  EXPECT_EQ(length, 3);
  EXPECT_EQ(product, kProductId);
  EXPECT_TRUE(sender == a_address_);
}

TEST_F(IPXManagerTest, ReportsAPrivatePacketFromAnUnknownAddress) {
  ASSERT_EQ(a_.Init(true), 1);
  ASSERT_EQ(b_.Init(true), 1);
  char b_name[] = "b";
  ASSERT_EQ(a_.Create_Connection(2, b_name, &b_address_), 1);
  // B has no connection to A, so A's packet comes from a stranger.
  static int calls = 0;
  static IPXAddressClass stranger;
  calls = 0;
  b_.Set_Unknown_Sender_Handler([](IPXManagerClass& /*manager*/,
                                   std::span<const std::byte> payload,
                                   const IPXAddressClass& from) {
    ++calls;
    stranger = from;
    EXPECT_EQ(payload.size(), 2U);
  });

  constexpr std::array<char, 2> kSync = {'s', 'y'};
  ASSERT_EQ(a_.Send_Private_Message(base::ObjectBytes(kSync), 2, 0, 2), 1);
  Service();

  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(stranger == a_address_);
}

TEST_F(IPXManagerTest, WithoutIpxOnlyAnInternetGameStarts) {
  IPXManagerClass offline{
      a_link_, false, &TestGlobalConn::Make, 4,         kMaxLen, kMaxLen,
      4,       4,     kVirginSocket,         kProductId};
  EXPECT_EQ(offline.Init(false), 0);
  EXPECT_FALSE(offline.Is_IPX());
  EXPECT_EQ(offline.Init(true), 1);
  EXPECT_TRUE(offline.Is_IPX());
}

TEST_F(IPXManagerTest, CapsTheNumberOfConnections) {
  IPXManagerClass small{
      a_link_, true, &TestGlobalConn::Make, 1,         kMaxLen, kMaxLen,
      4,       4,    kVirginSocket,         kProductId};
  ASSERT_EQ(small.Init(false), 1);
  char name[] = "x";
  EXPECT_EQ(small.Create_Connection(2, name, &b_address_), 1);
  EXPECT_EQ(small.Create_Connection(3, name, &b_address_), 0);
  EXPECT_EQ(small.Delete_Connection(2), 1);
  EXPECT_EQ(small.Num_Connections(), 0);
}

}  // namespace
}  // namespace engine::net
