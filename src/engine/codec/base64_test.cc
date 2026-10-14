// Tests for Base64_Encode and Base64_Decode against the RFC 4648 vectors.

#include "engine/codec/base64.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace {

std::string Encode(std::string_view text) {
  std::string out(((text.size() + 2) / 3) * 4, '\0');
  const int n = Base64_Encode(std::as_bytes(std::span(text)),
                              std::as_writable_bytes(std::span(out)));
  out.resize(static_cast<std::size_t>(n));
  return out;
}

std::string Decode(std::string_view code) {
  std::string out((code.size() / 4) * 3, '\0');
  const int n = Base64_Decode(std::as_bytes(std::span(code)),
                              std::as_writable_bytes(std::span(out)));
  out.resize(static_cast<std::size_t>(n));
  return out;
}

TEST(Base64Test, EncodesRfc4648Vectors) {
  EXPECT_EQ(Encode(""), "");
  EXPECT_EQ(Encode("M"), "TQ==");
  EXPECT_EQ(Encode("Ma"), "TWE=");
  EXPECT_EQ(Encode("Man"), "TWFu");
  EXPECT_EQ(Encode("foobar"), "Zm9vYmFy");
}

TEST(Base64Test, DecodesRfc4648Vectors) {
  EXPECT_EQ(Decode(""), "");
  EXPECT_EQ(Decode("TQ=="), "M");
  EXPECT_EQ(Decode("TWE="), "Ma");
  EXPECT_EQ(Decode("TWFu"), "Man");
  EXPECT_EQ(Decode("Zm9vYmFy"), "foobar");
}

TEST(Base64Test, RoundTripsEveryByte) {
  std::string text(256, '\0');
  for (int i = 0; char& c : text) {
    c = static_cast<char>(i++);
  }
  EXPECT_EQ(Decode(Encode(text)), text);
}

}  // namespace
