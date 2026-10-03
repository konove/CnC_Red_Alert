#ifndef CNC_RED_ALERT_GAME_CORE_DISPLAY_CONSTANTS_H_
#define CNC_RED_ALERT_GAME_CORE_DISPLAY_CONSTANTS_H_

// File: The size of a map cell in pixels and in leptons, shared by the
// two games' map display, coordinate conversion and rules defaults.

namespace game {

// A terrain icon is one cell-sized tile of a template, so the ICON_* and
// CELL_* names are the same sizes; the code uses whichever fits what it is
// measuring. Cells are square, and the W and H values are equal.
#define ICON_PIXEL_W 24
#define ICON_PIXEL_H 24

// A lepton is the simulation's unit of distance: 1/256 of a cell, so the low
// byte of a coordinate component is the offset within the cell.
#define ICON_LEPTON_W 256
#define ICON_LEPTON_H 256
#define CELL_PIXEL_W ICON_PIXEL_W
#define CELL_PIXEL_H ICON_PIXEL_H
#define CELL_LEPTON_W ICON_LEPTON_W
#define CELL_LEPTON_H ICON_LEPTON_H

// Leptons per pixel, truncated: the exact ratio is 256/24 = 10.67. It is good
// for "has this moved about a pixel" tests; an exact conversion has to multiply
// before dividing, as the games' Lepton_To_Pixel() and Pixel_To_Lepton() do.
#define PIXEL_LEPTON_W (ICON_LEPTON_W / ICON_PIXEL_W)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_CORE_DISPLAY_CONSTANTS_H_
