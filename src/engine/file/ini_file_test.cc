// Checks numeric defaults and legacy hexadecimal forms through the INI API.
#include "engine/file/ini_file.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "base/fixed.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"

namespace engine::file {

namespace {

TEST(IniFileTest, InvalidNumbersKeepTheCallersDefault) {
  IniFile ini;
  EXPECT_EQ(ini.GetInt("Values", "Missing", 73), 73);
  for (const char* text : {"invalid", "12tail", "2147483648", "-2147483649",
                           "$100000000", "ffjunkh"}) {
    ASSERT_TRUE(ini.PutString("Values", "Number", text));
    EXPECT_EQ(ini.GetInt("Values", "Number", 73), 73) << text;
  }
}

TEST(IniFileTest, PreservesIntegerAndHexadecimalForms) {
  IniFile ini;
  ASSERT_TRUE(ini.PutString("Values", "Number", "-42"));
  EXPECT_EQ(ini.GetInt("Values", "Number", 73), -42);
  ASSERT_TRUE(ini.PutString("Values", "Number", "$FFFFFFFF"));
  EXPECT_EQ(ini.GetInt("Values", "Number", 73), -1);
  ASSERT_TRUE(ini.PutString("Values", "Number", "10h"));
  EXPECT_EQ(ini.GetInt("Values", "Number", 73), 16);
  ASSERT_TRUE(ini.PutString("Values", "Number", "0x80"));
  EXPECT_EQ(ini.GetHex("Values", "Number", 73), 128);
  ASSERT_TRUE(ini.PutString("Values", "Number", "80garbage"));
  EXPECT_EQ(ini.GetHex("Values", "Number", 73), 73);
}

TEST(IniFileTest, ReplacingAnEntryKeepsTheOthersReachable) {
  IniFile ini;
  for (int i = 0; i < 50; ++i) {
    ASSERT_TRUE(ini.PutInt("S", absl::StrCat("K", i), i));
  }
  ASSERT_TRUE(ini.PutInt("S", "K7", 700));
  ASSERT_TRUE(ini.PutString("S", "K8", ""));  // removes K8
  EXPECT_EQ(ini.EntryCount("S"), 49);
  EXPECT_EQ(ini.GetInt("S", "K7", -1), 700);
  EXPECT_EQ(ini.GetInt("S", "K8", -1), -1);
  EXPECT_EQ(ini.GetInt("S", "K49", -1), 49);
  ASSERT_TRUE(ini.Clear("S"));
  EXPECT_FALSE(ini.HasSection("S"));
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
  EXPECT_GT(ini->GetString("Basic", "PLAYER", "", buf), 0);
  EXPECT_STREQ(buf.data(), "GoodGuy");
  EXPECT_TRUE(ini->HasSection("basic"));
  EXPECT_STREQ(ini->SectionName(0), "BASIC");  // stored spelling is kept
  EXPECT_EQ(ini->SectionName(1), nullptr);
}

TEST(IniFileTest, LongLinesAreNotTruncated) {
  const std::string value(400, 'x');
  const auto ini = Parse("[TeamTypes]\nT1=" + value + "\n");
  std::array<char, 500> buf{};
  EXPECT_EQ(ini->GetString("TeamTypes", "T1", "", buf), 400);
}

TEST(IniFileTest, FirstDuplicateWins) {
  const auto ini = Parse("[S]\nK=1\nK=2\n[S]\nK=3\n");
  EXPECT_EQ(ini->GetInt("S", "K", 0), 1);
}

TEST(IniFileTest, DuplicatesDifferingInCaseAreDuplicates) {
  const auto ini = Parse("[S]\nk=1\nK=2\n");
  EXPECT_EQ(ini->GetInt("S", "K", 0), 1);
  ASSERT_TRUE(ini->PutInt("S", "K", 9));
  EXPECT_EQ(ini->EntryCount("S"), 1);
  EXPECT_EQ(ini->GetInt("S", "k", 0), 9);
}

TEST(IniFileTest, AcceptsBareLineFeeds) {
  const auto ini = Parse("[S]\nA=1\nB=2\n");
  EXPECT_EQ(ini->EntryCount("S"), 2);
}

TEST(IniFileTest, PutReplacesEveryDuplicateOfAnEntry) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\nK=2\n");
  EXPECT_EQ(ini.GetInt("S", "K", 0), 1);
  ASSERT_TRUE(ini.PutInt("S", "K", 9));
  EXPECT_EQ(ini.GetInt("S", "K", 0), 9);
  EXPECT_EQ(ini.EntryCount("S"), 1);

  std::vector<std::byte> saved;
  VectorSink sink(saved);
  ASSERT_TRUE(ini.Save(sink));
  IniFile reloaded;
  SpanSource source(saved);
  ASSERT_TRUE(reloaded.Load(source));
  EXPECT_EQ(reloaded.GetInt("S", "K", 0), 9);
}

TEST(IniFileTest, ClearingAnEntryRemovesItsDuplicates) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\nK=2\nL=3\n");
  ASSERT_TRUE(ini.Clear("S", "K"));
  EXPECT_FALSE(ini.HasEntry("S", "K"));
  EXPECT_EQ(ini.GetInt("S", "L", 0), 3);
  EXPECT_EQ(ini.EntryCount("S"), 1);
}

TEST(IniFileTest, ClearingASectionRemovesItsDuplicates) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\n[T]\nM=5\n[S]\nK=3\n");
  ASSERT_TRUE(ini.Clear("S"));
  EXPECT_FALSE(ini.HasSection("S"));
  EXPECT_EQ(ini.SectionCount(), 1);
  EXPECT_EQ(ini.GetInt("T", "M", 0), 5);
}

TEST(IniFileTest, UniqueIdFollowsEdits) {
  IniFile ini;
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
  const int before = ini.UniqueId();
  ASSERT_TRUE(ini.PutInt("S", "K", 2));
  EXPECT_NE(ini.UniqueId(), before);
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
  EXPECT_EQ(ini.UniqueId(), before);
}

TEST(IniFileTest, UniqueIdFollowsRemovals) {
  IniFile ini;
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
  ASSERT_TRUE(ini.PutInt("S", "L", 2));
  const int both = ini.UniqueId();
  ASSERT_TRUE(ini.Clear("S", "L"));
  EXPECT_NE(ini.UniqueId(), both);
  ASSERT_TRUE(ini.Clear("S"));
  const int empty = IniFile().UniqueId();
  EXPECT_EQ(ini.UniqueId(), empty);
}

TEST(IniFileTest, DigestRoundTripsAndDetectsEdits) {
  IniFile ini;
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  ASSERT_TRUE(ini.SaveWithDigest(sink));
  EXPECT_FALSE(ini.HasSection("Digest"));

  IniFile same;
  SpanSource source(bytes);
  EXPECT_EQ(same.LoadVerified(source), IniLoadResult::kLoaded);
  EXPECT_FALSE(same.HasSection("Digest"));

  std::string text;
  for (const std::byte byte : bytes) {
    text.push_back(static_cast<char>(byte));
  }
  text.replace(text.find("K=1"), 3, "K=2");
  IniFile edited;
  SpanSource edited_source(std::as_bytes(std::span(text)));
  EXPECT_EQ(edited.LoadVerified(edited_source), IniLoadResult::kDigestMismatch);
  EXPECT_EQ(edited.GetInt("S", "K", 0), 2);  // still loaded
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
  EXPECT_EQ(loaded.GetInt("S", "K", 0), 1);
}

TEST(IniFileTest, ClearWithoutANameRemovesEverything) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\n[T]\nM=5\n");
  const int loaded = ini.UniqueId();
  EXPECT_TRUE(ini.is_loaded());
  ASSERT_TRUE(ini.Clear());
  EXPECT_FALSE(ini.is_loaded());
  EXPECT_EQ(ini.SectionCount(), 0);
  EXPECT_FALSE(ini.HasSection("T"));
  EXPECT_NE(ini.UniqueId(), loaded);
  EXPECT_EQ(ini.UniqueId(), IniFile().UniqueId());
}

TEST(IniFileTest, ClearingAMissingNameRemovesNothing) {
  IniFile ini;
  LoadText(ini, "[S]\nK=1\n");
  ASSERT_TRUE(ini.Clear("Missing"));
  ASSERT_TRUE(ini.Clear("S", "Missing"));
  ASSERT_TRUE(ini.Clear("Missing", "K"));
  // An empty name is a name like any other, not "everything".
  ASSERT_TRUE(ini.Clear(""));
  ASSERT_TRUE(ini.Clear("S", ""));
  EXPECT_EQ(ini.SectionCount(), 1);
  EXPECT_EQ(ini.GetInt("S", "K", 0), 1);
}

TEST(IniFileTest, HasSectionAndHasEntryFindNamesWithoutCase) {
  const auto ini = Parse("[Basic]\nPlayer=GoodGuy\n");
  EXPECT_TRUE(ini->HasSection("BASIC"));
  EXPECT_FALSE(ini->HasSection("Player"));
  EXPECT_FALSE(ini->HasSection(""));
  EXPECT_TRUE(ini->HasEntry("basic", "PLAYER"));
  EXPECT_FALSE(ini->HasEntry("Basic", "Basic"));
  EXPECT_FALSE(ini->HasEntry("Missing", "Player"));
  EXPECT_FALSE(ini->HasEntry("Basic", ""));
}

TEST(IniFileTest, EntryNamesComeInFileOrder) {
  const auto ini = Parse("[S]\nB=1\nA=2\n");
  ASSERT_EQ(ini->EntryCount("s"), 2);
  EXPECT_STREQ(ini->EntryName("S", 0), "B");
  EXPECT_STREQ(ini->EntryName("S", 1), "A");
  EXPECT_EQ(ini->EntryName("S", 2), nullptr);
  EXPECT_EQ(ini->EntryName("S", -1), nullptr);
  EXPECT_EQ(ini->EntryName("Missing", 0), nullptr);
  EXPECT_EQ(ini->EntryCount("Missing"), 0);
}

TEST(IniFileTest, GetStringReturnsTheValueOrTheDefault) {
  IniFile ini;
  ASSERT_TRUE(ini.PutString("S", "K", "  padded value "));
  EXPECT_EQ(ini.GetString("S", "K", "default"), "padded value");
  EXPECT_EQ(ini.GetString("S", "Missing", "default"), "default");
  EXPECT_EQ(ini.GetString("Missing", "K", " default "), "default");
  EXPECT_EQ(ini.GetString("S", "Missing", ""), "");
}

TEST(IniFileTest, GetStringIntoABufferCutsAndReturnsTheLength) {
  IniFile ini;
  ASSERT_TRUE(ini.PutString("S", "K", " value "));
  std::array<char, 16> buffer{};
  EXPECT_EQ(ini.GetString("S", "K", "default", buffer), 5);
  EXPECT_STREQ(buffer.data(), "value");
  EXPECT_EQ(ini.GetString("S", "Missing", "default", buffer), 7);
  EXPECT_STREQ(buffer.data(), "default");

  // The terminator takes one char of the buffer, and blanks left at the cut
  // are trimmed.
  ASSERT_TRUE(ini.PutString("S", "Long", "abc  defgh"));
  EXPECT_EQ(ini.GetString("S", "Long", "", std::span(buffer).first(6)), 3);
  EXPECT_STREQ(buffer.data(), "abc");

  EXPECT_EQ(ini.GetString("S", "K", "default", std::span<char>()), 0);
}

TEST(IniFileTest, GetStringAcceptsItsOwnBufferAsTheDefault) {
  IniFile ini;
  ASSERT_TRUE(ini.PutString("S", "K", "stored"));
  std::array<char, 16> buffer{"previous"};
  EXPECT_EQ(ini.GetString("S", "Missing", buffer.data(), buffer), 8);
  EXPECT_STREQ(buffer.data(), "previous");
  EXPECT_EQ(ini.GetString("S", "K", buffer.data(), buffer), 6);
  EXPECT_STREQ(buffer.data(), "stored");
}

TEST(IniFileTest, PutStringWithAnEmptyValueRemovesTheEntry) {
  IniFile ini;
  ASSERT_TRUE(ini.PutString("S", "K", "1"));
  ASSERT_TRUE(ini.PutString("S", "L", "2"));
  ASSERT_TRUE(ini.PutString("S", "K", ""));
  EXPECT_FALSE(ini.HasEntry("S", "K"));
  EXPECT_TRUE(ini.HasEntry("S", "L"));
  EXPECT_EQ(ini.EntryCount("S"), 1);
  // Nothing to remove: the section is still created, and stays empty.
  ASSERT_TRUE(ini.PutString("T", "K", ""));
  EXPECT_TRUE(ini.HasSection("T"));
  EXPECT_EQ(ini.EntryCount("T"), 0);
}

TEST(IniFileTest, TypedValuesRoundTrip) {
  IniFile ini;
  ASSERT_TRUE(ini.PutBool("S", "Yes", true));
  ASSERT_TRUE(ini.PutBool("S", "No", false));
  ASSERT_TRUE(ini.PutHex("S", "Hex", 255));
  ASSERT_TRUE(ini.PutInt("S", "TrailingH", 255, 1));
  ASSERT_TRUE(ini.PutInt("S", "Dollar", 255, 2));
  ASSERT_TRUE(ini.PutFixed("S", "Fixed", Fixed::FromString("1.5")));
  EXPECT_EQ(ini.GetString("S", "Yes", ""), "yes");
  EXPECT_EQ(ini.GetString("S", "No", ""), "no");
  EXPECT_TRUE(ini.GetBool("S", "Yes", false));
  EXPECT_FALSE(ini.GetBool("S", "No", true));
  // Only the first character counts, and one that is neither keeps the
  // default.
  EXPECT_FALSE(ini.GetBool("S", "Hex", true));    // "FF"
  EXPECT_TRUE(ini.GetBool("S", "Dollar", true));  // "$FF"
  EXPECT_TRUE(ini.GetBool("S", "Missing", true));
  EXPECT_EQ(ini.GetString("S", "Hex", ""), "FF");
  EXPECT_EQ(ini.GetHex("S", "Hex", 0), 255);
  EXPECT_EQ(ini.GetString("S", "TrailingH", ""), "FFh");
  EXPECT_EQ(ini.GetInt("S", "TrailingH", 0), 255);
  EXPECT_EQ(ini.GetString("S", "Dollar", ""), "$FF");
  EXPECT_EQ(ini.GetInt("S", "Dollar", 0), 255);
  EXPECT_EQ(ini.GetFixed("S", "Fixed", Fixed()), Fixed::FromString("1.5"));
  EXPECT_EQ(ini.GetFixed("S", "Missing", Fixed::FromString("75%")),
            Fixed::FromString("75%"));
}

TEST(IniFileTest, TextBlockBreaksAtBlanksAndJoinsWithSpaces) {
  // 12 words of 9 letters: 119 characters, so two lines.
  std::string text;
  for (int i = 0; i < 12; ++i) {
    absl::StrAppend(&text, i == 0 ? "" : " ", "word", 10000 + i);
  }
  IniFile ini;
  ASSERT_TRUE(ini.PutString("Briefing", "Stale", "x"));
  ASSERT_TRUE(ini.PutTextBlock("Briefing", text));
  ASSERT_EQ(ini.EntryCount("Briefing"), 2);
  EXPECT_FALSE(ini.HasEntry("Briefing", "Stale"));
  EXPECT_EQ(ini.GetString("Briefing", "1", ""), text.substr(0, 69));
  EXPECT_EQ(ini.GetString("Briefing", "2", ""), text.substr(70));

  std::array<char, 256> buffer{};
  EXPECT_EQ(ini.GetTextBlock("Briefing", buffer), std::ssize(text));
  EXPECT_EQ(std::string_view(buffer.data()), text);

  // A short buffer keeps what fits, terminated.
  EXPECT_EQ(ini.GetTextBlock("Briefing", std::span(buffer).first(11)), 10);
  EXPECT_EQ(std::string_view(buffer.data()), text.substr(0, 10));
  EXPECT_EQ(ini.GetTextBlock("Missing", buffer), 0);
  EXPECT_STREQ(buffer.data(), "");
}

TEST(IniFileTest, Base64BlockRoundTrips) {
  std::array<std::byte, 100> block{};
  for (std::size_t i = 0; i < block.size(); ++i) {
    block.at(i) = static_cast<std::byte>(i * 7);
  }
  IniFile ini;
  ASSERT_TRUE(ini.PutBase64Block("Pack", block));
  // 100 bytes are 136 base64 characters, 70 to an entry.
  EXPECT_EQ(ini.EntryCount("Pack"), 2);
  EXPECT_EQ(ini.GetString("Pack", "1", "").size(), 70U);

  std::array<std::byte, 128> decoded{};
  ASSERT_EQ(ini.GetBase64Block("Pack", decoded), 100);
  EXPECT_TRUE(std::ranges::equal(std::span(decoded).first(100), block));
  EXPECT_EQ(ini.GetBase64Block("Missing", decoded), 0);

  // An empty block is refused and leaves the section as it was.
  EXPECT_FALSE(ini.PutBase64Block("Pack", {}));
  EXPECT_EQ(ini.EntryCount("Pack"), 2);
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
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
  RecordingSink sink;
  ASSERT_TRUE(ini.SaveWithDigest(sink));
  EXPECT_EQ(sink.finishes_, 1);
  EXPECT_EQ(sink.writes_after_finish_, 0);
  EXPECT_FALSE(sink.bytes_.empty());
}

// The wire format and the unique ID of "[S] K=1" are pinned: the text was
// produced by SaveWithDigest and the ID by UniqueId() before the parser
// and writer changed.
constexpr std::string_view kGoldenText =
    "[S]\r\nK=1\r\n\r\n[Digest]\r\n1=0/u5ncnzgsS6A6G73V7Pib2M4ew=\r\n\r\n";
constexpr int kGoldenUniqueId = -289688347;

TEST(IniFileTest, GoldenDigestTextVerifies) {
  IniFile ini;
  SpanSource source(std::as_bytes(std::span(kGoldenText)));
  EXPECT_EQ(ini.LoadVerified(source), IniLoadResult::kLoaded);
  EXPECT_FALSE(ini.HasSection("Digest"));
  EXPECT_EQ(ini.GetInt("S", "K", 0), 1);
  EXPECT_EQ(ini.UniqueId(), kGoldenUniqueId);
}

TEST(IniFileTest, SaveWithDigestMatchesTheGoldenText) {
  IniFile ini;
  ASSERT_TRUE(ini.PutInt("S", "K", 1));
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
