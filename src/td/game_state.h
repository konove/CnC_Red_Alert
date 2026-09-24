// File: GameState, the loose run-state the game and its dialogs keep.

#ifndef CNC_RED_ALERT_TD_GAME_STATE_H_
#define CNC_RED_ALERT_TD_GAME_STATE_H_

#include <cstdint>
#include <string>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "sdllib/timer.h"
#include "td/defines.h"

// The flags that say what the game is doing right now: whether it is
// running at all, whether the window has the focus, whether a movie is
// playing, how the scenario ended, and what the modal dialogs need to tell
// their callers. None of it belongs to one subsystem, and none of it is
// saved; what the player chose is in Options and what the scenario is doing
// is in World.
//
// Game owns the one GameState; everything else reaches it through
// TheGameState().
//
// Example:
//   while (TheGameState().active()) { ... }
class GameState {
 public:
  // The length of the version string shown in the menus.
  static constexpr int kVersionTextLength = 16;

  GameState() = default;
  ~GameState() = default;

  GameState(const GameState&) = delete;
  GameState& operator=(const GameState&) = delete;
  GameState(GameState&&) = delete;
  GameState& operator=(GameState&&) = delete;

  // Most of these hand out a reference: they are read and written all over
  // the game loop and the dialogs, and a getter and setter pair for each
  // would only spell the same thing longer.

  // The game loop runs as long as this is true, and this says whether it is
  // playing a scenario rather than sitting in a menu.
  bool& active() ABSL_ATTRIBUTE_LIFETIME_BOUND { return active_; }
  bool& in_main_loop() ABSL_ATTRIBUTE_LIFETIME_BOUND { return in_main_loop_; }

  // Whether the window has the input focus. The loop keeps running while
  // it does not, but the game is paused and the sound is muted.
  bool& in_focus() ABSL_ATTRIBUTE_LIFETIME_BOUND { return in_focus_; }

  // Whether a VQA movie is on screen, whether a key may cut it short, and
  // whether what the last one left on screen should stay there.
  bool& in_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return in_movie_; }
  bool& breakout_allowed() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return breakout_allowed_;
  }
  bool& preserve_movie_screen() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return preserve_movie_screen_;
  }

  // How the scenario ended. The trigger system sets one of these and the
  // main loop acts on it at the end of the frame.
  bool& player_wins() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_wins_; }
  bool& player_loses() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_loses_; }
  bool& player_restarts() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return player_restarts_;
  }

  // Whether units answer an order out loud. Turned off while a whole group
  // is given one order, so it is not acknowledged once per unit.
  bool& allow_voice() ABSL_ATTRIBUTE_LIFETIME_BOUND { return allow_voice_; }

  // Whether the streaming music files were found, and whether sound is on
  // at all.
  bool& scores_present() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scores_present_;
  }
  bool& sound_on() ABSL_ATTRIBUTE_LIFETIME_BOUND { return sound_on_; }

  // The disc the game wants in the drive. -1 means "no particular disc";
  // the port never enumerates a drive, so this only drives the prompt.
  int& required_cd() ABSL_ATTRIBUTE_LIFETIME_BOUND { return required_cd_; }

  // The build this executable shows in the menus.
  auto& version_text() ABSL_ATTRIBUTE_LIFETIME_BOUND { return version_text_; }


  // Which dialog the main loop should pop up on its way round, because a
  // dialog cannot be opened from where the request came from.
  SpecialDialogType& special_dialog() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return special_dialog_;
  }

  // Paces the mission briefing: each line stays up until this runs out or
  // the speech over it finishes.
  CountDownTimerClass& speech_timer() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return speech_timer_;
  }

  // The key a dialog did not understand and handed back to its caller.
  int& unknown_key() ABSL_ATTRIBUTE_LIFETIME_BOUND { return unknown_key_; }

  // Whether the dinosaurs of the Jurassic cheat may appear, and the flag a
  // unit sets while it is carrying the scenario's archive target.
  bool& thingies_enabled() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return thingies_enabled_;
  }
  bool& special_flag() ABSL_ATTRIBUTE_LIFETIME_BOUND { return special_flag_; }

  // Whether Westwood Chat started this game, which skips the menus.
  bool& spawned_from_chat() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return spawned_from_chat_;
  }

  // Whether to behave like version 1.07, which priced the turret and
  // counted silo capacity differently. The command line and CONQUER.INI
  // both ask for it.
  bool& compatibility_v107() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return compatibility_v107_;
  }

  // The directory CONQUER.INI names in place of the CD, used only by the
  // demo build. Empty when the file says nothing.
  std::string& override_path() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return override_path_;
  }

 private:
  bool active_ = false;
  bool in_main_loop_ = false;
  bool in_focus_ = false;

  bool in_movie_ = false;
  bool breakout_allowed_ = true;
  bool preserve_movie_screen_ = false;

  bool player_wins_ = false;
  bool player_loses_ = false;
  bool player_restarts_ = false;

  bool allow_voice_ = true;
  bool scores_present_ = false;
  bool sound_on_ = false;

  int required_cd_ = -1;
  char version_text_[kVersionTextLength]{};

  SpecialDialogType special_dialog_ = SDLG_NONE;
  CountDownTimerClass speech_timer_{int64_t{0}};

  int unknown_key_ = 0;

  bool thingies_enabled_ = false;
  bool special_flag_ = false;
  bool spawned_from_chat_ = false;
  bool compatibility_v107_ = false;
  std::string override_path_;
};

// Returns the GameState that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline GameState& TheGameState() { return base::Installed<GameState>::Get(); }

#endif  // CNC_RED_ALERT_TD_GAME_STATE_H_
