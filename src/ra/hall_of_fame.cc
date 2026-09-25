#include "ra/hall_of_fame.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "base/buffer.h"

namespace {

constexpr std::size_t kScoreOffset = 12;
constexpr std::size_t kLevelOffset = 16;
constexpr std::size_t kSideOffset = 20;

void PutInt(std::span<std::byte> record, std::size_t offset, int32_t value) {
  std::ranges::copy(base::ObjectBytes(value),
                    record.subspan(offset, sizeof(value)).begin());
}

int32_t GetInt(std::span<const std::byte> record, std::size_t offset) {
  int32_t value = 0;
  std::ranges::copy(record.subspan(offset, sizeof(value)),
                    base::ObjectBytes(value).begin());
  return value;
}

}  // namespace

int InsertFameScore(FameTable& table, int total, int level, int side) {
  for (std::size_t i = 0; i < table.size(); i++) {
    if (total > table.at(i).score) {
      const auto tail = std::span(table).subspan(i);
      // Move the rows down one; the last drops off. (libstdc++ 13 has no
      // std::ranges::shift_right.)
      std::ranges::copy_backward(tail.first(tail.size() - 1), tail.end());
      tail.front() =
          FameEntry{.name = {}, .score = total, .level = level, .side = side};
      return static_cast<int>(i);
    }
  }
  return -1;
}

void EncodeFameTable(const FameTable& table,
                     std::span<std::byte, kFameFileSize> out) {
  std::ranges::fill(out, std::byte{0});
  for (std::size_t i = 0; i < table.size(); i++) {
    const FameEntry& entry = table.at(i);
    const auto record = out.subspan(i * kFameRecordSize, kFameRecordSize);
    // Copy the name up to its terminator only, so that whatever was typed and
    // then backspaced over does not reach the file.
    const std::string_view buffer(entry.name.data(), entry.name.size());
    std::ranges::transform(buffer.substr(0, buffer.find('\0')), record.begin(),
                           [](char c) { return static_cast<std::byte>(c); });
    PutInt(record, kScoreOffset, entry.score);
    PutInt(record, kLevelOffset, entry.level);
    PutInt(record, kSideOffset, entry.side);
  }
}

FameTable DecodeFameTable(std::span<const std::byte, kFameFileSize> in) {
  FameTable table;
  for (std::size_t i = 0; i < table.size(); i++) {
    FameEntry& entry = table.at(i);
    const auto record = in.subspan(i * kFameRecordSize, kFameRecordSize);
    std::ranges::transform(record.first(entry.name.size()), entry.name.begin(),
                           [](std::byte b) { return static_cast<char>(b); });
    // Files written by older builds can hold garbage after a short name, and
    // nothing guarantees the terminator.
    entry.name.back() = '\0';
    entry.score = GetInt(record, kScoreOffset);
    entry.level = GetInt(record, kLevelOffset);
    entry.side = GetInt(record, kSideOffset);
  }
  return table;
}
