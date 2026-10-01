// The C++ LCW kernels as of fa329aeb, verbatim but for their names; see
// lcw_reference_test_util.h.

#include "engine/codec/lcw_reference_test_util.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/lcw.h"

namespace lcw_reference {

int32_t Uncompress(std::span<const std::byte> source,
                   std::span<std::byte> dest) {
  std::size_t written = 0;
  while (written < dest.size() && !source.empty()) {
    const auto opcode = std::to_integer<unsigned char>(source.front());
    source = source.subspan(1);
    std::size_t count = 0;
    std::size_t offset = 0;
    if (!(opcode & 0x80)) {
      if (source.empty()) {
        break;
      }
      count = (opcode >> 4) + 3;
      const std::size_t distance =
          std::to_integer<unsigned char>(source.front()) +
          ((opcode & 0x0f) * 256);
      source = source.subspan(1);
      if (distance == 0 || distance > written) {
        break;
      }
      offset = written - distance;
    } else if (!(opcode & 0x40)) {
      if (opcode == 0x80) {
        break;
      }
      count = std::min<std::size_t>(opcode & 0x3f, dest.size() - written);
      count = std::min(count, source.size());
      // Forward copies preserve the legacy in-place decompression behavior.
      for (std::size_t i = 0; i < count; ++i) {
        base::At(dest, written++) = base::At(source, i);
      }
      source = source.subspan(count);
      continue;
    } else if (opcode == 0xfe) {
      if (source.size() < 3) {
        break;
      }
      count = std::to_integer<std::size_t>(base::At(source, 0)) +
              (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
      const std::byte value = base::At(source, 2);
      source = source.subspan(3);
      count = std::min(count, dest.size() - written);
      std::ranges::fill(dest.subspan(written, count), value);
      written += count;
      continue;
    } else {
      if (opcode == 0xff) {
        if (source.size() < 2) {
          break;
        }
        count = std::to_integer<std::size_t>(base::At(source, 0)) +
                (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
        source = source.subspan(2);
      } else {
        count = (opcode & 0x3f) + 3;
      }
      if (source.size() < 2) {
        break;
      }
      offset = std::to_integer<std::size_t>(base::At(source, 0)) +
               (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
      source = source.subspan(2);
      // A zero-length copy does not access its offset.
      if (count != 0 && offset >= written) {
        break;
      }
    }
    count = std::min(count, dest.size() - written);
    for (std::size_t i = 0; i < count; ++i) {
      base::At(dest, written++) = base::At(dest, offset++);
    }
  }
  return static_cast<int32_t>(written);
}

int UncompressBounded(std::span<const std::byte> source,
                      std::span<std::byte> dest) {
  const base::ssize in_size = std::ssize(source);
  const base::ssize out_size = std::ssize(dest);
  const auto in_bytes = source;
  const auto out_bytes = dest;
  base::ssize in = 0;
  base::ssize out = 0;

  const auto byte_at = [&](base::ssize offset) {
    return std::to_integer<uint8_t>(base::At(in_bytes, in + offset));
  };
  // Copies forward one byte at a time so a reference that overlaps the bytes
  // it is producing repeats a pattern, as the encoder intends.
  const auto copy_back = [&](base::ssize from, base::ssize count) {
    if (from < 0 || from >= out || count > out_size - out) {
      return false;
    }
    for (base::ssize i = 0; i < count; ++i) {
      base::At(out_bytes, out + i) = base::At(out_bytes, from + i);
    }
    out += count;
    return true;
  };

  while (in < in_size) {
    const uint8_t op_code = byte_at(0);
    ++in;

    if ((op_code & 0x80) == 0) {
      // Short copy, relative to the write position.
      if (in_size - in < 1) {
        return -1;
      }
      const base::ssize offset = ((op_code & 0x0f) * 256) + byte_at(0);
      in += 1;
      if (!copy_back(out - offset, (op_code >> 4) + 3)) {
        return -1;
      }
    } else if ((op_code & 0x40) == 0) {
      if (op_code == 0x80) {
        return static_cast<int>(out);
      }
      // Literal bytes from the source. Byte by byte, because the in-place
      // straw decodes with source and dest in the same buffer.
      const base::ssize count = op_code & 0x3f;
      if (count > in_size - in || count > out_size - out) {
        return -1;
      }
      for (base::ssize i = 0; i < count; ++i) {
        base::At(out_bytes, out + i) = base::At(in_bytes, in + i);
      }
      in += count;
      out += count;
    } else if (op_code == 0xfe) {
      // Long run of one byte value.
      if (in_size - in < 3) {
        return -1;
      }
      const base::ssize count = byte_at(0) + (byte_at(1) * 256);
      if (count > out_size - out) {
        return -1;
      }
      std::fill_n(out_bytes.begin() + out, count, base::At(in_bytes, in + 2));
      in += 3;
      out += count;
    } else if (op_code == 0xff) {
      // Long copy from an absolute output position.
      if (in_size - in < 4) {
        return -1;
      }
      const base::ssize count = byte_at(0) + (byte_at(1) * 256);
      const base::ssize from = byte_at(2) + (byte_at(3) * 256);
      in += 4;
      if (!copy_back(from, count)) {
        return -1;
      }
    } else {
      // Medium copy from an absolute output position.
      if (in_size - in < 2) {
        return -1;
      }
      const base::ssize from = byte_at(0) + (byte_at(1) * 256);
      in += 2;
      if (!copy_back(from, (op_code & 0x3f) + 3)) {
        return -1;
      }
    }
  }

  // The source ran out before the end marker.
  return -1;
}

namespace {

constexpr int kMaxLiteralRun = 63;        // 10xxxxxx holds 1..63 bytes.
constexpr int kMaxShortCount = 10;        // 0xxx: 3..10 bytes.
constexpr int kMaxShortDistance = 0xfff;  // 12-bit relative distance.
constexpr int kMaxMediumCount = 64;       // 11xxxxxx below 0xfe: 3..64.
constexpr int kMaxLongCount = 0xffff;     // 16-bit count for 0xfe and 0xff.
constexpr int kMaxAbsolutePosition = 0xffff;
constexpr int kHashSize = 1 << 12;
constexpr int kMaxChainSteps = 256;

// One way to encode the bytes at the current position.
struct Choice {
  enum class Kind { kLiteral, kShort, kMedium, kLong, kFill };
  using enum Kind;
  Kind kind = kLiteral;
  int count = 1;    // Input bytes covered.
  int from = 0;     // Distance (short) or absolute position (medium, long).
  int savings = 0;  // Input bytes covered minus encoded bytes.
};

// Returns the cheapest back-reference for a match of `length` bytes starting
// `distance` bytes back at absolute `position`.
Choice BestReference(int length, int distance, int position) {
  Choice best;
  if (distance <= kMaxShortDistance) {
    const int count = std::min(length, kMaxShortCount);
    best = {Choice::kShort, count, distance, count - 2};
  }
  if (position <= kMaxAbsolutePosition) {
    const int medium = std::min(length, kMaxMediumCount);
    if (medium - 3 > best.savings) {
      best = {Choice::kMedium, medium, position, medium - 3};
    }
    const int long_count = std::min(length, kMaxLongCount);
    if (long_count - 5 > best.savings) {
      best = {Choice::kLong, long_count, position, long_count - 5};
    }
  }
  return best;
}

int HashAt(std::span<const std::byte> in, int pos) {
  const uint32_t hash =
      (std::to_integer<uint32_t>(base::At(in, base::ToSize(pos))) << 8) ^
      (std::to_integer<uint32_t>(base::At(in, base::ToSize(pos + 1))) << 4) ^
      std::to_integer<uint32_t>(base::At(in, base::ToSize(pos + 2)));
  return static_cast<int>(hash % kHashSize);
}

}  // namespace

// Greedy encoder. A back-reference or fill is only used when it saves at least
// one byte over storing the bytes as literals, which also pays for any literal
// opcode it forces afterwards; so the output never exceeds LcwWorstCaseSize().
int Compress(std::span<const std::byte> in, std::span<std::byte> out) {
  const int length = static_cast<int>(in.size());
  if (out.size() < static_cast<std::size_t>(LcwWorstCaseSize(length))) {
    return -1;
  }
  int written = 0;
  int literal_opcode = -1;  // Index of the open literal run's opcode, or -1.

  // Hash chains over 3-byte prefixes find earlier occurrences quickly.
  std::vector<int> head(kHashSize, -1);
  std::vector<int> previous(base::ToSize(std::max(length, 0)), -1);
  const auto remember = [&](int pos) {
    if (pos + 2 < length) {
      const auto bucket = base::ToSize(HashAt(in, pos));
      previous.at(base::ToSize(pos)) = head.at(bucket);
      head.at(bucket) = pos;
    }
  };

  const auto put = [&](int value) {
    base::At(out, base::ToSize(written++)) = static_cast<std::byte>(value);
  };

  int pos = 0;
  while (pos < length) {
    const int limit = std::min(length - pos, kMaxLongCount);
    Choice best;

    int run = 1;
    while (run < limit && base::At(in, base::ToSize(pos + run)) ==
                              base::At(in, base::ToSize(pos))) {
      ++run;
    }
    if (run - 4 > best.savings) {
      best = {Choice::kFill, run, 0, run - 4};
    }

    if (pos + 2 < length) {
      int steps = 0;
      for (int candidate = head.at(base::ToSize(HashAt(in, pos)));
           candidate >= 0 && steps < kMaxChainSteps;
           candidate = previous.at(base::ToSize(candidate)), ++steps) {
        int match = 0;
        // Overlapping matches are fine: the decoder copies byte by byte.
        while (match < limit && base::At(in, base::ToSize(candidate + match)) ==
                                    base::At(in, base::ToSize(pos + match))) {
          ++match;
        }
        if (match < 3) {
          continue;
        }
        const Choice reference =
            BestReference(match, pos - candidate, candidate);
        if (reference.savings > best.savings) {
          best = reference;
        }
      }
    }

    if (best.savings < 1) {
      if (literal_opcode < 0 ||
          base::At(out, base::ToSize(literal_opcode)) ==
              static_cast<std::byte>(0x80 + kMaxLiteralRun)) {
        literal_opcode = written;
        put(0x80);
      }
      base::At(out, base::ToSize(literal_opcode)) = static_cast<std::byte>(
          std::to_integer<int>(base::At(out, base::ToSize(literal_opcode))) +
          1);
      put(std::to_integer<int>(base::At(in, base::ToSize(pos))));
      remember(pos);
      ++pos;
      continue;
    }

    switch (best.kind) {
      case Choice::kShort:
        put(((best.count - 3) * 16) + (best.from / 256));
        put(best.from % 256);
        break;
      case Choice::kMedium:
        put(0xc0 + (best.count - 3));
        put(best.from % 256);
        put(best.from / 256);
        break;
      case Choice::kLong:
        put(0xff);
        put(best.count % 256);
        put(best.count / 256);
        put(best.from % 256);
        put(best.from / 256);
        break;
      case Choice::kFill:
        put(0xfe);
        put(best.count % 256);
        put(best.count / 256);
        put(std::to_integer<int>(base::At(in, base::ToSize(pos))));
        break;
      case Choice::kLiteral:
      default:
        break;
    }
    for (int i = 0; i < best.count; ++i) {
      remember(pos + i);
    }
    pos += best.count;
    literal_opcode = -1;
  }

  put(0x80);
  return written;
}

}  // namespace lcw_reference
