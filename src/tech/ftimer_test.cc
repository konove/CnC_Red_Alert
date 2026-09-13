#include "tech/ftimer.h"

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "tech/archive.h"
#include "tech/pipe.h"
#include "tech/xstraw.h"

namespace {

// A tick source the test advances by hand.
struct FakeTick {
  static int64_t now;
  static int64_t Tick() { return now; }
};
int64_t FakeTick::now = 0;

class ByteSink : public Pipe {
 public:
  int Put(const void* source, int slen) override {
    const auto* begin = static_cast<const uint8_t*>(source);
    bytes.insert(bytes.end(), begin, begin + slen);
    return slen;
  }
  std::vector<uint8_t> bytes;
};

// Writes `subject`, advances the clock by `skew`, and reads it back into a
// fresh object. The skew models the wall-clock gap between saving and
// loading, which must not leak into the restored value.
template <class T>
T RoundTrip(T& subject, int64_t skew) {
  ByteSink sink;
  ArchiveWriter writer(sink);
  subject.Serialize(writer);
  EXPECT_EQ(sink.bytes.size(), 9U);  // int64_t value + bool running

  FakeTick::now += skew;
  BufferStraw straw(sink.bytes.data(), static_cast<int>(sink.bytes.size()));
  ArchiveReader reader(straw);
  T restored;
  restored.Serialize(reader);
  EXPECT_TRUE(reader.ok());
  return restored;
}

class TimerSerializeTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTick::now = 1000; }
};

TEST_F(TimerSerializeTest, RunningTimerKeepsItsRemainingValue) {
  Timer<FakeTick> timer(50);
  FakeTick::now += 20;
  ASSERT_EQ(timer.Value(), 30);

  Timer<FakeTick> restored = RoundTrip(timer, 500);
  EXPECT_EQ(restored.Value(), 30);
  EXPECT_TRUE(restored.IsRunning());

  FakeTick::now += 10;
  EXPECT_EQ(restored.Value(), 20);
}

TEST_F(TimerSerializeTest, StoppedTimerStaysStopped) {
  Timer<FakeTick> timer(50);
  FakeTick::now += 5;
  timer.Stop();

  Timer<FakeTick> restored = RoundTrip(timer, 500);
  EXPECT_FALSE(restored.IsRunning());
  EXPECT_EQ(restored.Value(), 45);
  FakeTick::now += 100;
  EXPECT_EQ(restored.Value(), 45);

  restored.Start();
  FakeTick::now += 5;
  EXPECT_EQ(restored.Value(), 40);
}

TEST_F(TimerSerializeTest, FinishedTimerStaysFinished) {
  Timer<FakeTick> timer(10);
  FakeTick::now += 100;
  ASSERT_TRUE(timer.IsFinished());

  Timer<FakeTick> restored = RoundTrip(timer, 500);
  EXPECT_TRUE(restored.IsFinished());
  EXPECT_EQ(restored.Value(), 0);
}

TEST_F(TimerSerializeTest, StopwatchKeepsAccumulatedTicks) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 40;
  ASSERT_EQ(watch.Value(), 40);

  Stopwatch<FakeTick> restored = RoundTrip(watch, 500);
  EXPECT_EQ(restored.Value(), 40);
  EXPECT_TRUE(restored.IsRunning());
  FakeTick::now += 2;
  EXPECT_EQ(restored.Value(), 42);
}

TEST_F(TimerSerializeTest, StoppedStopwatchStaysStopped) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 40;
  watch.Stop();

  Stopwatch<FakeTick> restored = RoundTrip(watch, 500);
  EXPECT_FALSE(restored.IsRunning());
  FakeTick::now += 100;
  EXPECT_EQ(restored.Value(), 40);
}

}  // namespace
