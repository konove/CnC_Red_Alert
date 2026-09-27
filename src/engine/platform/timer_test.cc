#include "engine/platform/timer.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace {

// A tick source the test advances by hand.
struct FakeTick {
  static int64_t now;
  static int64_t Tick() { return now; }
};
int64_t FakeTick::now = 0;

class RecordingSink : public ByteSink {
 public:
  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return true;
  }
  std::vector<uint8_t> bytes;
};

// Writes `subject`, advances the clock by `skew`, and reads it back into a
// fresh object. The skew models the wall-clock gap between saving and
// loading, which must not leak into the restored value.
template <class T>
T RoundTrip(T& subject, int64_t skew) {
  RecordingSink sink;
  ArchiveWriter writer(sink);
  subject.Serialize(writer);
  EXPECT_EQ(sink.bytes.size(), 9U);  // int64_t value + bool running

  FakeTick::now += skew;
  SpanSource straw(std::as_bytes(std::span(sink.bytes)));
  ArchiveReader reader(straw);
  T restored;
  restored.Serialize(reader);
  EXPECT_TRUE(reader.ok());
  return restored;
}

// Reads a timer from a saved (value, running) pair that no timer could have
// written. Returns whether the reader accepted it.
template <class T>
bool LoadsRaw(T& subject, int64_t value, bool running) {
  RecordingSink sink;
  ArchiveWriter writer(sink);
  writer(value, running);
  SpanSource straw(std::as_bytes(std::span(sink.bytes)));
  ArchiveReader reader(straw);
  subject.Serialize(reader);
  return reader.ok();
}

class TimerTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTick::now = 1000; }
};

TEST_F(TimerTest, CountsDownAndStopsAtZero) {
  const Timer<FakeTick> timer(30);
  EXPECT_TRUE(timer.IsRunning());
  EXPECT_TRUE(timer.HasTimeLeft());
  FakeTick::now += 10;
  EXPECT_EQ(timer.Value(), 20);
  FakeTick::now += 25;
  EXPECT_EQ(timer.Value(), 0);
  EXPECT_TRUE(timer.IsFinished());
  EXPECT_TRUE(timer.IsRunning());
}

TEST_F(TimerTest, StopFreezesAndStartResumes) {
  Timer<FakeTick> timer(50);
  FakeTick::now += 10;
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  FakeTick::now += 100;
  EXPECT_EQ(timer.Value(), 40);

  timer.Start();
  FakeTick::now += 5;
  EXPECT_EQ(timer.Value(), 35);
}

TEST_F(TimerTest, SetWhileStoppedRestartsTheCountdown) {
  Timer<FakeTick> timer(50);
  timer.Stop();
  timer.Set(20);
  EXPECT_TRUE(timer.IsRunning());
  FakeTick::now += 5;
  EXPECT_EQ(timer.Value(), 15);
}

TEST_F(TimerTest, StoppedAtZeroReadsFinished) {
  Timer<FakeTick> timer(50);
  timer.Set(0);
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  EXPECT_TRUE(timer.IsFinished());
  EXPECT_EQ(timer.Value(), 0);
}

TEST_F(TimerTest, StopwatchPausesAndResumes) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 10;
  watch.Stop();
  FakeTick::now += 100;
  EXPECT_EQ(watch.Value(), 10);

  watch.Start();
  FakeTick::now += 5;
  EXPECT_EQ(watch.Value(), 15);
}

TEST_F(TimerTest, StopwatchResetZeroesAndRuns) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 10;
  watch.Stop();
  watch.Reset();
  EXPECT_TRUE(watch.IsRunning());
  EXPECT_EQ(watch.Value(), 0);
  FakeTick::now += 3;
  EXPECT_EQ(watch.Value(), 3);
}

class TimerSerializeTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTick::now = 1000; }
};

TEST_F(TimerSerializeTest, RunningTimerKeepsItsRemainingValue) {
  Timer<FakeTick> timer(50);
  FakeTick::now += 20;
  ASSERT_EQ(timer.Value(), 30);

  const Timer<FakeTick> restored = RoundTrip(timer, 500);
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

  const Timer<FakeTick> restored = RoundTrip(timer, 500);
  EXPECT_TRUE(restored.IsFinished());
  EXPECT_EQ(restored.Value(), 0);
}

TEST_F(TimerSerializeTest, StopwatchKeepsAccumulatedTicks) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 40;
  ASSERT_EQ(watch.Value(), 40);

  const Stopwatch<FakeTick> restored = RoundTrip(watch, 500);
  EXPECT_EQ(restored.Value(), 40);
  EXPECT_TRUE(restored.IsRunning());
  FakeTick::now += 2;
  EXPECT_EQ(restored.Value(), 42);
}

TEST_F(TimerSerializeTest, StoppedStopwatchStaysStopped) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 40;
  watch.Stop();

  const Stopwatch<FakeTick> restored = RoundTrip(watch, 500);
  EXPECT_FALSE(restored.IsRunning());
  FakeTick::now += 100;
  EXPECT_EQ(restored.Value(), 40);
}

TEST_F(TimerSerializeTest, NegativeRemainingValueIsRejected) {
  Timer<FakeTick> timer(50);
  EXPECT_FALSE(LoadsRaw(timer, -5, true));
  EXPECT_EQ(timer.Value(), 50);
  EXPECT_TRUE(timer.IsRunning());
}

TEST_F(TimerSerializeTest, NegativeElapsedValueIsRejected) {
  Stopwatch<FakeTick> watch;
  FakeTick::now += 7;
  EXPECT_FALSE(LoadsRaw(watch, -5, false));
  EXPECT_EQ(watch.Value(), 7);
  EXPECT_TRUE(watch.IsRunning());
}

}  // namespace
