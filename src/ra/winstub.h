// File: The hooks the platform layer provides to the rest of the game.

#ifndef CNC_RED_ALERT_RA_WINSTUB_H_
#define CNC_RED_ALERT_RA_WINSTUB_H_

// Reports that an allocation failed and does not return. Set as the memory
// system's error handler.
[[noreturn]] void Memory_Error_Handler();

// Writes a line to the platform's debug output.
void WWDebugString(const char* string);

// Pumps the event queue while the window has no focus, so the game does not
// run on while it is in the background.
void Check_For_Focus_Loss();

// Opens the game's window.
void Create_Main_Window(void* instance, int command_show, int width,
                        int height);

// Called when the window loses and regains the input focus.
void Focus_Loss();
void Focus_Restore();

#endif  // CNC_RED_ALERT_RA_WINSTUB_H_
