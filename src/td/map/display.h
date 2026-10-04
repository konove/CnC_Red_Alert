/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\display.h_v   2.15   16 Oct 1995 16:47:42
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DISPLAY.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 1, 1994 *
 *                                                                                             *
 *                  Last Update : May 1, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_MAP_DISPLAY_H_
#define CNC_RED_ALERT_TD_MAP_DISPLAY_H_

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "base/enum_array.h"
#include "base/numeric.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"
#include "td/map/layer.h"
#include "td/map/map.h"
#include "td/objects/house.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace engine::file {
class IniFile;
}  // namespace engine::file

namespace td {

class DisplayClass : public MapClass {
 public:
  void ResetTransientUiState() override;
  // Saved gameplay state; runtime UI resources remain local.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	This indicates the theater that the display is to represent.
  */
  TheaterType Theater = THEATER_NONE;

  /*
  ** The tactical map display position is indicated by the cell of the
  **	upper left hand corner. These should not be altered directly. Use
  **	the Set_Tactical_Position function instead.
  */
  Coordinate TacticalCoord{};

  /*
  **	The dimensions (in cells) of the visible window onto the game map. This
  *tactical *	map is how the player interacts and views the game world.
  */
  int TacLeptonWidth = 0;
  int TacLeptonHeight = 0;

  /*
  **	These layer control elements are used to group the displayable objects
  **	so that proper overlap can be obtained.
  */
  static base::EnumArray<LayerType, LayerClass, kLayerCount> Layer;

  /*
  **	This records the position and shape of a placement cursor to display
  **	over the map. This cursor is used when placing buildings and also used
  **	extensively by the scenario editor.
  */
  Cell ZoneCell{};
  int16_t ZoneOffset{0};
  std::span<const int16_t> CursorSize;
  bool ProximityCheck{false};  // Is proximity check ok?

  /*
  ** This holds the building type that is about to be placed upon the map.
  **	It is only valid during the building placement state. The PendingLegal
  **	flag is updated as the cursor moves and it reflects the legality of
  **	placing the building at the desired location.
  */
  ObjectClass* PendingObjectPtr{nullptr};
  const ObjectTypeClass* PendingObject{nullptr};
  HousesType PendingHouse{HOUSE_NONE};

  static FadingTable FadingBrighten;
  static FadingTable FadingShade;
  static FadingTable FadingLight;
  static base::EnumArray<HousesType, unsigned char[3][256], kHouseCount>
      RemapTables;
  static FadingTable FadingGreen;
  static FadingTable FadingYellow;
  static FadingTable FadingRed;
  static TranslucencyTable<MAGIC_COL_COUNT> TranslucentTable;
  static TranslucencyTable<1> WhiteTranslucentTable;
  static TranslucencyTable<4> MouseTranslucentTable;
  static std::span<const std::byte> TransIconset;
  static TranslucencyTable<USHADOW_COL_COUNT> UnitShadow;
  static TranslucencyTable<1> SpecialGhost;

  //-------------------------------------------------------------------------
  DisplayClass();

  virtual void Read_INI(const engine::file::IniFile& ini);
  virtual void Write_INI(engine::file::IniFile& ini);

  /*
  ** Initialization
  */
  void One_Time() override;                         // One-time inits
  void Init_Clear() override;                       // Clears all to known state
  void Init_IO() override;                          // Inits button list
  void Init_Theater(TheaterType theater) override;  // Theater-specific inits

  /*
  **	General display/map/interface support functionality.
  */
  void AI(engine::window::InputEvent& event, int x, int y) override;
  // Repaints the whole tactical view when a cell was flagged or `forced` is
  // set: terrain, every object in layer order (the ground layer back to front
  // by Sort_Y()), the shroud and the rubber band.
  void Draw_It(PixelView& view, bool forced = false) override;

  /*
  **	Added functionality.
  */
  void Center_Map();
  virtual bool Map_Cell(Cell cell, HouseClass* house);
  virtual Cell Click_Cell_Calc(int x, int y);
  virtual void Help_Text(int /*unused*/, int /*unused*/ = -1,
                         int /*unused*/ = -1, int /*unused*/ = kYellow,
                         bool /*unused*/ = false, int /*unused*/ = 0) {}
  [[nodiscard]] virtual MouseType Get_Mouse_Shape() const = 0;
  virtual bool Scroll_Map(game::DirType facing, int& distance, bool really);
  virtual void Refresh_Cells(Cell cell, std::span<const int16_t> list);
  virtual void Set_View_Dimensions(int x, int y, int width = -1,
                                   int height = -1);

  /*
  **	Pending object placement control.
  */
  virtual void Put_Place_Back(TechnoClass* /*unused*/) {
  }  // Affects 'pending' system.
  void Cursor_Mark(Cell pos, bool on);
  void Set_Cursor_Shape(std::span<const int16_t> list);
  Cell Set_Cursor_Pos(Cell pos = Cell::None());
  static void Get_Occupy_Dimensions(int& w, int& h,
                                    std::span<const int16_t> list);

  /*
  **	Tactical map only functionality.
  */
  virtual void Set_Tactical_Position(Coordinate coord);
  void Refresh_Band();
  void Select_These(Coordinate coord1, Coordinate coord2);
  [[nodiscard]] Coordinate Pixel_To_Coord(int x, int y) const;
  bool Coord_To_Pixel(Coordinate coord, int& x, int& y) const;
  bool Push_Onto_TacMap(Coordinate& source, Coordinate& dest) const;
  static void Remove(ObjectClass* object, LayerType layer);
  static void Submit(ObjectClass* object, LayerType layer);
  Cell Calculated_Cell(SourceType dir, HousesType house);
  [[nodiscard]] bool In_View(Cell cell) const;
  bool Passes_Proximity_Check(const ObjectTypeClass* object);
  ObjectClass* Cell_Object(Cell cell, int x = 0, int y = 0);
  static ObjectClass* Next_Object(ObjectClass* object);
  static ObjectClass* Prev_Object(ObjectClass* object);
  int Cell_Shadow(Cell cell);
  std::span<const int16_t> Text_Overlap_List(const char* text, int x, int y,
                                             int lines = 1);
  [[nodiscard]] bool Is_Spot_Free(Coordinate coord) const;
  [[nodiscard]] Coordinate Closest_Free_Spot(Coordinate coord,
                                             bool any = false) const;
  void Sell_Mode_Control(int control);
  void Repair_Mode_Control(int control);

  virtual void Flag_Cell(Cell cell);
  [[nodiscard]] static bool Is_Cell_Flagged(Cell cell) {
    return CellRedraw.test(base::ToSize(cell.index()));
  }

  /*
  ** Computes starting position based on player's units' Coords.
  */
  void Compute_Start_Pos();

  /*
  **	File I/O.
  */

 protected:
  virtual void Mouse_Right_Press();
  virtual void Mouse_Left_Press(int x, int y);
  virtual void Mouse_Left_Up(bool shadow, ObjectClass* object,
                             ActionType action, bool wwsmall = false);
  virtual void Mouse_Left_Held(int x, int y);
  virtual void Mouse_Left_Release(Cell cell, int x, int y, ObjectClass* object,
                                  ActionType action, bool wwsmall = false);

 public:
  /*
  **	This is the pixel offset for the upper left corner of the tactical map.
  */
  int TacPixelX = 0;
  int TacPixelY = 0;

  /*
  **	This is the coordinate that the tactical map should be in at next
  *available opportunity.
  */
  Coordinate DesiredTacticalCoord{};

  /*
  **	If something in the tactical map is to be redrawn, this flag is set to
  *true.
  */
  bool IsDisplayToRedraw : 1 {true};

  /*
  **	If the player is currently wielding a wrench (to select buildings for
  *repair), *	then this flag is true. In such a state, normal movement and
  *combat orders *	are preempted.
  */
  bool IsRepairMode : 1 {false};

  /*
  **	If the player is currently in "sell back" mode, then this flag will be
  **	true. While in this mode, anything clicked on will be sold back to the
  **	"factory".
  */
  bool IsSellMode : 1 {false};

  /*
  **	If the player is currently in ion cannon targetting mode, then this
  ** flag will be true.  While in this mode, anything clicked on will be
  ** be destroyed by the ION cannon.
  */
  uint8_t IsTargettingMode : 2 {0};

 protected:
  /*
  **	If it is currently in rubber band mode (multi unit selection), then this
  **	flag will be true. While in such a mode, normal input is prempted while
  **	the extended selection is in progress.
  */
  bool IsRubberBand : 1 {false};

  /*
  **	The moment the mouse is held down, this flag gets set. If the mouse is
  *dragged *	a sufficient distance while held down, then true rubber band
  *mode selection *	can begin. Using a minimum distance prevents accidental
  *rubber band selection *	mode from being initiated.
  */
  bool IsTentative : 1 {false};

  /*
  **	This gadget class is used for capturing input to the tactical map. All
  *mouse input *	will be routed through this gadget.
  */
  class TacticalClass : public game::GadgetClass {
   public:
    TacticalClass() noexcept
        : game::GadgetClass(
              0, 0, 0, 0,
              kLeftPress | kLeftRelease | kLeftHeld | kLeftUp | kRightPress,
              true) {}

   protected:
    bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
                int y) override;
  };
  friend class TacticalClass;

  /*
  **	This is the "button" that tracks all input to the tactical map.
  ** It must be available to derived classes, for Save/Load purposes.
  */
  static TacticalClass TacButton;

 private:
  /*
  **	This is a utility flag that is set during the icon draw process only if
  *there *	was at least one shadow icon detected that should be redrawn.
  *When the shadow *	drawing logic is to take place, but this flag is false,
  *then the shadow drawing *	will be skipped since it would perform no
  *function.
  */
  bool IsShadowPresent : 1 = false;

  /*
  **	Rubber band mode consists of stretching a box from the anchor point
  *(specified *	here) to the current cursor position.
  */
  int BandX = 0;
  int BandY = 0;
  int NewX = 0;
  int NewY = 0;

  static std::span<const std::byte> ShadowShapes;
  static TranslucencyTable<SHADOW_COL_COUNT> ShadowTrans;

  void Redraw_Icons(PixelView& view, int draw_flags = 0);
  void Redraw_Shadow(PixelView& view);
  void Redraw_Shadow_Rects(PixelView& view);

  // Draws every object in every layer once, bottom layer first, the ground
  // layer sorted back to front.
  void RenderObjects(PixelView& view);

  // RenderObjects()'s scratch copy of one layer, kept to reuse its storage.
  std::vector<ObjectClass*> draw_list_;

  /*
  **	This bit array is used to flag cells to be redrawn. If the icon needs to
  **	be redrawn for a cell, then the corresponding flag will be true. A
  **	bitset, so that it needs no one-time sizing: a map that never loads its
  **	display data, as in the tests, can still flag cells.
  */
  static std::bitset<MAP_CELL_TOTAL> CellRedraw;
};

#define CELL_BLIT_ONLY 1
#define CELL_DRAW_ONLY 2

extern template void DisplayClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void DisplayClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_MAP_DISPLAY_H_
