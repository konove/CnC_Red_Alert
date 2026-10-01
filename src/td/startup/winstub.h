// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_TD_STARTUP_WINSTUB_H_
#define CNC_RED_ALERT_TD_STARTUP_WINSTUB_H_

#include <cstdint>
#include <memory>
#include <span>

#include "engine/gfx/palette.h"
#include "engine/window/window_events.h"

class PixelBuffer;
class PixelView;

namespace game {
class Input;
}  // namespace game

namespace td {

// Writes a line to the platform's debug output.
void CCDebugString(const char* string);

// Pumps the event queue while the window has no focus, so the game does not
// run on while it is in the background.
void Check_For_Focus_Loss();

// The game's side of the window's events: focus changes pause and resume
// the game, and closing the window shuts it down. Game owns the one
// instance and hands it to its Display.
class GameWindowEvents final : public engine::window::WindowEvents {
 public:
  // Feeds the window's keys, clicks and mouse motion to `input`, which must
  // outlive this.
  explicit GameWindowEvents(game::Input& input);

 private:
  void FocusChanged(bool in_focus) override;
  void Quit() override;
};

// Opens the game's window.
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);

// Reads the PCX picture `name` into a new buffer, backed by `buff` when it is
// not empty (at most `size` bytes of it, 0 meaning all). Fills `*palette` with
// the file's palette when `palette` is not nullptr. Returns null if the file
// is missing, malformed or does not fit.
std::unique_ptr<PixelBuffer> Read_PCX_File(const char* name,
                                           engine::gfx::Palette* palette,
                                           std::span<uint8_t> buff,
                                           int32_t size);

// Reads a title screen picture into the given page and its palette.
void Load_Title_Screen(const char* name, PixelView* video_page,
                       engine::gfx::Palette& palette);

// Jolts the visible page up and down `shakes` times, for explosions.
void ShakeScreen(int shakes);

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

}  // namespace td

#endif  // CNC_RED_ALERT_TD_STARTUP_WINSTUB_H_
