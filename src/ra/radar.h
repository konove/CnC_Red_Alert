/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/RADAR.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RADAR.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/15/94 *
 *                                                                                             *
 *                  Last Update : December 15, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_RADAR_H_
#define CNC_RED_ALERT_RA_RADAR_H_

#include <cstdint>

#include "ra/defines.h"
#include "ra/display.h"
#include "ra/gadget.h"
#include "ra/house.h"
#include "ra/jshell.h"
#include "sdllib/keyboard.h"

class RadarClass : public DisplayClass {
 public:
  // Resets transient UI state after loading, preserving saved game state.
  void ResetTransientUiState() override;

  // Saved-game state; defined in iomap.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  RadarClass();

  /*
  **	The dimensions and coordinates of the radar map.
  */
  int RadX = 0;
  int RadOffX = 0;
  int RadY = 0;
  int RadOffY = 0;
  int RadWidth = 0;
  int RadHeight = 0;
  int RadIWidth = 0;
  int RadIHeight = 0;
  int RadPWidth = 0;
  int RadPHeight = 0;

  /*
  ** Initialization
  */
  void One_Time() override;    // One-time inits
  void Init_Clear() override;  // Clears all to known state

  void Flag_Cell(CELL cell) override;
  bool Map_Cell(CELL cell, HouseClass* house) override;
  virtual bool Jam_Cell(CELL cell, HouseClass* house);
  virtual bool UnJam_Cell(CELL cell, HouseClass* house);
  [[nodiscard]] CELL Click_Cell_Calc(int x, int y) const override;
  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool forced = false) override;
  void Refresh_Cells(CELL cell, const int16_t* list) override;
  void Set_Map_Dimensions(int x, int y, int w, int h) override;
  void Set_Tactical_Position(COORDINATE coord) override;
  void Zoom_Mode(CELL cell);
  int Click_In_Radar(int& x, int& y, bool change = false) const;
  void Cell_XY_To_Radar_Pixel(int cellx, int celly, int& x, int& y) const;

  [[nodiscard]] bool Is_Zoomable() const;
  void Set_Radar_Position(CELL cell);
  [[nodiscard]] CELL Radar_Position() const;
  bool Radar_Activate(int control);
  void Plot_Radar_Pixel(CELL cell);
  void Radar_Pixel(CELL cell);
  void Coord_To_Radar_Pixel(COORDINATE coord, int& x, int& y);
  void Cursor_Cell(CELL cell, bool value);
  void Mark_Radar(int x1, int y1, int x2, int y2, bool value, int barlen);
  void Radar_Cursor(bool forced = false);
  void Render_Terrain(CELL cell, int x, int y, int size) const;
  [[nodiscard]] bool Cell_On_Radar(CELL cell) const;
  static void Render_Infantry(CELL cell, int x, int y, int size);
  void Render_Overlay(CELL cell, int x, int y, int size);
  void Radar_Anim();
  [[nodiscard]] bool Is_Radar_Active() const;
  [[nodiscard]] bool Is_Radar_Existing() const;

  /*
  ** Toggles player names on & off
  */
  void Player_Names(bool on);
  [[nodiscard]] int Is_Player_Names() const { return IsPlayerNames; }
  [[nodiscard]] bool Spying_On_House() const { return IsHouseSpy; }
  void Draw_Names() const;
  bool Draw_House_Info();
  [[nodiscard]] int Is_Zoomed() const { return IsZoomed; }
  [[nodiscard]] bool Get_Jammed() const;
  void Set_Jammed(bool jam) { IsRadarJammed = jam; }
  bool Spy_Next_House();
  void Activate_Pulse();

 protected:
  /*
  **	Radar map constant values.
  */
  enum RadarClassEnums { RADAR_ACTIVATED_FRAME = 22, MAX_RADAR_FRAMES = 41 };

  // If the radar map must be completely redrawn, then this flag will be true.
  // Typical causes of this would be when the radar first appears, or when the
  // screen has been damaged.
  bool IsRadarToRedraw : 1 {false};
  bool RadarCursorRedraw : 1 {false};

  /*
  **	If the radar map is visible then this flag is true.
  */
  bool DoesRadarExist : 1 {false};
  bool IsRadarActive : 1 {false};
  bool IsRadarActivating : 1 {false};
  bool IsRadarDeactivating : 1 {false};
  bool IsRadarJammed : 1 {false};

  /*
  ** Flag to tell whether sonar pulse should be displayed on radar map
  */
  bool IsPulseActive : 1 {false};
  int RadarPulseFrame{0};

  /*
  ** Special radar frame is set when a new location is selected on the
  ** radar map.  It counts down through the special radar cursors until
  ** either the radar cursor becomes normal or the radar cursor is moved
  ** again.
  */
  int SpecialRadarFrame{0};
  int RadarAnimFrame{0};

  static const void* RadarAnim;
  static const void* RadarPulse;
  static const void* RadarFrame;

  /*
  **	This gadget class is used for capturing input to the tactical map. All
  *mouse input *	will be routed through this gadget.
  */
  class RTacticalClass : public GadgetClass {
   public:
    RTacticalClass() noexcept
        : GadgetClass(0, 0, 0, 0,
                      LEFTPRESS | LEFTRELEASE | LEFTHELD | LEFTUP | RIGHTPRESS,
                      true) {}

   protected:
    bool Action(unsigned flags, KeyNumType& key) override;
    friend class RadarClass;
  };
  friend class RTacticalClass;

  /*
  **	This is the "button" that tracks all input to the tactical map.
  ** It must be available to derived classes, for Save/Load purposes.
  */
  static RTacticalClass RadarButton;

 private:
  /*
  **	The current radar position as the upper left corner cell for the
  **	radar map display. The width and height is controlled by the
  **	actual dimensions of the radar map display box (in pixels).
  */
  int RadarX{0};
  int RadarY{0};
  int RadarCellWidth{0};
  int RadarCellHeight{0};
  int RadarCell{0};

  /*
  **	This is the origin (pixel offsets) for the upper left corner
  **	of the radar map within the full radar map area of the screen.
  **	This is biased so that the radar map, when smaller than full
  **	size will appear centered.
  */
  int BaseX{0};
  int BaseY{0};

  int RadarWidth{0};
  int RadarHeight{0};

  /*
  **	If the radar map is in zoom mode, then this value will be true.
  */
  bool IsZoomed : 1 {true};

  /*
  ** This flag is true if the radar map is in its special show-the-player
  ** names mode.
  */
  bool IsPlayerNames : 1 {false};

  /*
  ** This flag is true if the radar map is in its special show-the-units
  ** of-another-house mode.
  */
  bool IsHouseSpy : 1 {false};

  /*
  **	This is the zoom factor to use. This value is the number of pixels wide
  **	each cell will occupy on the radar map. Completely zoomed out would be a
  **	value of 1.
  */
  int ZoomFactor{0};

  /*
  ** If we're spying on a house's radar facility, this field shows the
  ** name of the house we're spying on.
  */
  HousesType SpyingOn{HOUSE_SPAIN};

  /*
  **	This is the list of radar pixels that need to be updated. Only a partial
  **	list is maintained for maximum speed.
  */
  int PixelPtr{0};
  enum PixelStackEnums { PIXELSTACK = 400 };
  CELL PixelStack[PIXELSTACK]{};
};

class ArchiveReader;
class ArchiveWriter;
extern template void RadarClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void RadarClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_RADAR_H_
