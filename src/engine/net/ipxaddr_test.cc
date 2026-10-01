#include "engine/net/ipxaddr.h"

#include "gtest/gtest.h"

namespace engine::net {
namespace {

TEST(IPXAddressTest, DefaultIsBroadcast) {
  IPXAddressClass address;
  EXPECT_TRUE(address.Is_Broadcast());
}

TEST(IPXAddressTest, SetAndGetRoundTrip) {
  const NetNumType net = {10, 0, 0, 1};
  const NetNodeType node = {1, 2, 3, 4, 5, 6};
  IPXAddressClass address(net, node);
  EXPECT_FALSE(address.Is_Broadcast());

  NetNumType net_out = {};
  NetNodeType node_out = {};
  address.Get_Address(net_out, node_out);
  EXPECT_EQ(net_out[3], 1);
  EXPECT_EQ(node_out[5], 6);
}

TEST(IPXAddressTest, ZeroNetworkMatchesOnTheNodeAlone) {
  const NetNumType net = {10, 0, 0, 1};
  const NetNumType no_net = {0, 0, 0, 0};
  const NetNodeType node = {1, 2, 3, 4, 5, 6};
  IPXAddressClass full(net, node);
  IPXAddressClass node_only(no_net, node);
  EXPECT_TRUE(full == node_only);
  EXPECT_FALSE(full != node_only);

  const NetNumType other_net = {10, 0, 0, 2};
  IPXAddressClass elsewhere(other_net, node);
  EXPECT_FALSE(full == elsewhere);
  EXPECT_TRUE(full != elsewhere);
}

}  // namespace
}  // namespace engine::net
