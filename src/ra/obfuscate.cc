#include "ra/obfuscate.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "tech/crc.h"

uint32_t Obfuscate(const std::string_view string) {
  std::array<char, 129> buffer{};

  base::FillBytes(base::ObjectBytes(buffer), '\xA5', sizeof(buffer));

  /*
  **	Copy key phrase into a working buffer. This hides any transformation
  *done *	to the string.
  */
  // Retain the 127-character input limit, with room for padding and terminator.
  port::SafeCopy(std::span(buffer).first(128), string);
  int length = static_cast<int>(std::string_view(buffer.data()).size());

  /*
  **	Only upper case letters are significant.
  */
  std::ranges::transform(port::MutableCString(buffer.data()), buffer.begin(),
                         absl::ascii_toupper);

  /*
  **	Ensure that only visible ASCII characters compose the key phrase. This
  **	discourages the direct forced illegal character input method of attack.
  */
  for (int index = 0; index < length; index++) {
    if (!isgraph(buffer.at(base::ToSize(index)))) {
      buffer.at(base::ToSize(index)) = static_cast<char>('A' + (index % 26));
    }
  }

  /*
  **	Increase the strength of even short pass phrases by extending the
  **	length to be at least a minimum number of characters. This helps prevent
  **	a weak pass phrase from compromising the obfuscation process. This
  **	process also forces the key phrase to be an even multiple of four.
  **	This is necessary to support the cypher process that occurs later.
  */
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

  /*
  **	Transform the buffer into a number. This transformation is character
  **	order dependant.
  */
  uint32_t code =
      CrcEngine::Compute(std::string_view(buffer.data(), base::ToSize(length)));

  /*
  **	Record a copy of this initial transformation to be used in a later
  **	self referential transformation.
  */
  const uint32_t copy = code;

  /*
  **	Reverse the character string and combine with the previous
  *transformation. *	This doubles the workload of trying to reverse engineer
  *the CRC calculation.
  */
  std::ranges::reverse(std::span(buffer).first(base::ToSize(length)));
  code ^=
      CrcEngine::Compute(std::string_view(buffer.data(), base::ToSize(length)));

  /*
  **	Perform a self referential transformation. This makes a reverse
  *engineering *	by using a cause and effect attack more difficult.
  */
  code = code ^ copy;

  /*
  **	Unroll and combine the code value into the pass phrase and then perform
  **	another self referential transformation. Although this is a trivial
  *cypher *	process, it gives the sophisticated hacker false hope since the
  *strong *	cypher process occurs later.
  */
  // Restore original string order.
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

  /*
  **	Introduce loss into the vector. This strengthens the key against
  *traditional *	cryptographic attack engines. Since this also weakens
  *the key against *	unconventional attacks, the loss is limited to less than
  *10%.
  */
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

  /*
  **	Perform a general cypher transformation on the vector
  **	and use the vector itself as the cypher key. This is a variation on the
  **	cypher process used in PGP. It is a very strong cypher process with no
  *known *	weaknesses. However, in this case, the cypher key is the vector
  *itself and this *	opens up a weakness against attacks that have access to
  *this transformation *	algorithm. The sheer workload of reversing this
  *transformation should be enough *	to discourage even the most determined
  *hackers.
  */
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

  /*
  **	Convert this final vector into a cypher key code to be
  **	returned by this routine.
  */
  // The transformed data can contain zero bytes; hash all of it.
  return CrcEngine::Compute(
      std::string_view(buffer.data(), base::ToSize(length)));
}
