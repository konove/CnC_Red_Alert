// Holds the Rust Blowfish cipher behind BlowfishEngine to the C++ engine it
// replaced: the same bytes and return values for every key length, with and
// without a key, into another buffer and in place.

#include <cstddef>
#include <span>
#include <vector>

#include "base/lcg_test_util.h"
#include "engine/crypto/blowfish.h"
#include "engine/crypto/blowfish_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

constexpr std::byte kFill{0xcc};  // What untouched destination bytes hold.

// Encrypts and decrypts `data` with both engines, into a destination `slack`
// bytes longer than it and in place.
void ExpectMatch(BlowfishEngine& engine,
                 blowfish_reference::BlowfishEngine& reference,
                 const std::vector<std::byte>& data, const int slack) {
  for (const bool encrypt : {true, false}) {
    std::vector<std::byte> want(data.size() + static_cast<std::size_t>(slack),
                                kFill);
    std::vector<std::byte> got(want);
    ASSERT_EQ(
        encrypt ? engine.Encrypt(data, got) : engine.Decrypt(data, got),
        encrypt ? reference.Encrypt(data, want) : reference.Decrypt(data, want));
    ASSERT_TRUE(got == want) << data.size() << " bytes, encrypt " << encrypt;

    std::vector<std::byte> want_in_place(data);
    std::vector<std::byte> got_in_place(data);
    ASSERT_EQ(encrypt ? engine.Encrypt(got_in_place, got_in_place)
                      : engine.Decrypt(got_in_place, got_in_place),
              encrypt ? reference.Encrypt(want_in_place, want_in_place)
                      : reference.Decrypt(want_in_place, want_in_place));
    ASSERT_TRUE(got_in_place == want_in_place)
        << data.size() << " bytes in place, encrypt " << encrypt;
  }
}

TEST(BlowfishDifferentialTest, EveryKeyLengthMatchesReference) {
  base::Lcg lcg(1);
  for (int key_size = 1; key_size <= BlowfishEngine::kMaxKeyLength;
       ++key_size) {
    for (int round = 0; round < 8; ++round) {
      const auto key = lcg.Bytes(key_size);
      BlowfishEngine engine;
      blowfish_reference::BlowfishEngine reference;
      engine.Submit_Key(key);
      reference.Submit_Key(key);
      ExpectMatch(engine, reference, lcg.Bytes(lcg.Below(200)), lcg.Below(3));
    }
  }
}

TEST(BlowfishDifferentialTest, EveryLengthMatchesReference) {
  base::Lcg lcg(2);
  const auto key = lcg.Bytes(BlowfishEngine::kMaxKeyLength);
  BlowfishEngine engine;
  blowfish_reference::BlowfishEngine reference;
  engine.Submit_Key(key);
  reference.Submit_Key(key);
  for (int size = 0; size < 70; ++size) {
    ExpectMatch(engine, reference, lcg.Bytes(size), 0);
    ExpectMatch(engine, reference, lcg.Bytes(size), 5);
  }
}

TEST(BlowfishDifferentialTest, RekeyingAndUnkeyingMatchReference) {
  base::Lcg lcg(3);
  BlowfishEngine engine;
  blowfish_reference::BlowfishEngine reference;
  // No key yet: the data is copied.
  ExpectMatch(engine, reference, lcg.Bytes(21), 2);
  for (int round = 0; round < 200; ++round) {
    // One time in four an empty key, which removes the key.
    const auto key = lcg.Bytes(
        lcg.Below(4) == 0 ? 0 : 1 + lcg.Below(BlowfishEngine::kMaxKeyLength));
    engine.Submit_Key(key);
    reference.Submit_Key(key);
    ExpectMatch(engine, reference, lcg.Bytes(lcg.Below(100)), lcg.Below(2));
  }
}

TEST(BlowfishDeathTest, RejectsBuffersThatPartlyOverlap) {
  std::vector<std::byte> buffer(32);
  BlowfishEngine engine;
  engine.Submit_Key(std::span(buffer).first(8));
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(engine.Encrypt(std::span(buffer).first(16),
                              std::span(buffer).subspan(8, 16)),
               "without being one buffer");
}

TEST(BlowfishDeathTest, RejectsAKeyThatIsTooLong) {
  const std::vector<std::byte> key(BlowfishEngine::kMaxKeyLength + 1);
  BlowfishEngine engine;
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(engine.Submit_Key(key), "");
}

}  // namespace
