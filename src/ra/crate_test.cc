#include "ra/crate.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "tech/archive.h"
#include "tech/byte_sink.h"
#include "tech/ftimer.h"
#include "tech/span_source.h"

// The game clock is supplied by globals.cc in rasdl.
int64_t Frame = 0;

namespace {

class CrateSink : public ByteSink {
 public:
  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return true;
  }

  std::vector<uint8_t> bytes;
};

bool ReadCrate(CrateClass& crate, const std::vector<uint8_t>& bytes) {
  SpanSource straw(std::as_bytes(std::span(bytes)));
  ArchiveReader reader(straw);
  reader(crate);
  return reader.ok();
}

TEST(CrateSerializeTest, DefaultCrateHasNoCellOrExpiry) {
  CrateClass crate;
  EXPECT_FALSE(crate.Is_Valid());
  EXPECT_FALSE(crate.Is_Expired());
  CrateSink pipe;
  ArchiveWriter writer(pipe);
  writer(crate);
  CrateClass loaded;
  ASSERT_TRUE(ReadCrate(loaded, pipe.bytes));
  EXPECT_FALSE(loaded.Is_Valid());
  EXPECT_FALSE(loaded.Is_Expired());
}

TEST(CrateSerializeTest, CountdownSurvivesSaveAndReanchorsOnLoad) {
  Frame = 100;
  CELL cell = 42;
  Timer<FrameTickSource> timer(30);
  CrateSink fixture;
  ArchiveWriter fixture_writer(fixture);
  fixture_writer(cell, timer);
  CrateClass crate;
  ASSERT_TRUE(ReadCrate(crate, fixture.bytes));
  EXPECT_TRUE(crate.Is_Here(cell));

  Frame += 10;
  CrateSink saved;
  ArchiveWriter writer(saved);
  writer(crate);
  EXPECT_FALSE(crate.Is_Expired());

  Frame = 1000;
  CrateClass loaded;
  ASSERT_TRUE(ReadCrate(loaded, saved.bytes));
  EXPECT_TRUE(loaded.Is_Here(cell));
  Frame += 19;
  EXPECT_FALSE(loaded.Is_Expired());
  ++Frame;
  EXPECT_TRUE(loaded.Is_Expired());
  loaded.Init();
  EXPECT_FALSE(loaded.Is_Valid());
  EXPECT_FALSE(loaded.Is_Expired());
}

TEST(CrateSerializeTest, RejectsCellsOutsideTheMap) {
  for (CELL cell : {static_cast<CELL>(-2), static_cast<CELL>(MAP_CELL_TOTAL)}) {
    CrateSink pipe;
    ArchiveWriter writer(pipe);
    Timer<FrameTickSource> timer(30);
    writer(cell, timer);
    CrateClass crate;
    EXPECT_FALSE(ReadCrate(crate, pipe.bytes));
  }
}

TEST(CrateSerializeTest, RejectsTruncatedTimer) {
  CrateClass crate;
  CrateSink pipe;
  ArchiveWriter writer(pipe);
  writer(crate);
  pipe.bytes.pop_back();
  CrateClass loaded;
  EXPECT_FALSE(ReadCrate(loaded, pipe.bytes));
}

}  // namespace
