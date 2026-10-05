// File: MakeStringTable(), a text table made in memory for tests that need
// the game's strings without its archives.

#ifndef CNC_RED_ALERT_ENGINE_FILE_STRING_TABLE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_FILE_STRING_TABLE_TEST_UTIL_H_

#include <cstddef>
#include <span>
#include <string>
#include <vector>

// Returns a text table, in the format Extract_String() reads, whose entry i
// is strings[i]. The offsets are 16 bits wide, so the table, two bytes an
// entry plus every string and its terminator, must stay under 64 KB. A
// string is stored up to its first NUL.
//
// Example:
//   const std::vector<std::string> strings = {"OK", "Cancel"};
//   const std::vector<std::byte> table = MakeStringTable(strings);
//   EXPECT_EQ(Extract_String(table, 1), "Cancel");
std::vector<std::byte> MakeStringTable(std::span<const std::string> strings);

#endif  // CNC_RED_ALERT_ENGINE_FILE_STRING_TABLE_TEST_UTIL_H_
