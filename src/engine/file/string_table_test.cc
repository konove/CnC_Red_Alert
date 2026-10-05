#include "engine/file/string_table.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "engine/file/string_table_test_util.h"
#include "gtest/gtest.h"

namespace {

TEST(StringTableTest, ServesEveryEntryOfATableMadeInMemory) {
  const std::vector<std::string> strings = {"OK", "", "Cancel"};
  const std::vector<std::byte> table = MakeStringTable(strings);

  EXPECT_EQ(Extract_String(table, 0), "OK");
  EXPECT_EQ(Extract_String(table, 1), "");
  EXPECT_EQ(Extract_String(table, 2), "Cancel");
}

// An empty entry is still a string: callers hand data() to code that expects
// a C string, and only a missing entry may be null.
TEST(StringTableTest, AnEmptyEntryIsNotAMissingOne) {
  const std::vector<std::string> strings = {"", "x"};
  const std::vector<std::byte> table = MakeStringTable(strings);

  EXPECT_NE(Extract_String(table, 0).data(), nullptr);
  EXPECT_EQ(Extract_String(table, 2).data(), nullptr);
}

TEST(StringTableTest, FindsNothingOutsideTheTable) {
  const std::vector<std::string> strings = {"OK"};
  const std::vector<std::byte> table = MakeStringTable(strings);

  EXPECT_TRUE(Extract_String(table, 1).empty());
  EXPECT_TRUE(Extract_String(table, -1).empty());
  EXPECT_TRUE(Extract_String({}, 0).empty());
}

TEST(StringTableTest, ATableOfNoStringsIsEmpty) {
  EXPECT_TRUE(MakeStringTable({}).empty());
}

TEST(StringTableTest, AStringStopsAtItsFirstNul) {
  const std::vector<std::string> strings = {std::string("ab\0cd", 5), "next"};
  const std::vector<std::byte> table = MakeStringTable(strings);

  EXPECT_EQ(Extract_String(table, 0), "ab");
  EXPECT_EQ(Extract_String(table, 1), "next");
}

}  // namespace
