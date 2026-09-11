// Checks numeric defaults and legacy hexadecimal forms through the INI API.
#include "gtest/gtest.h"
#include "ra/ini.h"

namespace {

TEST(IniNumberTest, InvalidNumbersKeepTheCallersDefault) {
  INIClass ini;
  EXPECT_EQ(ini.Get_Int("Values", "Missing", 73), 73);
  for (const char* text : {"invalid", "12tail", "2147483648", "-2147483649",
                           "$100000000", "ffjunkh"}) {
    ASSERT_TRUE(ini.Put_String("Values", "Number", text));
    EXPECT_EQ(ini.Get_Int("Values", "Number", 73), 73) << text;
  }
}

TEST(IniNumberTest, PreservesIntegerAndHexadecimalForms) {
  INIClass ini;
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

}  // namespace
