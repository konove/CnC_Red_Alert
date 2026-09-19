#include "ra/obfuscate.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "tech/crc.h"

uint32_t Obfuscate(const std::string_view string) {
  // 127 phrase characters, the padding that can bring 127 up to 128, and a
  // terminator. The 0xA5 fill is not just initialization: an empty phrase's
  // padding is derived from it (see below), so it is part of the hash.
  std::array<char, 129> buffer{};

  base::FillBytes(base::ObjectBytes(buffer), '\xA5', sizeof(buffer));

  // Work on a copy so the caller's phrase is left alone. The original read at
  // most 127 characters and stopped at an embedded NUL; keep both limits.
  port::SafeCopy(std::span(buffer).first(128), string);
  int length = static_cast<int>(std::string_view(buffer.data()).size());

  // Case-insensitive: fold the phrase to upper case.
  std::ranges::transform(port::MutableCString(buffer.data()), buffer.begin(),
                         absl::ascii_toupper);

  // Replace spaces, control characters and other non-printing bytes with a
  // letter that depends on the position, so the hash only ever sees visible
  // ASCII; bytes >= 0x80 count as non-printing. "7TH GRADE" therefore hashes
  // like "7THDGRADE".
  for (int index = 0; index < length; index++) {
    if (!absl::ascii_isgraph(
            static_cast<unsigned char>(buffer.at(base::ToSize(index))))) {
      buffer.at(base::ToSize(index)) = static_cast<char>('A' + (index % 26));
    }
  }

  // Pad the phrase to at least 16 characters and to a multiple of four, which
  // the four-byte cipher below needs. Each padding letter is derived from the
  // character `length` positions earlier, which past the phrase is earlier
  // padding. An empty phrase reads its own slot instead - the terminator, then
  // the 0xA5 fill - so it still hashes to a non-zero code.
  if (length < 16 || length % 4 != 0) {
    const int maxlen = std::max(((length + 3) / 4) * 4, 16);
    int index = 0;
    for (index = length; index < maxlen; index++) {
      const int mixed =
          static_cast<uint8_t>('?') ^
          static_cast<uint8_t>(buffer.at(base::ToSize(index - length)));
      buffer.at(base::ToSize(index)) =
          static_cast<char>('A' + ((mixed + index) % 26));
    }
    length = index;
    buffer.at(base::ToSize(length)) = '\0';
  }

  // Hash the padded phrase. The CRC is order dependent, so anagrams differ.
  uint32_t code =
      CrcEngine::Compute(std::string_view(buffer.data(), base::ToSize(length)));

  // Kept to XOR back in below.
  const uint32_t copy = code;

  // Fold in the CRC of the reversed phrase. The original meant this to double
  // the work of reversing the CRC, but see the next step.
  std::ranges::reverse(std::span(buffer).first(base::ToSize(length)));
  code ^=
      CrcEngine::Compute(std::string_view(buffer.data(), base::ToSize(length)));

  // XORing the first CRC back out cancels it: from here `code` is just the
  // CRC of the reversed phrase. The historical codes depend on exactly that.
  code = code ^ copy;

  // Feed `code` through the phrase one byte at a time: each byte is XORed with
  // the low byte of the running code, and that byte is rotated back in at the
  // top. The original calls this a decoy cipher ahead of the real one.
  // Put the phrase back in its original order first.
  std::ranges::reverse(std::span(buffer).first(base::ToSize(length)));
  for (int index = 0; index < length; index++) {
    code ^= static_cast<unsigned char>(buffer.at(base::ToSize(index)));
    const auto temp = static_cast<unsigned char>(code);
    buffer.at(base::ToSize(index)) = static_cast<char>(
        static_cast<uint8_t>(buffer.at(base::ToSize(index))) ^ temp);
    // Preserve the original signed shift's sign extension using unsigned
    // operations. A logical shift changes the historical password hashes.
    const uint32_t sign_extension = (code & 0x80000000U) ? 0xFF000000U : 0U;
    code = (code >> 8) | sign_extension;
    code |= uint32_t{temp} << 24;
  }

  // Force a few bits on and a few off, repeating every eight bytes, so that
  // the scrambled bytes lose information. The original meant this to frustrate
  // cryptographic attacks and limited it to under 10% of the bits.
  for (int index = 0; index < length; index++) {
    static constexpr std::array<uint8_t, 8> _lossbits = {
        0x00, 0x08, 0x00, 0x20, 0x00, 0x04, 0x10, 0x00};
    static constexpr std::array<uint8_t, 8> _addbits = {0x10, 0x00, 0x00, 0x80,
                                                        0x40, 0x00, 0x00, 0x04};

    buffer.at(base::ToSize(index)) =
        static_cast<char>(static_cast<uint8_t>(buffer.at(base::ToSize(index))) |
                          _addbits.at(base::ToSize(index) % _addbits.size()));
    buffer.at(base::ToSize(index)) =
        static_cast<char>(static_cast<uint8_t>(buffer.at(base::ToSize(index))) &
                          static_cast<uint8_t>(~_lossbits.at(
                              base::ToSize(index) % _lossbits.size())));
  }

  // Scramble each group of four bytes with a multiply/add/XOR round that
  // uses the bytes themselves as the key. The original calls it a variation
  // on the cipher in PGP; keyed by its own data it is not a real cipher, only
  // one more step an attacker has to invert.
  for (int index = 0; index < length; index += 4) {
    // The original read these bytes as signed char and computed in signed
    // 16-bit values. Unsigned ones give the same result: the transformation
    // below uses only +, * and ^, whose low 8 bits depend only on the low 8
    // bits of their operands, and only those low 8 bits are stored back into
    // the buffer.
    const uint16_t key1 =
        static_cast<unsigned char>(buffer.at(base::ToSize(index)));
    const uint16_t key2 =
        static_cast<unsigned char>(buffer.at(base::ToSize(index + 1)));
    const uint16_t key3 =
        static_cast<unsigned char>(buffer.at(base::ToSize(index + 2)));
    const uint16_t key4 =
        static_cast<unsigned char>(buffer.at(base::ToSize(index + 3)));
    uint16_t val1 = key1;
    uint16_t val2 = key2;
    uint16_t val3 = key3;
    uint16_t val4 = key4;

    val1 = static_cast<uint16_t>(val1 * key1);
    val2 = static_cast<uint16_t>(val2 + key2);
    val3 = static_cast<uint16_t>(val3 + key3);
    val4 = static_cast<uint16_t>(val4 * key4);

    const uint16_t s3 = val3;
    val3 = static_cast<uint16_t>(val3 ^ val1);
    val3 = static_cast<uint16_t>(val3 * key1);
    const uint16_t s2 = val2;
    val2 = static_cast<uint16_t>(val2 ^ val4);
    val2 = static_cast<uint16_t>(val2 + val3);
    val2 = static_cast<uint16_t>(val2 * key3);
    val3 = static_cast<uint16_t>(val3 + val2);

    val1 = static_cast<uint16_t>(val1 ^ val2);
    val4 = static_cast<uint16_t>(val4 ^ val3);

    val2 = static_cast<uint16_t>(val2 ^ s3);
    val3 = static_cast<uint16_t>(val3 ^ s2);

    buffer.at(base::ToSize(index)) = static_cast<char>(val1);
    buffer.at(base::ToSize(index + 1)) = static_cast<char>(val2);
    buffer.at(base::ToSize(index + 2)) = static_cast<char>(val3);
    buffer.at(base::ToSize(index + 3)) = static_cast<char>(val4);
  }

  // The result is the CRC of the scrambled bytes. They can contain zeros, so
  // hash by length rather than as a string.
  return CrcEngine::Compute(
      std::string_view(buffer.data(), base::ToSize(length)));
}
