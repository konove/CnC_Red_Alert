// File: The games' text-window geometry. A "window" here is a rectangle of
// the low-res page, addressed by index into WindowList, not an SDL window.
// PixelView::DrawStamp clips against these rows.

#ifndef CNC_RED_ALERT_SDLLIB_TEXT_WINDOW_H_
#define CNC_RED_ALERT_SDLLIB_TEXT_WINDOW_H_

/*
**	The WindowList[][8] array contains the following elements.  Use these
**	defines when accessing the WindowList.
*/
// Column indices into a WindowList row.
inline constexpr int kWindowX = 0;       // X byte position of left edge.
inline constexpr int kWindowY = 1;       // Y pixel position of top edge.
inline constexpr int kWindowWidth = 2;   // Width in bytes of the window.
inline constexpr int kWindowHeight = 3;  // Height in pixels of the window.
inline constexpr int kWindowFCol = 4;    // Default foreground color.
inline constexpr int kWindowBCol = 5;    // Default background color.
inline constexpr int kWindowCursorX =
    6;  // Current cursor X position (in rows).
inline constexpr int kWindowCursorY =
    7;  // Current cursor Y position (in lines).
inline constexpr int kWindowPadding = 0x1000;

// How many windows the games describe. The first two rows are the screen
// and the error window and the system needs them where they are.
inline constexpr int kWindowCount = 7;

extern int WindowList[kWindowCount][8];
extern unsigned int WinX;
extern unsigned int WinY;
extern unsigned int Window;

#endif  // CNC_RED_ALERT_SDLLIB_TEXT_WINDOW_H_
