// Holds the Rust public-key code behind PKey to the C++ bignum it replaced:
// the same block sizes, the same encodings, and the same bytes out of
// Encrypt() and Decrypt(), for generated key pairs and for moduli and
// exponents of every kind a DER integer can carry. The one exception is
// spelled out at ArbitraryModuliMatchReferenceOrLongDivision.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/numbers.h"
#include "base/lcg_test_util.h"
#include "base/numeric.h"
#include "engine/crypto/digit_cursor_reference_test_util.h"
#include "engine/crypto/int_reference_test_util.h"
#include "engine/crypto/pk.h"
#include "engine/crypto/pk_reference_test_util.h"
#include "engine/crypto/random_source.h"
#include "gtest/gtest.h"

namespace {

constexpr std::byte kFill{0xcc};  // What untouched destination bytes hold.

// The same key in both implementations.
struct Keys {
  PKey rust;
  pk_reference::PKey reference;

  void DecodeModulus(const std::span<const std::byte> der) {
    rust.Decode_Modulus(der);
    reference.Decode_Modulus(der);
  }
  void DecodeExponent(const std::span<const std::byte> der) {
    rust.Decode_Exponent(der);
    reference.Decode_Exponent(der);
  }
};

void Seed(RandomSource& rng, const int32_t salt) {
  for (int32_t value = 1; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long((value * 40503) + salt);
  }
}

// `size` random bytes as a DER integer, in the short or the long form of the
// length as the size needs.
std::vector<std::byte> DerInteger(const std::vector<std::byte>& digits) {
  std::vector<std::byte> der{std::byte{2}};
  if (digits.size() < 128) {
    der.push_back(static_cast<std::byte>(digits.size()));
  } else if (digits.size() < 256) {
    der.push_back(std::byte{0x81});
    der.push_back(static_cast<std::byte>(digits.size()));
  } else {
    der.push_back(std::byte{0x82});
    der.push_back(static_cast<std::byte>(digits.size() >> 8U));
    der.push_back(static_cast<std::byte>(digits.size() & 0xffU));
  }
  der.insert(der.end(), digits.begin(), digits.end());
  return der;
}

template <typename Key>
std::vector<std::byte> Encoded(const Key& key, const bool modulus) {
  std::vector<std::byte> buffer(PKey::kMaxEncodedBytes);
  buffer.resize(base::ToSize(modulus ? key.Encode_Modulus(buffer)
                                     : key.Encode_Exponent(buffer)));
  return buffer;
}

// Requires the two keys to describe themselves alike and to turn the same
// random data into the same bytes, both ways.
void ExpectSameBehavior(base::Lcg& lcg, const Keys& keys) {
  ASSERT_EQ(keys.rust.Plain_Block_Size(), keys.reference.Plain_Block_Size());
  ASSERT_EQ(keys.rust.Crypt_Block_Size(), keys.reference.Crypt_Block_Size());
  ASSERT_TRUE(Encoded(keys.rust, true) == Encoded(keys.reference, true));
  ASSERT_TRUE(Encoded(keys.rust, false) == Encoded(keys.reference, false));
  const int plain = keys.rust.Plain_Block_Size();
  const int crypt = keys.rust.Crypt_Block_Size();
  for (const bool encrypt : {true, false}) {
    // Up to three blocks and a part of one, into a destination that may be a
    // little short of or beyond what they need.
    const int in_block = encrypt ? plain : crypt;
    const int out_block = encrypt ? crypt : plain;
    const auto source = lcg.Bytes((in_block * lcg.Below(4)) + lcg.Below(5));
    std::vector<std::byte> want(
        base::ToSize(std::max(0, (out_block * 3) + lcg.Below(9) - 4)), kFill);
    std::vector<std::byte> got(want);
    ASSERT_EQ(encrypt ? keys.rust.Encrypt(source, got)
                      : keys.rust.Decrypt(source, got),
              encrypt ? keys.reference.Encrypt(source, want)
                      : keys.reference.Decrypt(source, want));
    ASSERT_TRUE(got == want) << "encrypt " << encrypt << ", blocks of "
                             << in_block << " into " << out_block;
  }
}

// A key pair as each implementation generates it must work in the other.
TEST(PkDifferentialTest, ReferenceKeyPairsWorkInRust) {
  base::Lcg lcg(1);
  for (const int bits : {40, 64, 96, 128, 200}) {
    RandomSource rng;
    Seed(rng, bits);
    pk_reference::PKey fast;
    pk_reference::PKey slow;
    pk_reference::PKey::Generate(rng, bits, fast, slow);
    for (const pk_reference::PKey* const key : {&fast, &slow}) {
      Keys keys;
      keys.DecodeModulus(Encoded(*key, true));
      keys.DecodeExponent(Encoded(*key, false));
      for (int round = 0; round < 20; ++round) {
        ExpectSameBehavior(lcg, keys);
      }
    }
  }
}

TEST(PkDifferentialTest, RustKeyPairsWorkInTheReference) {
  base::Lcg lcg(2);
  for (const int bits : {32, 64, 96, 128, 200, 256}) {
    RandomSource rng;
    Seed(rng, bits);
    PKey fast;
    PKey slow;
    PKey::Generate(rng, bits, fast, slow);
    Keys fast_keys;
    fast_keys.DecodeModulus(Encoded(fast, true));
    fast_keys.DecodeExponent(Encoded(fast, false));
    Keys slow_keys;
    slow_keys.DecodeModulus(Encoded(slow, true));
    slow_keys.DecodeExponent(Encoded(slow, false));
    for (int round = 0; round < 20; ++round) {
      ExpectSameBehavior(lcg, fast_keys);
      ExpectSameBehavior(lcg, slow_keys);
    }
    // And the reference undoes with one key what it did with the other.
    const auto plain = lcg.Bytes(fast.Plain_Block_Size());
    std::vector<std::byte> cipher(base::ToSize(fast.Crypt_Block_Size()));
    std::vector<std::byte> round_trip(plain.size());
    ASSERT_GT(fast_keys.reference.Encrypt(plain, cipher), 0);
    ASSERT_GT(slow_keys.reference.Decrypt(cipher, round_trip), 0);
    EXPECT_TRUE(round_trip == plain) << bits << "-bit primes";
  }
}

// A big-endian number as the reference's bignum.
BigInt FromBigEndian(const std::span<const std::byte> bytes) {
  BigInt number;
  number.Unsigned_Decode(DigitCursor<const unsigned char>::FromBytes(bytes),
                         static_cast<int>(bytes.size()));
  return number;
}

BigInt FromLittleEndian(std::vector<std::byte> bytes) {
  std::ranges::reverse(bytes);
  return FromBigEndian(bytes);
}

// base ^ exponent mod modulus by squaring and multiplying, each product
// reduced by the reference's long division: slow, and independent of the
// reduction PKey's exponentiation used. For numbers of up to 512 bits, whose
// products fit the bignum.
BigInt PowerByLongDivision(const BigInt& base, const BigInt& exponent,
                           const BigInt& modulus) {
  BigInt result = 1;
  for (int bit = exponent.BitCount() - 1; bit >= 0; --bit) {
    result = (result * result) % modulus;
    if (exponent.at(bit)) {
      result = (result * base) % modulus;
    }
  }
  return result;
}

// Numbers that are no key pair at all: any modulus, odd or even, with any
// exponent below it. The C++ aborts on a modulus of 32 bits or fewer, so
// these start at five bytes with the top one set.
//
// This is where the two were found to disagree: about once in several
// thousand blocks the reference's exponentiation returns a number that is not
// the power (see TheReferenceGetsThisPowerWrong). So a disagreement is taken
// to long division, which must side with Rust, and there may only be a few.
TEST(PkDifferentialTest, ArbitraryModuliMatchReferenceOrLongDivision) {
  base::Lcg lcg(3);
  int reference_errors = 0;
  int blocks = 0;
  for (int round = 0; round < 1500; ++round) {
    auto modulus = lcg.Bytes(5 + lcg.Below(60));
    // Positive, so that it is not sign-extended to 2048 bits, and with a top
    // byte that is not zero.
    modulus.front() = static_cast<std::byte>(1 + lcg.Below(127));
    // Mostly an exponent below the modulus; sometimes above it, or zero.
    auto exponent = lcg.Bytes(1 + lcg.Below(static_cast<int>(modulus.size())));
    exponent.front() &= std::byte{0x7f};
    if (lcg.Below(20) == 0) {
      std::ranges::fill(exponent, std::byte{0});
    }
    Keys keys;
    keys.DecodeModulus(DerInteger(modulus));
    keys.DecodeExponent(DerInteger(exponent));
    ASSERT_EQ(keys.rust.Plain_Block_Size(), keys.reference.Plain_Block_Size());
    ASSERT_TRUE(Encoded(keys.rust, true) == Encoded(keys.reference, true));
    ASSERT_TRUE(Encoded(keys.rust, false) == Encoded(keys.reference, false));

    for (int block = 0; block < 6; ++block, ++blocks) {
      // Encrypting takes blocks that are below the modulus, decrypting ones
      // that may not be.
      const bool encrypt = block % 2 == 0;
      const auto source = lcg.Bytes(encrypt ? keys.rust.Plain_Block_Size()
                                            : keys.rust.Crypt_Block_Size());
      std::vector<std::byte> want(base::ToSize(keys.rust.Crypt_Block_Size()));
      std::vector<std::byte> got(want);
      ASSERT_EQ(encrypt ? keys.rust.Encrypt(source, got)
                        : keys.rust.Decrypt(source, got),
                encrypt ? keys.reference.Encrypt(source, want)
                        : keys.reference.Decrypt(source, want));
      if (got == want) {
        continue;
      }
      ++reference_errors;
      const BigInt base = FromLittleEndian(source);
      // The engine refuses a block or an exponent that is not below the
      // modulus with 1; the two never disagreed about that.
      ASSERT_TRUE(base < FromBigEndian(modulus));
      ASSERT_TRUE(FromBigEndian(exponent) < FromBigEndian(modulus));
      const BigInt power = PowerByLongDivision(base, FromBigEndian(exponent),
                                               FromBigEndian(modulus));
      // Decrypting keeps the power's low bytes: one byte less than it may
      // have. Encode() writes them most significant first.
      if (!encrypt) {
        got.pop_back();
      }
      std::vector<std::byte> kept(got.size());
      static_cast<void>(
          power.Encode(DigitCursor<unsigned char>::FromBytes(kept),
                       static_cast<unsigned>(kept.size())));
      std::ranges::reverse(kept);
      EXPECT_TRUE(got == kept) << "round " << round;
    }
  }
  RecordProperty("blocks", blocks);
  RecordProperty("reference_errors", reference_errors);
  EXPECT_LE(reference_errors, 5) << "of " << blocks << " blocks";
}

// One block the C++ exponentiation got wrong, found by the test above. The
// expected power is Python's pow(block, exponent, modulus); long division in
// the reference's own bignum agrees with it, the reference's PKey does not.
TEST(PkDifferentialTest, TheReferenceGetsThisPowerWrong) {
  const auto bytes = [](const std::string_view hex) {
    std::vector<std::byte> result;
    for (std::size_t at = 0; at + 1 < hex.size(); at += 2) {
      int value = 0;
      CHECK(absl::SimpleHexAtoi(hex.substr(at, 2), &value));
      result.push_back(static_cast<std::byte>(value));
    }
    return result;
  };
  const auto modulus = bytes(
      "6c02fff5c8a3ed65eda64b082d7ffe720ce029784cbf302df34fb7e54404a334c39217ef"
      "35ee77299d4b68363f1dccaa9f19c95a8131c258ea9b");
  const auto exponent = bytes(
      "1dca79d49e733fb8328911bcdc409433e00caab2c2a1790b47f46454713810e086e15f43"
      "0aa47752bf73bb");
  // A block and its power, least significant byte first.
  const auto block = bytes(
      "cb5c98225686f8d8943e04ce87b760577565f108e2fcdab390717f6e85f7848c0402cea2"
      "91478082f1b8fe42268bec3576342fef6565ea45b5");
  const auto power = bytes(
      "b2a0129f56ef872ba5423c1e0a803ee5fe80211443a68a52e10dd4af7222e07558d71e4f"
      "7c65927f7aed3448a0c30965815bc7a0bbc55dc0a000");
  Keys keys;
  keys.DecodeModulus(DerInteger(modulus));
  keys.DecodeExponent(DerInteger(exponent));
  ASSERT_EQ(keys.rust.Plain_Block_Size(), std::ssize(block));

  std::vector<std::byte> got(power.size());
  ASSERT_EQ(keys.rust.Encrypt(block, got), std::ssize(power));
  EXPECT_TRUE(got == power);
  EXPECT_TRUE(PowerByLongDivision(FromLittleEndian(block),
                                  FromBigEndian(exponent),
                                  FromBigEndian(modulus)) ==
              FromLittleEndian(power));

  std::vector<std::byte> reference(power.size());
  ASSERT_EQ(keys.reference.Encrypt(block, reference), std::ssize(power));
  EXPECT_FALSE(reference == power);
}

TEST(PkDifferentialTest, LargeAndNegativeNumbersMatchReference) {
  base::Lcg lcg(4);
  for (int round = 0; round < 40; ++round) {
    // Up to the whole 2048-bit register, and one time in four with the top
    // bit set, which sign-extends the modulus to the register's size.
    auto modulus = lcg.Bytes(65 + lcg.Below(192));
    if (lcg.Below(4) != 0) {
      modulus.front() = static_cast<std::byte>(1 + lcg.Below(127));
    }
    // A short exponent mostly: a long one takes the C++ a second.
    auto exponent = lcg.Bytes(lcg.Below(8) == 0 ? 40 : 1 + lcg.Below(4));
    if (lcg.Below(4) != 0) {
      exponent.front() &= std::byte{0x7f};
    }
    Keys keys;
    keys.DecodeModulus(DerInteger(modulus));
    keys.DecodeExponent(DerInteger(exponent));
    ExpectSameBehavior(lcg, keys);
  }
}

// Whatever a decode makes of bytes that are not a DER integer, on top of a key
// that already holds numbers.
TEST(PkDifferentialTest, MalformedDerMatchesReference) {
  base::Lcg lcg(5);
  for (int round = 0; round < 3000; ++round) {
    Keys keys;
    auto modulus = lcg.Bytes(8 + lcg.Below(20));
    modulus.front() = std::byte{0x55};
    keys.DecodeModulus(DerInteger(modulus));
    keys.DecodeExponent(DerInteger(lcg.Bytes(3)));

    // A valid integer cut short or with a byte changed, or plain noise that
    // starts like an integer.
    std::vector<std::byte> der;
    if (lcg.Below(2) == 0) {
      auto digits = lcg.Bytes(6 + lcg.Below(300));
      digits.front() = std::byte{0x41};
      der = DerInteger(digits);
      if (lcg.Below(2) == 0) {
        der.resize(base::ToSize(lcg.Below(static_cast<int>(der.size()))));
      } else {
        der.at(base::ToSize(lcg.Below(4))) ^=
            static_cast<std::byte>(1 + lcg.Below(255));
      }
    } else {
      der = lcg.Bytes(lcg.Below(12));
      if (!der.empty() && lcg.Below(4) != 0) {
        der.front() = std::byte{2};
      }
    }
    // A changed length can make a valid number of 32 bits or fewer, which
    // the C++ cannot use as a modulus; decode those as the exponent only.
    const bool short_number = der.size() < 7 || der.at(1) < std::byte{5} ||
                              der.at(2) == std::byte{0};
    if (!short_number && lcg.Below(2) == 0) {
      keys.DecodeModulus(der);
    } else {
      keys.DecodeExponent(der);
    }
    ExpectSameBehavior(lcg, keys);
  }
}

}  // namespace
