// Checks numeric defaults and legacy hexadecimal forms through the INI API.
#include "engine/file/ini_file.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_cat.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"

namespace engine::file {

namespace {

TEST(IniFileTest, InvalidNumbersKeepTheCallersDefault) {
  IniFile ini;
  EXPECT_EQ(ini.Get_Int("Values", "Missing", 73), 73);
  for (const char* text : {"invalid", "12tail", "2147483648", "-2147483649",
                           "$100000000", "ffjunkh"}) {
    ASSERT_TRUE(ini.Put_String("Values", "Number", text));
    EXPECT_EQ(ini.Get_Int("Values", "Number", 73), 73) << text;
  }
}

TEST(IniFileTest, PreservesIntegerAndHexadecimalForms) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_String("Values", "Number", "-42"));
  EXPECT_EQ(ini.Get_Int("Values", "Number", 73), -42);
  ASSERT_TRUE(ini.Put_String("Values", "Number", "$FFFFFFFF"));
  EXPECT_EQ(ini.Get_Int("Values", "Number", 73), -1);
  ASSERT_TRUE(ini.Put_String("Values", "Number", "10h"));
  EXPECT_EQ(ini.Get_Int("Values", "Number", 73), 16);
  ASSERT_TRUE(ini.Put_String("Values", "Number", "0x80"));
  EXPECT_EQ(ini.Get_Hex("Values", "Number", 73), 128);
  ASSERT_TRUE(ini.Put_String("Values", "Number", "80garbage"));
  EXPECT_EQ(ini.Get_Hex("Values", "Number", 73), 73);
}

TEST(IniFileTest, ReplacingAnEntryKeepsTheOthersReachable) {
  IniFile ini;
  for (int i = 0; i < 50; ++i) {
    ASSERT_TRUE(ini.Put_Int("S", absl::StrCat("K", i).c_str(), i));
  }
  ASSERT_TRUE(ini.Put_Int("S", "K7", 700));
  ASSERT_TRUE(ini.Put_String("S", "K8", nullptr));  // removes K8
  EXPECT_EQ(ini.Entry_Count("S"), 49);
  EXPECT_EQ(ini.Get_Int("S", "K7", -1), 700);
  EXPECT_EQ(ini.Get_Int("S", "K8", -1), -1);
  EXPECT_EQ(ini.Get_Int("S", "K49", -1), 49);
  ASSERT_TRUE(ini.Clear("S"));
  EXPECT_FALSE(ini.Section_Present("S"));
}

// Loads INI text the way a file would be read, duplicates and all.
void LoadText(IniFile& ini, std::string_view text) {
  SpanSource source(std::as_bytes(std::span(text)));
  ASSERT_TRUE(ini.Load(source));
}

TEST(IniFileTest, PutReplacesEveryDuplicateOfAnEntry) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\nK=2\n");
  EXPECT_EQ(ini.Get_Int("S", "K", 0), 1);
  ASSERT_TRUE(ini.Put_Int("S", "K", 9));
  EXPECT_EQ(ini.Get_Int("S", "K", 0), 9);
  EXPECT_EQ(ini.Entry_Count("S"), 1);

  std::vector<std::byte> saved;
  VectorSink sink(saved);
  ASSERT_TRUE(ini.Save(sink));
  IniFile reloaded;
  SpanSource source(saved);
  ASSERT_TRUE(reloaded.Load(source));
  EXPECT_EQ(reloaded.Get_Int("S", "K", 0), 9);
}

TEST(IniFileTest, ClearingAnEntryRemovesItsDuplicates) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\nK=2\nL=3\n");
  ASSERT_TRUE(ini.Clear("S", "K"));
  EXPECT_FALSE(ini.Is_Present("S", "K"));
  EXPECT_EQ(ini.Get_Int("S", "L", 0), 3);
  EXPECT_EQ(ini.Entry_Count("S"), 1);
}

TEST(IniFileTest, ClearingASectionRemovesItsDuplicates) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\n[T]\nM=5\n[S]\nK=3\n");
  ASSERT_TRUE(ini.Clear("S"));
  EXPECT_FALSE(ini.Section_Present("S"));
  EXPECT_EQ(ini.Section_Count(), 1);
  EXPECT_EQ(ini.Get_Int("T", "M", 0), 5);
}

TEST(IniFileTest, UniqueIdFollowsEdits) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  const int before = ini.Get_Unique_ID();
  ASSERT_TRUE(ini.Put_Int("S", "K", 2));
  EXPECT_NE(ini.Get_Unique_ID(), before);
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  EXPECT_EQ(ini.Get_Unique_ID(), before);
}

TEST(IniFileTest, UniqueIdFollowsRemovals) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  ASSERT_TRUE(ini.Put_Int("S", "L", 2));
  const int both = ini.Get_Unique_ID();
  ASSERT_TRUE(ini.Clear("S", "L"));
  EXPECT_NE(ini.Get_Unique_ID(), both);
  ASSERT_TRUE(ini.Clear("S"));
  const int empty = IniFile().Get_Unique_ID();
  EXPECT_EQ(ini.Get_Unique_ID(), empty);
}

TEST(IniFileTest, DigestRoundTripsAndDetectsEdits) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  ASSERT_TRUE(ini.SaveWithDigest(sink));
  EXPECT_FALSE(ini.Section_Present("Digest"));

  IniFile same;
  SpanSource source(bytes);
  EXPECT_EQ(same.LoadVerified(source), IniLoadResult::kLoaded);
  EXPECT_FALSE(same.Section_Present("Digest"));

  std::string text;
  for (const std::byte byte : bytes) {
    text.push_back(static_cast<char>(byte));
  }
  text.replace(text.find("K=1"), 3, "K=2");
  IniFile edited;
  SpanSource edited_source(std::as_bytes(std::span(text)));
  EXPECT_EQ(edited.LoadVerified(edited_source), IniLoadResult::kDigestMismatch);
  EXPECT_EQ(edited.Get_Int("S", "K", 0), 2);  // still loaded
}

TEST(IniFileTest, LoadVerifiedAcceptsDataWithoutADigest) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\n");
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  ASSERT_TRUE(ini.Save(sink));
  IniFile loaded;
  SpanSource source(bytes);
  EXPECT_EQ(loaded.LoadVerified(source), IniLoadResult::kLoaded);
  EXPECT_EQ(loaded.Get_Int("S", "K", 0), 1);
}

}  // namespace

}  // namespace engine::file
