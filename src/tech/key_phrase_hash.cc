#include "tech/key_phrase_hash.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <ranges>
#include <span>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/array.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "tech/crc.h"

uint32_t HashKeyPhrase(const std::string_view phrase) {
  // Up to 127 phrase characters and a terminator, which the padding can
  // overwrite to reach 128.
  std::array<char, 128> phrase_buffer{};

  // Work on a copy so the caller's phrase is left alone. The original read at
  // most 127 characters and stopped at an embedded NUL; keep both limits.
  // SafeCopy() zero-fills the rest of the buffer, as the original's strncpy()
  // did, and an empty phrase's padding reads those zeros.
  port::SafeCopy(phrase_buffer, phrase);
  const int length =
      static_cast<int>(std::string_view(phrase_buffer.data()).size());

  // Fold the phrase to upper case, and replace spaces, control characters and
  // other non-printing bytes with a letter that depends on the position, so
  // the hash only ever sees visible ASCII; bytes >= 0x80 count as
  // non-printing. "7TH GRADE" therefore hashes like "7THDGRADE".
  for (int index = 0; index < length; index++) {
    char& c = phrase_buffer.at(base::ToSize(index));
    c = absl::ascii_isgraph(static_cast<uint8_t>(c))
            ? absl::ascii_toupper(static_cast<uint8_t>(c))
            : static_cast<char>('A' + (index % 26));
  }

  // Pad the phrase to at least 16 characters and to a multiple of four, which
  // the four-byte round below needs. Each padding letter is derived from the
  // character `length` positions earlier, which past the phrase is earlier
  // padding. An empty phrase reads its own slot instead, which is zero, and
  // still hashes to a non-zero code.
  const int padded_length = std::max(((length + 3) / 4) * 4, 16);
  for (int index = length; index < padded_length; index++) {
    const int mixed =
        static_cast<uint8_t>('?') ^
        static_cast<uint8_t>(phrase_buffer.at(base::ToSize(index - length)));
    phrase_buffer.at(base::ToSize(index)) =
        static_cast<char>('A' + ((mixed + index) % 26));
  }
  const std::span padded =
      std::span(phrase_buffer).first(base::ToSize(padded_length));

  // Start from the CRC of the padded phrase read backwards. The original also
  // hashed it forwards, meaning to double the work of reversing the CRC, but
  // then XORed that same CRC back out; only the backward one reaches the
  // result.
  CrcEngine backward_crc;
  for (const char c : padded | std::views::reverse) {
    backward_crc.Update(static_cast<uint8_t>(c));
  }
  uint32_t code = backward_crc.Value();

  // Feed the phrase through `code` one byte at a time, which the original
  // calls a decoy cipher ahead of the real one: each byte is replaced by the
  // code's current low byte, XORed into it, and the code rotates right by a
  // byte. The rotation must also sign-extend, as the original's shift of a
  // signed long did; a plain rotate changes the historical codes. Then force a
  // few bits on and a few off, repeating every eight bytes, so that the bytes
  // lose information; the original meant this to frustrate cryptographic
  // attacks and limited it to under 10% of the bits.
  static constexpr std::array<uint8_t, 8> kBitsForcedOff = {
      0x00, 0x08, 0x00, 0x20, 0x00, 0x04, 0x10, 0x00};
  static constexpr std::array<uint8_t, 8> kBitsForcedOn = {
      0x10, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x04};
  for (int index = 0; index < padded_length; index++) {
    char& byte = phrase_buffer.at(base::ToSize(index));
    const auto phrase_byte = static_cast<uint8_t>(byte);
    const uint32_t forced_on = kBitsForcedOn.at(base::ToSize(index % 8));
    const uint32_t forced_off = kBitsForcedOff.at(base::ToSize(index % 8));
    byte = static_cast<char>(((code & 0xFFU) | forced_on) & ~forced_off);
    code ^= phrase_byte;
    const uint32_t sign_extension = (code & 0x80000000U) ? 0xFF000000U : 0U;
    code = std::rotr(code, 8) | sign_extension;
  }

  // Scramble each group of four bytes with a multiply/add/XOR round that
  // uses the bytes themselves as the key. The original calls it a variation
  // on the cipher in PGP; keyed by its own data it is not a real cipher, only
  // one more step an attacker has to invert.
  for (int index = 0; index < padded_length; index += 4) {
    // The original computed in signed 16-bit values from signed chars. 32-bit
    // unsigned ones give the same bytes: +, * and ^ produce low 8 bits that
    // depend only on the low 8 bits of their operands, and only those are
    // stored back.
    const std::span group = padded.subspan(base::ToSize(index), 4);
    const uint32_t a = static_cast<uint8_t>(base::At(group, 0));
    const uint32_t b = static_cast<uint8_t>(base::At(group, 1));
    const uint32_t c = static_cast<uint8_t>(base::At(group, 2));
    const uint32_t d = static_cast<uint8_t>(base::At(group, 3));
    const uint32_t a_squared = a * a;
    const uint32_t d_squared = d * d;
    const uint32_t mix3 = ((c + c) ^ a_squared) * a;
    const uint32_t mix2 = (((b + b) ^ d_squared) + mix3) * c;
    const uint32_t mix3b = mix3 + mix2;
    base::At(group, 0) = static_cast<char>(a_squared ^ mix2);
    base::At(group, 1) = static_cast<char>(mix2 ^ (c + c));
    base::At(group, 2) = static_cast<char>(mix3b ^ (b + b));
    base::At(group, 3) = static_cast<char>(d_squared ^ mix3b);
  }

  return CrcEngine::Compute(
      std::string_view(phrase_buffer.data(), base::ToSize(padded_length)));
}
