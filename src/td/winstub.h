// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_TD_WINSTUB_H_
#define CNC_RED_ALERT_TD_WINSTUB_H_

#include <span>

class PixelView;

// Reports that an allocation failed and does not return. Set as the memory
// system's error handler.
[[noreturn]] void Memory_Error_Handler();

// Writes a line to the platform's debug output.
void CCDebugString(const char* string);

// Pumps the event queue while the window has no focus, so the game does not
// run on while it is in the background.
void Check_For_Focus_Loss();

// Opens the game's window.
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);

// Reads a title screen picture into the given page and its palette.
void Load_Title_Screen(const char* name, PixelView* video_page,
                       std::span<unsigned char> palette);

// Jolts the visible page up and down `shakes` times, for explosions.
void ShakeScreen(int shakes);

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_TD_WINSTUB_H_
