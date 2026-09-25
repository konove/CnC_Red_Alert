#include "ra/hall_of_fame.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "base/array.h"
#include "base/buffer.h"
#include "gtest/gtest.h"

namespace {

FameEntry Entry(std::string_view name, int score, int level = 1, int side = 0) {
  FameEntry entry{.name = {}, .score = score, .level = level, .side = side};
  std::ranges::copy(name, entry.name.begin());
  return entry;
}

// A full table scoring 700, 600, ... 100.
FameTable FullTable() {
  FameTable table;
  for (int i = 0; i < kFameRows; i++) {
    table.at(static_cast<std::size_t>(i)) =
        Entry("PLAYER", (kFameRows - i) * 100);
  }
  return table;
}

TEST(InsertFameScoreTest, FirstScoreTakesTheTopOfAnEmptyTable) {
  FameTable table;
  EXPECT_EQ(InsertFameScore(table, 1234, 5, 1), 0);
  EXPECT_EQ(table.at(0), Entry("", 1234, 5, 1));
  EXPECT_EQ(table.at(1), FameEntry{});
}

TEST(InsertFameScoreTest, InsertsBelowBetterScoresAndShiftsTheRestDown) {
  FameTable table = FullTable();
  EXPECT_EQ(InsertFameScore(table, 450, 3, 0), 3);
  EXPECT_EQ(table.at(2).score, 500);
  EXPECT_EQ(table.at(3), Entry("", 450, 3, 0));
  EXPECT_EQ(table.at(4), Entry("PLAYER", 400));
  EXPECT_EQ(table.back().score, 200);
}

TEST(InsertFameScoreTest, BeatingOnlyTheLastRowReplacesIt) {
  FameTable table = FullTable();
  EXPECT_EQ(InsertFameScore(table, 150, 3, 0), kFameRows - 1);
  EXPECT_EQ(table.back(), Entry("", 150, 3, 0));
}

TEST(InsertFameScoreTest, ScoreBelowTheLastRowLeavesTheTableAlone) {
  FameTable table = FullTable();
  EXPECT_EQ(InsertFameScore(table, 50, 3, 0), -1);
  EXPECT_EQ(table, FullTable());
}

TEST(InsertFameScoreTest, TieWithTheLastRowDoesNotEvictIt) {
  FameTable table = FullTable();
  EXPECT_EQ(InsertFameScore(table, 100, 3, 0), -1);
  EXPECT_EQ(table, FullTable());
}

TEST(InsertFameScoreTest, NonPositiveScoreNeverEnters) {
  FameTable table;
  EXPECT_EQ(InsertFameScore(table, 0, 3, 0), -1);
  EXPECT_EQ(InsertFameScore(table, -500, 3, 0), -1);
  EXPECT_EQ(table, FameTable{});
}

TEST(FameTableCodecTest, RoundTrips) {
  FameTable table = FullTable();
  table.at(1) = Entry("TENLETTERS", 650, 14, 1);
  std::array<std::byte, kFameFileSize> raw{};
  EncodeFameTable(table, raw);
  EXPECT_EQ(DecodeFameTable(raw), table);
}

// Reads the native 32-bit integer at `offset` of a record.
int32_t IntAt(std::span<const std::byte> record, std::size_t offset) {
  int32_t value = 0;
  std::ranges::copy(record.subspan(offset, sizeof(value)),
                    base::ObjectBytes(value).begin());
  return value;
}

TEST(FameTableCodecTest, MatchesTheLegacyRecordLayout) {
  FameTable table;
  table.at(1) = Entry("AB", 0x01020304, 7, 1);
  std::array<std::byte, kFameFileSize> raw{};
  raw.fill(std::byte{0xCC});
  EncodeFameTable(table, raw);

  const auto record = std::span(raw).subspan(kFameRecordSize, kFameRecordSize);
  EXPECT_EQ(record.front(), std::byte{'A'});
  EXPECT_EQ(base::At(record, 1), std::byte{'B'});
  // The rest of the name and the padding byte are zero, never leftovers.
  for (const std::byte b : record.subspan(2, 10)) {
    EXPECT_EQ(b, std::byte{0});
  }
  EXPECT_EQ(IntAt(record, 12), 0x01020304);
  EXPECT_EQ(IntAt(record, 16), 7);
  EXPECT_EQ(IntAt(record, 20), 1);
}

TEST(FameTableCodecTest, EncodingDropsBytesAfterTheTerminator) {
  FameTable table;
  table.at(0) = Entry("ABCDEF", 10);
  table.at(0).name.at(2) = '\0';  // Backspaced down to "AB".
  std::array<std::byte, kFameFileSize> raw{};
  EncodeFameTable(table, raw);
  EXPECT_EQ(DecodeFameTable(raw).at(0), Entry("AB", 10));
}

TEST(FameTableCodecTest, DecodingTerminatesAnUnterminatedName) {
  std::array<std::byte, kFameFileSize> raw{};
  raw.fill(std::byte{'X'});
  const FameTable table = DecodeFameTable(raw);
  EXPECT_EQ(std::string_view(table.at(0).name.data()), "XXXXXXXXXX");
}

}  // namespace
