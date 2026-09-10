#include "ra/crate.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "tech/archive.h"
#include "tech/ftimer.h"
#include "tech/pipe.h"
#include "tech/xstraw.h"

// The game clock is supplied by globals.cc in rasdl.
int64_t Frame = 0;

namespace {

class CratePipe : public Pipe {
 public:
  int Put(const void* source, int length) override {
    const auto* begin = static_cast<const uint8_t*>(source);
    bytes.insert(bytes.end(), begin, begin + length);
    return length;
  }

  std::vector<uint8_t> bytes;
};

bool ReadCrate(CrateClass& crate, const std::vector<uint8_t>& bytes) {
  BufferStraw straw(bytes.data(), static_cast<int>(bytes.size()));
  ArchiveReader reader(straw);
  reader(crate);
  return reader.ok();
}

TEST(CrateSerializeTest, DefaultCrateHasNoCellOrExpiry) {
  CrateClass crate;
  EXPECT_FALSE(crate.Is_Valid());
  EXPECT_FALSE(crate.Is_Expired());
  CratePipe pipe;
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
  CratePipe fixture;
  ArchiveWriter fixture_writer(fixture);
  fixture_writer(cell, timer);
  CrateClass crate;
  ASSERT_TRUE(ReadCrate(crate, fixture.bytes));
  EXPECT_TRUE(crate.Is_Here(cell));

  Frame += 10;
  CratePipe saved;
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
    CratePipe pipe;
    ArchiveWriter writer(pipe);
    Timer<FrameTickSource> timer(30);
    writer(cell, timer);
    CrateClass crate;
    EXPECT_FALSE(ReadCrate(crate, pipe.bytes));
  }
}

TEST(CrateSerializeTest, RejectsTruncatedTimer) {
  CrateClass crate;
  CratePipe pipe;
  ArchiveWriter writer(pipe);
  writer(crate);
  pipe.bytes.pop_back();
  CrateClass loaded;
  EXPECT_FALSE(ReadCrate(loaded, pipe.bytes));
}

}  // namespace
