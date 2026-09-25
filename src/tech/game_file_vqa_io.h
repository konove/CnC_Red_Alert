#ifndef CNC_RED_ALERT_TECH_GAME_FILE_VQA_IO_H_
#define CNC_RED_ALERT_TECH_GAME_FILE_VQA_IO_H_

// File: GameFileVqaIo, the VQA player's file source for both games.

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include "engine/base/types.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "winvq/vqa32/vqaio.h"

// Serves movie data through the game's file lookup (OpenGameFile), so a
// movie plays the same whether it is a loose file or packed in a mixfile,
// cached or on disk. Pass one to VqaPlayer::Open(); it must outlive the
// player.
class GameFileVqaIo final : public VqaIo {
 public:
  GameFileVqaIo() = default;
  ~GameFileVqaIo() override = default;

  GameFileVqaIo(const GameFileVqaIo&) = delete;
  GameFileVqaIo& operator=(const GameFileVqaIo&) = delete;
  GameFileVqaIo(GameFileVqaIo&&) = delete;
  GameFileVqaIo& operator=(GameFileVqaIo&&) = delete;

  bool Open(std::string_view name) override;
  bool Read(std::span<std::byte> buffer) override;
  bool Seek(base::ssize offset, SeekOrigin origin) override;
  void Close() override { stream_.reset(); }

 private:
  std::unique_ptr<ByteStream> stream_;  // Null when no movie is open.
};

#endif  // CNC_RED_ALERT_TECH_GAME_FILE_VQA_IO_H_
