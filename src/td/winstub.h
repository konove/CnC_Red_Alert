// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_TD_WINSTUB_H_
#define CNC_RED_ALERT_TD_WINSTUB_H_

#include <span>

class GraphicViewPortClass;

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
void Load_Title_Screen(const char* name, GraphicViewPortClass* video_page,
                       std::span<unsigned char> palette);

// Applies a palette change a movie queued from its own thread, or drops one
// that was queued and never applied.
void Check_VQ_Palette_Set();
void Discard_VQ_Palette_Change();

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_TD_WINSTUB_H_
