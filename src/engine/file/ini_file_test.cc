// Checks numeric defaults and legacy hexadecimal forms through the INI API.
#include "engine/file/ini_file.h"

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "engine/stream/byte_sink.h"
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

std::unique_ptr<IniFile> Parse(std::string_view text) {
  auto ini = std::make_unique<IniFile>();
  SpanSource source(std::as_bytes(std::span(text)));
  CHECK(ini->Load(source));
  return ini;
}

TEST(IniFileTest, NamesCompareWithoutCase) {
  const auto ini = Parse("[BASIC]\r\nPlayer=GoodGuy\r\n");
  std::array<char, 32> buf{};
  EXPECT_GT(ini->Get_String("Basic", "PLAYER", "", buf, 32), 0);
  EXPECT_STREQ(buf.data(), "GoodGuy");
  EXPECT_TRUE(ini->Section_Present("basic"));
  EXPECT_STREQ(ini->Get_Section(0), "BASIC");  // stored spelling is kept
  EXPECT_EQ(ini->Get_Section(1), nullptr);
}

TEST(IniFileTest, LongLinesAreNotTruncated) {
  const std::string value(400, 'x');
  const auto ini = Parse("[TeamTypes]\nT1=" + value + "\n");
  std::array<char, 500> buf{};
  EXPECT_EQ(ini->Get_String("TeamTypes", "T1", "", buf, 500), 400);
}

TEST(IniFileTest, FirstDuplicateWins) {
  const auto ini = Parse("[S]\nK=1\nK=2\n[S]\nK=3\n");
  EXPECT_EQ(ini->Get_Int("S", "K", 0), 1);
}

TEST(IniFileTest, DuplicatesDifferingInCaseAreDuplicates) {
  const auto ini = Parse("[S]\nk=1\nK=2\n");
  EXPECT_EQ(ini->Get_Int("S", "K", 0), 1);
  ASSERT_TRUE(ini->Put_Int("S", "K", 9));
  EXPECT_EQ(ini->Entry_Count("S"), 1);
  EXPECT_EQ(ini->Get_Int("S", "k", 0), 9);
}

TEST(IniFileTest, AcceptsBareLineFeeds) {
  const auto ini = Parse("[S]\nA=1\nB=2\n");
  EXPECT_EQ(ini->Entry_Count("S"), 2);
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

// Records the order of calls reaching the end of a chain.
class RecordingSink : public ByteSink {
 public:
  RecordingSink() = default;
  ~RecordingSink() override;
  RecordingSink(const RecordingSink&) = delete;
  RecordingSink& operator=(const RecordingSink&) = delete;
  RecordingSink(RecordingSink&&) = delete;
  RecordingSink& operator=(RecordingSink&&) = delete;
  bool Write(std::span<const std::byte> bytes) override {
    if (finishes_ > 0) {
      ++writes_after_finish_;
    }
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return true;
  }
  bool Finish() override {
    ++finishes_;
    return true;
  }
  int finishes_ = 0;
  int writes_after_finish_ = 0;
  std::vector<std::byte> bytes_;
};

RecordingSink::~RecordingSink() = default;

TEST(IniFileTest, SaveWithDigestFinishesTheSinkOnce) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  RecordingSink sink;
  ASSERT_TRUE(ini.SaveWithDigest(sink));
  EXPECT_EQ(sink.finishes_, 1);
  EXPECT_EQ(sink.writes_after_finish_, 0);
  EXPECT_FALSE(sink.bytes_.empty());
}

// The wire format and the unique ID of "[S] K=1" are pinned: the text was
// produced by SaveWithDigest and the ID by Get_Unique_ID() before the parser
// and writer changed.
constexpr std::string_view kGoldenText =
    "[S]\r\nK=1\r\n\r\n[Digest]\r\n1=0/u5ncnzgsS6A6G73V7Pib2M4ew=\r\n\r\n";
constexpr int kGoldenUniqueId = -289688347;

TEST(IniFileTest, GoldenDigestTextVerifies) {
  IniFile ini;
  SpanSource source(std::as_bytes(std::span(kGoldenText)));
  EXPECT_EQ(ini.LoadVerified(source), IniLoadResult::kLoaded);
  EXPECT_FALSE(ini.Section_Present("Digest"));
  EXPECT_EQ(ini.Get_Int("S", "K", 0), 1);
  EXPECT_EQ(ini.Get_Unique_ID(), kGoldenUniqueId);
}

TEST(IniFileTest, SaveWithDigestMatchesTheGoldenText) {
  IniFile ini;
  ASSERT_TRUE(ini.Put_Int("S", "K", 1));
  RecordingSink sink;
  ASSERT_TRUE(ini.SaveWithDigest(sink));
  std::string text;
  for (const std::byte byte : sink.bytes_) {
    text.push_back(static_cast<char>(byte));
  }
  EXPECT_EQ(text, kGoldenText);
}

}  // namespace

}  // namespace engine::file
