// Tests that the multiplayer settings CONQUER.INI keeps read back as they were
// written, and that saving them leaves the rest of the file alone.

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/installed.h"
#include "base/strings/safe_string.h"
#include "engine/file/ini_file.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"
#include "td/ai/special.h"
#include "td/core/defines.h"
#include "td/data/ini_io.h"
#include "td/debug/debug_state.h"
#include "td/net/mplayer.h"
#include "td/net/network.h"
#include "td/net/phone.h"
#include "td/net/session.h"

namespace td {

namespace {

using engine::file::IniFile;

// Fills `reloaded` from the text Save() writes for `ini`, which is what the
// next run of the game reads.
void SaveAndReload(const IniFile& ini, IniFile& reloaded) {
  std::vector<std::byte> saved;
  VectorSink sink(saved);
  ASSERT_TRUE(ini.Save(sink));
  SpanSource source(saved);
  ASSERT_TRUE(reloaded.Load(source));
}

// The settings live in the session, the network, the special flags and the
// debug state, so each test installs its own and leaves the shared ones alone.
class MultiPlayerSettingsTest : public testing::Test {
 protected:
  static void AddInitString(const std::string_view text) {
    auto entry = std::make_unique<char[]>(INITSTRBUF_MAX);
    // The allocation above owns exactly INITSTRBUF_MAX characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    base::SafeCopy(std::span(entry.get(), INITSTRBUF_MAX), text);
    TheNetwork().init_strings().push_back(std::move(entry));
  }

  static PhoneEntryClass& AddPhone(const std::string_view name,
                                   const std::string_view number) {
    auto phone = std::make_unique<PhoneEntryClass>();
    base::SafeCopy(phone->Name, name);
    base::SafeCopy(phone->Number, number);
    return *TheNetwork().phone_book().emplace_back(std::move(phone));
  }

  static std::string_view InitString(const int index) {
    return TheNetwork()
        .init_strings()
        .at(static_cast<std::size_t>(index))
        .get();
  }

 private:
  SessionClass session_;
  base::Installed<SessionClass>::Scope session_scope_{session_};
  SpecialClass special_{};
  base::Installed<SpecialClass>::Scope special_scope_{special_};
  DebugState debug_state_;
  base::Installed<DebugState>::Scope debug_state_scope_{debug_state_};
  Network network_;
  base::Installed<Network>::Scope network_scope_{network_};
};

// Sets everything the settings hold to a value that is not its default.
void FillUnusualSettings() {
  base::SafeCopy(TheSession().player_name(), "BOB");
  TheSession().preferred_color() = 3;
  TheSession().house() = HOUSE_BAD;
  TheNetwork().current_phone_index() = 1;

  SerialSettingsType& serial = TheNetwork().serial_defaults();
  serial.Port = 0x2f8;
  serial.IRQ = 3;
  serial.Baud = 9600;
  serial.DialMethod = DIAL_PULSE;
  serial.InitStringIndex = 1;
  serial.CallWaitStringIndex = 2;
  base::SafeCopy(serial.CallWaitString, "*70");
  serial.Init = true;
  serial.Compression = true;
  serial.ErrorCorrection = true;
  serial.HardwareFlowControl = false;
  base::SafeCopy(serial.ModemName, "HAYES");
}

TEST_F(MultiPlayerSettingsTest,
       SavingKeepsSectionsAndEntriesTheGameDoesNotKnow) {
  IniFile ini;
  LoadIniText(
      "; written by hand\r\n"
      "[Mine]\r\n"
      "Foo=Bar\r\n"
      "[MultiPlayer]\r\n"
      "Unknown=Kept\r\n"
      "Color=2\r\n"
      "[SerialDefaults]\r\n"
      "Stale=Dropped\r\n",
      ini);
  FillUnusualSettings();

  WriteMultiPlayerSettings(ini);
  IniFile reloaded;
  SaveAndReload(ini, reloaded);

  EXPECT_EQ(reloaded.GetString("Mine", "Foo", ""), "Bar");
  EXPECT_EQ(reloaded.GetString("MultiPlayer", "Unknown", ""), "Kept");
  EXPECT_EQ(reloaded.GetInt("MultiPlayer", "Color", -1), 3);
  // The section the settings own is rewritten whole.
  EXPECT_FALSE(reloaded.HasEntry("SerialDefaults", "Stale"));
  EXPECT_EQ(reloaded.GetInt("SerialDefaults", "Baud", -1), 9600);
}

TEST_F(MultiPlayerSettingsTest, EverySettingReadsBackAsItWasWritten) {
  FillUnusualSettings();
  AddInitString("ATZ");
  AddInitString("AT&F");
  PhoneEntryClass& phone = AddPhone("HOME", "555-1234");
  phone.Settings.Port = 0x3e8;
  phone.Settings.IRQ = 5;
  phone.Settings.Baud = 19200;
  phone.Settings.Compression = true;
  phone.Settings.ErrorCorrection = true;
  phone.Settings.HardwareFlowControl = false;
  phone.Settings.DialMethod = DIAL_PULSE;
  phone.Settings.InitStringIndex = 1;
  phone.Settings.CallWaitStringIndex = 2;
  base::SafeCopy(phone.Settings.CallWaitString, "*71");
  AddPhone("WORK", "555-9876").Settings.Baud = 2400;

  IniFile ini;
  WriteMultiPlayerSettings(ini);

  // Forget everything, then read it back.
  TheNetwork().init_strings().clear();
  TheNetwork().phone_book().clear();
  TheNetwork().serial_defaults() = SerialSettingsType{};
  base::SafeCopy(TheSession().player_name(), "");
  TheSession().preferred_color() = 0;
  TheSession().house() = HOUSE_GOOD;
  TheNetwork().current_phone_index() = -1;
  ReadMultiPlayerSettings(ini);

  EXPECT_STREQ(TheSession().player_name(), "BOB");
  EXPECT_EQ(TheSession().preferred_color(), 3);
  EXPECT_EQ(TheSession().house(), HOUSE_BAD);
  EXPECT_EQ(TheNetwork().current_phone_index(), 1);

  const SerialSettingsType& serial = TheNetwork().serial_defaults();
  EXPECT_EQ(serial.Port, 0x2f8);
  EXPECT_EQ(serial.IRQ, 3);
  EXPECT_EQ(serial.Baud, 9600);
  EXPECT_EQ(serial.DialMethod, DIAL_PULSE);
  EXPECT_EQ(serial.InitStringIndex, 1);
  EXPECT_EQ(serial.CallWaitStringIndex, 2);
  EXPECT_STREQ(serial.CallWaitString, "*70");
  EXPECT_TRUE(serial.Init);
  EXPECT_TRUE(serial.Compression);
  EXPECT_TRUE(serial.ErrorCorrection);
  EXPECT_FALSE(serial.HardwareFlowControl);
  EXPECT_STREQ(serial.ModemName, "HAYES");

  ASSERT_EQ(TheNetwork().init_strings().size(), 2U);
  EXPECT_EQ(InitString(0), "ATZ");
  EXPECT_EQ(InitString(1), "AT&F");

  ASSERT_EQ(TheNetwork().phone_book().size(), 2U);
  const PhoneEntryClass& home = *TheNetwork().phone_book().at(0);
  EXPECT_STREQ(home.Name, "HOME");
  EXPECT_STREQ(home.Number, "555-1234");
  EXPECT_EQ(home.Settings.Port, 0x3e8);
  EXPECT_EQ(home.Settings.IRQ, 5);
  EXPECT_EQ(home.Settings.Baud, 19200);
  EXPECT_TRUE(home.Settings.Compression);
  EXPECT_TRUE(home.Settings.ErrorCorrection);
  EXPECT_FALSE(home.Settings.HardwareFlowControl);
  EXPECT_EQ(home.Settings.DialMethod, DIAL_PULSE);
  // The three fields after the dial method are written but read back as their
  // defaults: the reader copies the dial method over the start of the text it
  // is still splitting, which zero-fills the rest. The file text says what was
  // written.
  EXPECT_EQ(ini.GetString("PhoneBook", "000", ""),
            "HOME|555-1234|3e8|5|19200|1|1|0|P|1|2|*71");
  EXPECT_STREQ(TheNetwork().phone_book().at(1)->Name, "WORK");
  EXPECT_EQ(TheNetwork().phone_book().at(1)->Settings.Baud, 2400);
}

// A file the old binary wrote with an empty init string holds "000=" and
// indexes counted with it. The empty string is not a list entry, and an index
// the list lost falls back to the first string instead of overrunning it.
TEST_F(MultiPlayerSettingsTest,
       AnOldFileWithAnEmptyInitStringGivesAValidIndex) {
  IniFile ini;
  LoadIniText(
      "[SerialDefaults]\r\nInitStringIndex=1\r\n"
      "[InitStrings]\r\n000=\r\n001=ATZ\r\n",
      ini);
  ReadMultiPlayerSettings(ini);

  ASSERT_EQ(TheNetwork().init_strings().size(), 1U);
  EXPECT_EQ(InitString(0), "ATZ");
  EXPECT_EQ(TheNetwork().serial_defaults().InitStringIndex, 0);
}

TEST_F(MultiPlayerSettingsTest, AnIndexBeyondTheListIsClamped) {
  IniFile ini;
  LoadIniText(
      "[SerialDefaults]\r\nInitStringIndex=7\r\n"
      "[InitStrings]\r\n000=ATZ\r\n001=AT&F\r\n"
      "[PhoneBook]\r\n000=home|555|3f8|4|9600|0|0|1|T|9|3|\r\n",
      ini);
  ReadMultiPlayerSettings(ini);
  EXPECT_EQ(TheNetwork().serial_defaults().InitStringIndex, 0);

  // -1 is the "send nothing" choice and stays.
  IniFile none;
  LoadIniText(
      "[SerialDefaults]\r\nInitStringIndex=-1\r\n[InitStrings]\r\n000=ATZ\r\n",
      none);
  ReadMultiPlayerSettings(none);
  EXPECT_EQ(TheNetwork().serial_defaults().InitStringIndex, -1);
}

TEST_F(MultiPlayerSettingsTest, ABlankInitStringIsNotAListEntry) {
  IniFile ini;
  LoadIniText("[InitStrings]\r\n000=   \r\n001=ATZ\r\n", ini);
  ReadMultiPlayerSettings(ini);

  ASSERT_EQ(TheNetwork().init_strings().size(), 1U);
  EXPECT_EQ(InitString(0), "ATZ");
}

TEST_F(MultiPlayerSettingsTest, WritingAndReadingKeepsTheInitStringIndex) {
  AddInitString("ATZ");
  AddInitString("AT&F");
  AddInitString("AT&K3");
  TheNetwork().serial_defaults().InitStringIndex = 2;

  IniFile ini;
  WriteMultiPlayerSettings(ini);
  EXPECT_EQ(ini.GetInt("SerialDefaults", "InitStringIndex", -9), 2);
  ReadMultiPlayerSettings(ini);

  ASSERT_EQ(TheNetwork().init_strings().size(), 3U);
  EXPECT_EQ(TheNetwork().serial_defaults().InitStringIndex, 2);
  EXPECT_EQ(InitString(2), "AT&K3");
}

// An entry with no value is not stored, and the readers' defaults for it are
// what the old reader returned for the empty value it kept.
TEST_F(MultiPlayerSettingsTest, EmptyTextsReadBackAsTheirDefaults) {
  base::SafeCopy(TheSession().player_name(), "");
  base::SafeCopy(TheNetwork().serial_defaults().ModemName, "");
  base::SafeCopy(TheNetwork().serial_defaults().CallWaitString, "");

  IniFile ini;
  LoadIniText(
      "[MultiPlayer]\r\nHandle=Old\r\n[SerialDefaults]\r\nModemName=X\r\n",
      ini);
  WriteMultiPlayerSettings(ini);
  EXPECT_FALSE(ini.HasEntry("MultiPlayer", "Handle"));

  base::SafeCopy(TheNetwork().serial_defaults().ModemName, "STALE");
  ReadMultiPlayerSettings(ini);

  EXPECT_STREQ(TheSession().player_name(), "Noname");
  EXPECT_STREQ(TheNetwork().serial_defaults().ModemName, "");
  EXPECT_STREQ(TheNetwork().serial_defaults().CallWaitString, "");
}

TEST_F(MultiPlayerSettingsTest, NoInitStringsGivesOneDefaultOne) {
  const IniFile ini;
  TheNetwork().serial_defaults().InitStringIndex = 5;
  ReadMultiPlayerSettings(ini);

  ASSERT_EQ(TheNetwork().init_strings().size(), 1U);
  EXPECT_EQ(InitString(0), "ATZ");
  EXPECT_EQ(TheNetwork().serial_defaults().InitStringIndex, 0);
}

// A key with nothing after the '=' is not kept by IniFile, so it is not a
// string or a phone book entry. The old reader listed it as an empty one.
TEST_F(MultiPlayerSettingsTest, AKeyWithoutAValueIsNotAListEntry) {
  IniFile ini;
  LoadIniText(
      "[InitStrings]\r\n000=\r\n001=at&f\r\n"
      "[PhoneBook]\r\n000=\r\n001=home|555|3f8|4|9600|0|0|1|T|0|3|\r\n",
      ini);
  ReadMultiPlayerSettings(ini);

  ASSERT_EQ(TheNetwork().init_strings().size(), 1U);
  EXPECT_EQ(InitString(0), "AT&F");
  ASSERT_EQ(TheNetwork().phone_book().size(), 1U);
  EXPECT_STREQ(TheNetwork().phone_book().at(0)->Name, "HOME");
}

}  // namespace

}  // namespace td
