#ifndef CNC_RED_ALERT_GAME_CORE_CELL_SIZE_H_
#define CNC_RED_ALERT_GAME_CORE_CELL_SIZE_H_

// File: The size of a map cell in pixels and in leptons, shared by the
// two games' map display, coordinate conversion and rules defaults.

namespace game {

// A cell is square, 24 pixels a side on screen. A terrain icon (one tile of a
// template) covers exactly one cell, so these also size the icons.
inline constexpr int kCellWidthPixels = 24;
inline constexpr int kCellHeightPixels = 24;

// A lepton is the simulation's unit of distance: 1/256 of a cell, so the low
// byte of a coordinate component is the offset within the cell.
inline constexpr int kCellWidthLeptons = 256;
inline constexpr int kCellHeightLeptons = 256;

// Leptons per pixel, truncated: the exact ratio is 256/24 = 10.67. It is good
// for "has this moved about a pixel" tests; an exact conversion has to multiply
// before dividing, as the games' Lepton_To_Pixel() and Pixel_To_Lepton() do.
inline constexpr int kLeptonsPerPixel = kCellWidthLeptons / kCellWidthPixels;

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_CELL_SIZE_H_
