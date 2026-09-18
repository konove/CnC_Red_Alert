// The single-player hall of fame: the table of best mission scores kept in
// HALLFAME.DAT and shown on the score screen.

#ifndef CNC_RED_ALERT_RA_HALL_OF_FAME_H_
#define CNC_RED_ALERT_RA_HALL_OF_FAME_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

inline constexpr int kFameRows = 7;
// Ten letters and the terminator.
inline constexpr int kFameNameSize = 11;

// One row of the table.
struct FameEntry {
  std::array<char, kFameNameSize> name{};  // NUL-terminated; empty until typed.
  int32_t score = 0;                       // 0 marks an empty row.
  int32_t level = 0;  // Scenario number the score was earned on.
  int32_t side = 0;   // 0 Allied, 1 Soviet.

  bool operator==(const FameEntry&) const = default;
};

// Best score first.
using FameTable = std::array<FameEntry, kFameRows>;

// Size of one row and of the whole table in HALLFAME.DAT. The file is the
// original game's raw dump of its table: the name, one byte of padding, then
// score, level and side as native 32-bit integers.
inline constexpr std::size_t kFameRecordSize = 24;
inline constexpr std::size_t kFameFileSize = kFameRows * kFameRecordSize;

// Enters a finished mission into `table` if `total` beats one of its rows:
// the rows from there on move down one, the last one dropping off, and the new
// row gets `total`, `level`, `side` and an empty name for the player to fill
// in. Returns the new row's index, or -1 if the score did not make the table,
// which a total of zero or less never does.
int InsertFameScore(FameTable& table, int total, int level, int side);

// Converts the table to and from its HALLFAME.DAT form. Encoding writes every
// byte of `out`; bytes of a name after its terminator are written as zero.
void EncodeFameTable(const FameTable& table,
                     std::span<std::byte, kFameFileSize> out);
FameTable DecodeFameTable(std::span<const std::byte, kFameFileSize> in);

#endif  // CNC_RED_ALERT_RA_HALL_OF_FAME_H_
