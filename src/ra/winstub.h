// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_RA_WINSTUB_H_
#define CNC_RED_ALERT_RA_WINSTUB_H_

#include "engine/window/window_events.h"

class Input;

// Pumps the event queue while the window has no focus, so the game does not
// run on while it is in the background.
void Check_For_Focus_Loss();

// The game's side of the window's events: focus changes pause and resume
// the game, and closing the window shuts it down. It also polls the
// network sockets on each pass of the event loop. Game owns the one
// instance and hands it to its Display.
class GameWindowEvents final : public engine::window::WindowEvents {
 public:
  // Feeds the window's keys, clicks and mouse motion to `input`, which must
  // outlive this.
  explicit GameWindowEvents(Input& input);

  // Pumps the sockets. Replaces WSAAsyncSelect, which would send through the
  // Windows event loop.
  void Pump() override;

 private:
  void FocusChanged(bool in_focus) override;
  void Quit() override;
};

// Opens the game's window.
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_RA_WINSTUB_H_
