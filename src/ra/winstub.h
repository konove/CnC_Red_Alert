// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_RA_WINSTUB_H_
#define CNC_RED_ALERT_RA_WINSTUB_H_

#include "engine/window/display_client.h"

// Pumps the event queue while the window has no focus, so the game does not
// run on while it is in the background.
void Check_For_Focus_Loss();

// Handles what the window's event loop polls - keyboard, mouse, focus and
// quit events - and polls the network sockets on each pass. Game owns the one
// instance and hands it to its Display.
class WindowEvents final : public engine::window::DisplayClient {
 public:
  void HandleEvent(const SDL_Event& event) override;
  // Pumps the sockets. Replaces WSAAsyncSelect, which would send through the
  // Windows event loop.
  void Pump() override;
};

// Opens the game's window.
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_RA_WINSTUB_H_
