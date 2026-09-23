// File: SessionClass, the settings of the multiplayer game being played.

#ifndef CNC_RED_ALERT_TD_SESSION_H_
#define CNC_RED_ALERT_TD_SESSION_H_

#include <cstdint>
#include <memory>
#include <string>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "td/defines.h"
#include "td/msglist.h"
#include "td/vector.h"
#include "tech/byte_stream.h"

// What kind of game is being played and, when it is a multiplayer one, who
// is in it and under what rules. Red Alert gathers the same state into a
// SessionClass of its own; Tiberian Dawn kept it in three dozen MPlayer*
// globals, so the members here drop that prefix -- the class name says it.
//
// None of this is written to a saved game: Tiberian Dawn restores a network
// game from the connections it still holds, not from the file.
//
// Game owns the one SessionClass; everything else reaches it through
// TheSession().
//
// Example:
//   if (TheSession().type() != GAME_NORMAL) { ... }
class SessionClass {
 public:
  // Index 0 is for scenarios without bases, index 1 for scenarios with
  // them, which is how the unit count limits are looked up.
  static constexpr int kBaseSettingCount = 2;

  SessionClass() = default;
  ~SessionClass() = default;

  SessionClass(const SessionClass&) = delete;
  SessionClass& operator=(const SessionClass&) = delete;
  SessionClass(SessionClass&&) = delete;
  SessionClass& operator=(SessionClass&&) = delete;

  // Most of these hand out a reference: the dialogs and the network code
  // read and write them all over, and a getter and setter pair for each
  // would only spell the same thing longer.

  // Which kind of game this is, and how its packets are sent.
  GameType& type() ABSL_ATTRIBUTE_LIFETIME_BOUND { return type_; }
  CommProtocolType& comm_protocol() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return comm_protocol_;
  }

  // The local player: the name typed into the dialog, the house picked, the
  // colour asked for and the one actually assigned, and the id the other
  // machines know this player by.
  auto& player_name() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_name_; }
  HousesType& house() ABSL_ATTRIBUTE_LIFETIME_BOUND { return house_; }
  int& preferred_color() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return preferred_color_;
  }
  int& color_index() ABSL_ATTRIBUTE_LIFETIME_BOUND { return color_index_; }
  unsigned char& local_id() ABSL_ATTRIBUTE_LIFETIME_BOUND { return local_id_; }

  // The name of the game as it is advertised on the network. It does not
  // include the "'s Game" suffix, so comparing it with player_name() says
  // whether this machine started the game.
  auto& game_name() ABSL_ATTRIBUTE_LIFETIME_BOUND { return game_name_; }

  // The players in the game, in the order their events execute. An id
  // packs the player's house in its low four bits and colour index in its
  // high four, and doubles as the IPX connection id.
  auto& player_ids() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_ids_; }
  auto& player_houses() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_houses_; }
  auto& player_names() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_names_; }
  int& player_count() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_count_; }
  int& max_players() ABSL_ATTRIBUTE_LIFETIME_BOUND { return max_players_; }

  // The two remap tables a player's colour index selects: one for drawing
  // and one for printing text.
  auto& graphic_colors() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return graphic_colors_;
  }
  auto& text_colors() ABSL_ATTRIBUTE_LIFETIME_BOUND { return text_colors_; }

  // The multiplayer scenarios offered in the dialog, by description and by
  // file number, and which row of the list is selected.
  DynamicVectorClass<char*>& scenarios() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scenarios_;
  }
  DynamicVectorClass<int>& scenario_files() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scenario_files_;
  }
  int& scenario_index() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scenario_index_;
  }

  // The rules the game was set up with.
  int& bases() ABSL_ATTRIBUTE_LIFETIME_BOUND { return bases_; }
  int& credits() ABSL_ATTRIBUTE_LIFETIME_BOUND { return credits_; }
  int& tiberium() ABSL_ATTRIBUTE_LIFETIME_BOUND { return tiberium_; }
  int& crates() ABSL_ATTRIBUTE_LIFETIME_BOUND { return crates_; }
  int& ghosts() ABSL_ATTRIBUTE_LIFETIME_BOUND { return ghosts_; }
  bool& solo() ABSL_ATTRIBUTE_LIFETIME_BOUND { return solo_; }

  // How many units each player starts with in a scenario without bases,
  // and the limits the slider allows, indexed by whether bases are on.
  int& unit_count() ABSL_ATTRIBUTE_LIFETIME_BOUND { return unit_count_; }
  auto& unit_count_min() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_count_min_;
  }
  auto& unit_count_max() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_count_max_;
  }

  // How many frames ahead of the current one an event is scheduled, set by
  // the RESPONSE_TIME event, and how many frames apart packets go out.
  int& max_ahead() ABSL_ATTRIBUTE_LIFETIME_BOUND { return max_ahead_; }
  int32_t& frame_send_rate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return frame_send_rate_;
  }

  // What the other machines report their logic takes, in the same order as
  // player_ids(), and the frame rate the slowest of them allows.
  auto& their_process_time() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return their_process_time_;
  }
  int& desired_frame_rate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return desired_frame_rate_;
  }

  // How long the game's own logic takes, with no packet handling and no
  // artificial delay: accumulated ticks over the frames they were measured
  // across.
  int& process_ticks() ABSL_ATTRIBUTE_LIFETIME_BOUND { return process_ticks_; }
  int& process_frames() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return process_frames_;
  }

  // The messages sent and received, and the last one this machine sent,
  // which the computer players quote back.
  MessageListClass& messages() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return messages_;
  }
  auto& last_message() ABSL_ATTRIBUTE_LIFETIME_BOUND { return last_message_; }

  // Whether the computer players attack all at once rather than trickling
  // their units out, and whether the local player has been defeated but is
  // still watching.
  int& blitz() ABSL_ATTRIBUTE_LIFETIME_BOUND { return blitz_; }
  bool& obi_wan() ABSL_ATTRIBUTE_LIFETIME_BOUND { return obi_wan_; }

  // The score screen's running tally across the games played this run.
  auto& scores() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scores_; }
  int& games_played() ABSL_ATTRIBUTE_LIFETIME_BOUND { return games_played_; }
  int& score_count() ABSL_ATTRIBUTE_LIFETIME_BOUND { return score_count_; }
  int& current_game() ABSL_ATTRIBUTE_LIFETIME_BOUND { return current_game_; }

  // The recording of the game.
  //
  // record_file_name is the name of the file recorded to or played back
  // from. record_stream is that file while a recording or playback is in
  // progress, and nullptr otherwise. record_game is true while recording;
  // playback_game is true while playing a recording back. super_record
  // flushes the recording to disk every frame, so it survives a crash.
  // allow_attract lets an idle menu start a playback.
  [[nodiscard]] const std::string& record_file_name() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return record_file_name_;
  }
  std::unique_ptr<ByteStream>& record_stream() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return record_stream_;
  }
  bool& record_game() ABSL_ATTRIBUTE_LIFETIME_BOUND { return record_game_; }
  int& super_record() ABSL_ATTRIBUTE_LIFETIME_BOUND { return super_record_; }
  bool& playback_game() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return playback_game_;
  }
  bool& allow_attract() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return allow_attract_;
  }

 private:
  GameType type_ = GAME_NORMAL;
  CommProtocolType comm_protocol_{};

  char player_name_[MPLAYER_NAME_MAX]{};
  HousesType house_{};
  int preferred_color_ = 0;
  int color_index_ = 0;
  unsigned char local_id_ = 0;

  char game_name_[MPLAYER_NAME_MAX]{};

  unsigned char player_ids_[MAX_PLAYERS]{};
  HousesType player_houses_[MAX_PLAYERS]{};
  char player_names_[MAX_PLAYERS][MPLAYER_NAME_MAX]{};
  int player_count_ = 0;
  int max_players_ = 4;

  int graphic_colors_[MAX_MPLAYER_COLORS] = {
      5,    // Yellow
      127,  // Red
      135,  // BlueGreen
      26,   // Orange
      4,    // Green
      202   // Blue-Grey
  };
  int text_colors_[MAX_MPLAYER_COLORS] = {
      kCcGdiColor,   // Yellow
      kCcNodColor,   // Red
      kCcBlueGreen,  // BlueGreen
      kCcOrange,     // Orange
      kCcGreen,      // Green
      kCcBlueGrey,   // Blue
  };

  DynamicVectorClass<char*> scenarios_;
  DynamicVectorClass<int> scenario_files_;
  int scenario_index_ = 0;

  int bases_ = 0;
  int credits_ = 0;
  int tiberium_ = 0;
  int crates_ = 0;
  int ghosts_ = 0;
  bool solo_ = false;

  int unit_count_ = 10;
  int unit_count_min_[kBaseSettingCount] = {1, 0};
  int unit_count_max_[kBaseSettingCount] = {50, 12};

  int max_ahead_ = 3;
  int32_t frame_send_rate_ = 0;

  int their_process_time_[MAX_PLAYERS - 1]{};
  int desired_frame_rate_ = 0;

  int process_ticks_ = 0;
  int process_frames_ = 0;

  MessageListClass messages_;
  char last_message_[MAX_MESSAGE_LENGTH]{};

  int blitz_ = 0;
  bool obi_wan_ = false;

  MPlayerScoreType scores_[MAX_MULTI_NAMES]{};
  int games_played_ = 0;
  int score_count_ = 0;
  int current_game_ = 0;

  std::string record_file_name_ = "RECORD.BIN";
  std::unique_ptr<ByteStream> record_stream_;
  bool record_game_ = false;
  int super_record_ = 0;
  bool playback_game_ = false;
  bool allow_attract_ = false;
};

// Returns the SessionClass that Game installed. CHECK-fails outside a
// Game's lifetime unless a test installed its own.
inline SessionClass& TheSession() {
  return base::Installed<SessionClass>::Get();
}

#endif  // CNC_RED_ALERT_TD_SESSION_H_
