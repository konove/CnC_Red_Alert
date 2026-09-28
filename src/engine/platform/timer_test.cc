#include "engine/platform/timer.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/platform/sleep.h"
#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"

namespace {

TEST(SystemClockTest, NeverRunsBackward) {
  int64_t previous = SystemTicks();
  for (int i = 0; i < 1000; ++i) {
    const int64_t now = SystemTicks();
    EXPECT_GE(now, previous);
    previous = now;
  }
}

TEST(SystemClockTest, CountsSixtyTicksPerSecond) {
  // 100 ms is 6 ticks; flooring both readings can lose one. Only a lower bound
  // is checked, since a loaded machine may sleep much longer.
  const int64_t start = SystemTicks();
  engine::platform::SleepMs(100);
  EXPECT_GE(SystemTicks() - start, 5);
}

TEST(SystemClockTest, MillisecondsNeverRunBackward) {
  int64_t previous = SystemMilliseconds();
  for (int i = 0; i < 1000; ++i) {
    const int64_t now = SystemMilliseconds();
    EXPECT_GE(now, previous);
    previous = now;
  }
}

TEST(SystemClockTest, CountsMilliseconds) {
  // Only a lower bound is checked, since a loaded machine may sleep longer.
  const int64_t start = SystemMilliseconds();
  engine::platform::SleepMs(100);
  EXPECT_GE(SystemMilliseconds() - start, 100);
}

// A tick source the test advances by hand.
struct FakeTickSource {
  static int64_t now;
  static int64_t Tick() { return now; }
};
int64_t FakeTickSource::now = 0;

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

  FakeTickSource::now += skew;
  SpanSource source(std::as_bytes(std::span(sink.bytes)));
  ArchiveReader reader(source);
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
  SpanSource source(std::as_bytes(std::span(sink.bytes)));
  ArchiveReader reader(source);
  subject.Serialize(reader);
  return reader.ok();
}

class TimerTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTickSource::now = 1000; }
};

TEST_F(TimerTest, CountsDownAndStopsAtZero) {
  const Timer<FakeTickSource> timer(30);
  EXPECT_TRUE(timer.IsRunning());
  EXPECT_TRUE(timer.HasTimeLeft());
  FakeTickSource::now += 10;
  EXPECT_EQ(timer.Remaining(), 20);
  FakeTickSource::now += 25;
  EXPECT_EQ(timer.Remaining(), 0);
  EXPECT_TRUE(timer.IsFinished());
  EXPECT_TRUE(timer.IsRunning());
}

TEST_F(TimerTest, StopFreezesAndStartResumes) {
  Timer<FakeTickSource> timer(50);
  FakeTickSource::now += 10;
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  FakeTickSource::now += 100;
  EXPECT_EQ(timer.Remaining(), 40);

  timer.Start();
  FakeTickSource::now += 5;
  EXPECT_EQ(timer.Remaining(), 35);
}

TEST_F(TimerTest, SetWhileStoppedRestartsTheCountdown) {
  Timer<FakeTickSource> timer(50);
  timer.Stop();
  timer.Set(20);
  EXPECT_TRUE(timer.IsRunning());
  FakeTickSource::now += 5;
  EXPECT_EQ(timer.Remaining(), 15);
}

TEST_F(TimerTest, StoppedAtZeroReadsFinished) {
  Timer<FakeTickSource> timer(50);
  timer.Set(0);
  timer.Stop();
  EXPECT_FALSE(timer.IsRunning());
  EXPECT_TRUE(timer.IsFinished());
  EXPECT_EQ(timer.Remaining(), 0);
}

TEST_F(TimerTest, StopwatchPausesAndResumes) {
  Stopwatch<FakeTickSource> watch;
  FakeTickSource::now += 10;
  watch.Stop();
  FakeTickSource::now += 100;
  EXPECT_EQ(watch.Elapsed(), 10);

  watch.Start();
  FakeTickSource::now += 5;
  EXPECT_EQ(watch.Elapsed(), 15);
}

TEST_F(TimerTest, StopwatchResetZeroesAndRuns) {
  Stopwatch<FakeTickSource> watch;
  FakeTickSource::now += 10;
  watch.Stop();
  watch.Reset();
  EXPECT_TRUE(watch.IsRunning());
  EXPECT_EQ(watch.Elapsed(), 0);
  FakeTickSource::now += 3;
  EXPECT_EQ(watch.Elapsed(), 3);
}

class TimerSerializeTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTickSource::now = 1000; }
};

TEST_F(TimerSerializeTest, RunningTimerKeepsItsRemainingValue) {
  Timer<FakeTickSource> timer(50);
  FakeTickSource::now += 20;
  ASSERT_EQ(timer.Remaining(), 30);

  const Timer<FakeTickSource> restored = RoundTrip(timer, 500);
  EXPECT_EQ(restored.Remaining(), 30);
  EXPECT_TRUE(restored.IsRunning());

  FakeTickSource::now += 10;
  EXPECT_EQ(restored.Remaining(), 20);
}

TEST_F(TimerSerializeTest, StoppedTimerStaysStopped) {
  Timer<FakeTickSource> timer(50);
  FakeTickSource::now += 5;
  timer.Stop();

  Timer<FakeTickSource> restored = RoundTrip(timer, 500);
  EXPECT_FALSE(restored.IsRunning());
  EXPECT_EQ(restored.Remaining(), 45);
  FakeTickSource::now += 100;
  EXPECT_EQ(restored.Remaining(), 45);

  restored.Start();
  FakeTickSource::now += 5;
  EXPECT_EQ(restored.Remaining(), 40);
}

TEST_F(TimerSerializeTest, FinishedTimerStaysFinished) {
  Timer<FakeTickSource> timer(10);
  FakeTickSource::now += 100;
  ASSERT_TRUE(timer.IsFinished());

  const Timer<FakeTickSource> restored = RoundTrip(timer, 500);
  EXPECT_TRUE(restored.IsFinished());
  EXPECT_EQ(restored.Remaining(), 0);
}

TEST_F(TimerSerializeTest, StopwatchKeepsAccumulatedTicks) {
  Stopwatch<FakeTickSource> watch;
  FakeTickSource::now += 40;
  ASSERT_EQ(watch.Elapsed(), 40);

  const Stopwatch<FakeTickSource> restored = RoundTrip(watch, 500);
  EXPECT_EQ(restored.Elapsed(), 40);
  EXPECT_TRUE(restored.IsRunning());
  FakeTickSource::now += 2;
  EXPECT_EQ(restored.Elapsed(), 42);
}

TEST_F(TimerSerializeTest, StoppedStopwatchStaysStopped) {
  Stopwatch<FakeTickSource> watch;
  FakeTickSource::now += 40;
  watch.Stop();

  const Stopwatch<FakeTickSource> restored = RoundTrip(watch, 500);
  EXPECT_FALSE(restored.IsRunning());
  FakeTickSource::now += 100;
  EXPECT_EQ(restored.Elapsed(), 40);
}

TEST_F(TimerSerializeTest, NegativeRemainingValueIsRejected) {
  Timer<FakeTickSource> timer(50);
  EXPECT_FALSE(LoadsRaw(timer, -5, true));
  EXPECT_EQ(timer.Remaining(), 50);
  EXPECT_TRUE(timer.IsRunning());
}

TEST_F(TimerSerializeTest, NegativeElapsedValueIsRejected) {
  Stopwatch<FakeTickSource> watch;
  FakeTickSource::now += 7;
  EXPECT_FALSE(LoadsRaw(watch, -5, false));
  EXPECT_EQ(watch.Elapsed(), 7);
  EXPECT_TRUE(watch.IsRunning());
}

}  // namespace
