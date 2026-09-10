// Round-trip coverage for non-heap Red Alert save state.

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"
#include "ra/_wsproto.h"
#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/special.h"
#include "tech/archive.h"
#include "tech/ftimer.h"
#include "tech/pipe.h"
#include "tech/xstraw.h"

// Game clock used by the scenario serializer in this test executable.
int64_t Frame = 0;
// Address tests do not use a live network transport.
WinsockInterfaceClass* PacketTransport = nullptr;

namespace {

static_assert(std::is_trivially_default_constructible_v<SpecialClass>);

class GlobalsPipe : public Pipe {
 public:
  int Put(const void* source, int length) override {
    const auto* begin = static_cast<const uint8_t*>(source);
    bytes.insert(bytes.end(), begin, begin + length);
    return length;
  }
  std::vector<uint8_t> bytes;
};

template <class T>
bool ReadValue(T& value, const std::vector<uint8_t>& bytes) {
  BufferStraw straw(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(straw);
  reader(value);
  return reader.ok();
}

TEST(SaveGlobalsTest, SpecialFlagsUseExactlyOneByteForEveryCombination) {
  for (int flags = 0; flags < 256; ++flags) {
    std::vector<uint8_t> bytes{static_cast<uint8_t>(flags)};
    SpecialClass special{};
    ASSERT_TRUE(ReadValue(special, bytes));
    GlobalsPipe pipe;
    ArchiveWriter writer(pipe);
    writer(special);
    EXPECT_EQ(pipe.bytes, bytes);
  }
}

TEST(SaveGlobalsTest, ScorePreservesWideCountersAndPausedTime) {
  ScoreClass score;
  score.NKilled = 17;
  score.GHarvested = 123456;
  score.ElapsedTime = int64_t{1} << 40;
  GlobalsPipe timer_data;
  ArchiveWriter timer_writer(timer_data);
  int64_t elapsed = 9876543210;
  bool running = false;
  timer_writer(elapsed, running);
  ASSERT_TRUE(ReadValue(score.RealTime, timer_data.bytes));

  GlobalsPipe pipe;
  ArchiveWriter writer(pipe);
  writer(score);
  EXPECT_EQ(score.RealTime.Value(), elapsed);
  EXPECT_FALSE(score.RealTime.IsRunning());

  ScoreClass loaded;
  ASSERT_TRUE(ReadValue(loaded, pipe.bytes));
  EXPECT_EQ(loaded.NKilled, 17);
  EXPECT_EQ(loaded.GHarvested, 123456);
  EXPECT_EQ(loaded.ElapsedTime, int64_t{1} << 40);
  EXPECT_EQ(loaded.RealTime.Value(), elapsed);
  EXPECT_FALSE(loaded.RealTime.IsRunning());

  score.RealTime.Start();
  GlobalsPipe running_data;
  ArchiveWriter running_writer(running_data);
  running_writer(score);
  EXPECT_TRUE(score.RealTime.IsRunning());
  ASSERT_TRUE(ReadValue(loaded, running_data.bytes));
  EXPECT_TRUE(loaded.RealTime.IsRunning());
}

TEST(SaveGlobalsTest, TruncatedScoreFails) {
  ScoreClass score;
  GlobalsPipe pipe;
  ArchiveWriter writer(pipe);
  writer(score);
  pipe.bytes.pop_back();
  ScoreClass loaded;
  EXPECT_FALSE(ReadValue(loaded, pipe.bytes));
}

TEST(SaveGlobalsTest, PlayerRecordPreservesNameAddressAndPlayerFields) {
  NodeNameType player{};
  std::memcpy(player.Name, "Player", 7);
  NetNumType network = {1, 2, 3, 4};
  NetNodeType node = {5, 6, 7, 8, 9, 10};
  player.Address.Set_Address(network, node);
  player.Player.House = HOUSE_USSR;
  player.Player.Color = PCOLOR_RED;
  player.Player.ID = HOUSE_MULTI1;
  player.Player.ProcessTime = 123;
  GlobalsPipe pipe;
  ArchiveWriter writer(pipe);
  writer(player);
  EXPECT_EQ(pipe.bytes.size(), sizeof(player.Name) + 10 + 4 * sizeof(int32_t));

  NodeNameType loaded{};
  ASSERT_TRUE(ReadValue(loaded, pipe.bytes));
  EXPECT_STREQ(loaded.Name, "Player");
  NetNumType loaded_network{};
  NetNodeType loaded_node{};
  loaded.Address.Get_Address(loaded_network, loaded_node);
  EXPECT_EQ(std::memcmp(loaded_network, network, sizeof(network)), 0);
  EXPECT_EQ(std::memcmp(loaded_node, node, sizeof(node)), 0);
  EXPECT_EQ(loaded.Player.House, HOUSE_USSR);
  EXPECT_EQ(loaded.Player.Color, PCOLOR_RED);
  EXPECT_EQ(loaded.Player.ID, HOUSE_MULTI1);
  EXPECT_EQ(loaded.Player.ProcessTime, 123);
}

}  // namespace
