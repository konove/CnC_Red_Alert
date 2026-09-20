// File: GameState, the loose run-state the game and its dialogs keep.

#ifndef CNC_RED_ALERT_RA_GAME_STATE_H_
#define CNC_RED_ALERT_RA_GAME_STATE_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "ra/defines.h"
#include "winvq/vqa32/vqaplay.h"

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
  GameState() = default;
  ~GameState() = default;

  GameState(const GameState&) = delete;
  GameState& operator=(const GameState&) = delete;
  GameState(GameState&&) = delete;
  GameState& operator=(GameState&&) = delete;

  // Most of these hand out a reference: they are read and written all over
  // the game loop and the dialogs, and a getter and setter pair for each
  // would only spell the same thing longer.

  // The game loop runs as long as this is true.
  bool& active() ABSL_ATTRIBUTE_LIFETIME_BOUND { return active_; }

  // Whether the window has the input focus. The loop keeps running while
  // it does not, but the game is paused and the sound is muted.
  bool& in_focus() ABSL_ATTRIBUTE_LIFETIME_BOUND { return in_focus_; }

  // Whether a VQA movie is on screen, and whether a key may cut it short.
  bool& in_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return in_movie_; }
  bool& breakout_allowed() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return breakout_allowed_;
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

  // The disc the game wants in the drive and the one it believes is there.
  // -1 means "no particular disc"; the port never enumerates a drive, so
  // these only drive the prompt.
  int& required_cd() ABSL_ATTRIBUTE_LIFETIME_BOUND { return required_cd_; }
  int& current_cd() ABSL_ATTRIBUTE_LIFETIME_BOUND { return current_cd_; }

  // How movies are played, read from the INI file and overridden in code.
  VQAConfig& anim_control() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_control_;
  }

  // Which dialog the main loop should pop up on its way round, because a
  // dialog cannot be opened from where the request came from.
  SpecialDialogType& special_dialog() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return special_dialog_;
  }

  // Dialog plumbing: whether the options menu needs redrawing, the key a
  // dialog did not understand and handed back, and the two flags a message
  // box watches so another thread of control can take it down or stop it
  // from appearing.
  bool& redraw_options_menu() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return redraw_options_menu_;
  }
  int& unknown_key() ABSL_ATTRIBUTE_LIFETIME_BOUND { return unknown_key_; }
  bool& cancel_msgbox() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return cancel_msgbox_;
  }
  bool& disable_msgbox() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return disable_msgbox_;
  }

  // Set while an edit field consumes the Tab key, so the dialog does not
  // also move the focus on.
  bool& tab_key_pressed() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return tab_key_pressed_;
  }

 private:
  bool active_ = false;
  bool in_focus_ = false;

  bool in_movie_ = false;
  bool breakout_allowed_ = true;

  bool player_wins_ = false;
  bool player_loses_ = false;
  bool player_restarts_ = false;

  bool allow_voice_ = true;
  bool scores_present_ = false;
  bool sound_on_ = false;

  int required_cd_ = -1;
  int current_cd_ = -1;

  VQAConfig anim_control_{};

  SpecialDialogType special_dialog_ = SDLG_NONE;

  bool redraw_options_menu_ = false;
  int unknown_key_ = 0;
  bool cancel_msgbox_ = false;
  bool disable_msgbox_ = false;
  bool tab_key_pressed_ = false;
};

// Returns the GameState that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline GameState& TheGameState() { return base::Installed<GameState>::Get(); }

#endif  // CNC_RED_ALERT_RA_GAME_STATE_H_
