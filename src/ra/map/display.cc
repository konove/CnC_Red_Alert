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

/* $Header: /CounterStrike/DISPLAY.CPP 3     3/09/97 8:04p Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DISPLAY.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : October 20, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * DisplayClass::Compute_Start_Pos -- Computes player's start pos
 *from unit coords.          * DisplayClass::AI -- Handles the maintenance tasks
 *for the map display.                    * DisplayClass::All_To_Look -- Direct
 *all objects to look around for the player.            *
 *   DisplayClass::Calculated_Cell -- Fetch a map cell based on specified
 *method.              * DisplayClass::Cell_Object -- Determines what has been
 *clicked on.                         * DisplayClass::Cell_Shadow   -- Determine
 *what shadow icon to use for the cell.            * DisplayClass::Center_Map --
 *Centers the map about the currently selected objects          *
 *   DisplayClass::Click_Cell_Calc -- Determines cell from screen X & Y. *
 *   DisplayClass::Closest_Free_Spot -- Finds the closest cell sub spot that is
 *free.          * DisplayClass::Coord_To_Pixel -- Determines X and Y pixel
 *coordinates.                     * DisplayClass::Cursor_Mark -- Set or resets
 *the cursor display flag bits.                  * DisplayClass::DisplayClass --
 *Default constructor for display class.                      *
 *   DisplayClass::Draw_It -- Draws the tactical map. *
 *   DisplayClass::Encroach_Shadow -- Causes the shadow to creep back by one
 *cell.             * DisplayClass::Flag_Cell -- Flag the specified cell to be
 *redrawn.                         * DisplayClass::Flag_To_Redraw -- Flags the
 *display so that it will be redrawn as soon as poss*
 *   DisplayClass::Get_Occupy_Dimensions -- computes width & height of the given
 *occupy list   * DisplayClass::Good_Reinforcement_Cell -- Checks cell for
 *renforcement legality.           * DisplayClass::In_View -- Determines if cell
 *is visible on screen.                         * DisplayClass::Init_Clear --
 *Clears the display to a known state.                          *
 *   DisplayClass::Init_IO -- Creates the map's button list *
 *   DisplayClass::Init_Theater -- Theater-specific initialization *
 *   DisplayClass::Is_Spot_Free -- Determines if cell sub spot is free of
 *occupation.          * DisplayClass::Map_Cell -- Mark specified cell as having
 *been mapped.                      * DisplayClass::Mouse_Left_Held -- Handles
 *the left button held down.                       *
 *   DisplayClass::Mouse_Left_Press -- Handles the left mouse button press. *
 *   DisplayClass::Mouse_Left_Release -- Handles the left mouse button release.
 ** DisplayClass::Mouse_Left_Up -- Handles the left mouse "cruising" over the
 *map.            * DisplayClass::Mouse_Right_Press -- Handles the right mouse
 *button press.                  * DisplayClass::Next_Object -- Searches for
 *next object on display.                         * DisplayClass::One_Time --
 *Performs any special one time initializations.                  *
 *   DisplayClass::Passes_Proximity_Check -- Determines if building placement is
 *near friendly sq* DisplayClass::Pixel_To_Coord -- converts screen coord to
 *Coordinate                       * DisplayClass::Prev_Object -- Searches for
 *the previous object on the map.                 * DisplayClass::Read_INI --
 *Reads map control data from INI file.                           *
 *   DisplayClass::Redraw_Icons -- Draws all terrain icons necessary. *
 *   DisplayClass::Redraw_Shadow -- Draw the shadow overlay. *
 *   DisplayClass::Refresh_Band -- Causes all cells under the rubber band to be
 *redrawn.       * DisplayClass::Refresh_Cells -- Redraws all cells in list. *
 *   DisplayClass::Remove -- Removes a game object from the rendering system. *
 *   DisplayClass::Repair_Mode_Control -- Controls the repair mode. *
 *   DisplayClass::Scroll_Map -- Scroll the tactical map in desired direction. *
 *   DisplayClass::Select_These -- All selectable objects in region are
 *selected.              * DisplayClass::Sell_Mode_Control -- Controls the sell
 *mode.                                * DisplayClass::Set_Cursor_Pos --
 *Controls the display and animation of the tac cursor.     *
 *   DisplayClass::Set_Cursor_Shape -- Changes the shape of the terrain square
 *cursor.         * DisplayClass::Set_Tactical_Position -- Sets the tactical
 *view position.                   * DisplayClass::Set_View_Dimensions -- Sets
 *the tactical display screen coordinates.        * DisplayClass::Shroud_Cell --
 *Returns the specified cell into the shrouded condition.      *
 *   DisplayClass::Submit -- Adds a game object to the map rendering system. *
 *   DisplayClass::TacticalClass::Action -- Processes input for the tactical
 *map.              * DisplayClass::Text_Overlap_List -- Creates cell overlap
 *list for specified text string.   * DisplayClass::Write_INI -- Write the map
 *data to the INI file specified.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/map/display.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/bit.h"
#include "base/buffer.h"
#include "base/clip.h"
#include "base/enum_array.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/point.h"
#include "base/rect.h"
#include "base/strings/number_parse.h"
#include "base/trig.h"
#include "engine/file/disk_file.h"  // IWYU pragma: keep - used by the debug-only loose-file override below.
#include "engine/file/ini_file.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/cell_size.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "game/sim/mph.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/assets.h"
#include "ra/assets/audio.h"
#include "ra/assets/compat.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/fading_table_builders.h"
#include "ra/core/object_heaps.h"
#include "ra/core/random_pick.h"
#include "ra/core/target.h"
#include "ra/data/ini_types.h"
#include "ra/data/rules.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/layer.h"
#include "ra/map/logic.h"
#include "ra/map/map.h"
#include "ra/map/selection.h"
#include "ra/map/vortex.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/foot.h"
#include "ra/objects/infantry.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"
#include "ra/screens/goptions.h"
#include "ra/startup/game_state.h"
#include "ra/ui/dialog.h"

namespace ra {

// Scratch space for packing and unpacking the MapPack INI block.
static char map_pack_buffer[32000];

/*
**	These layer control elements are used to group the displayable objects
**	so that proper overlap can be obtained.
*/
base::EnumArray<LayerType, LayerClass> DisplayClass::Layer;

/*
** Fading tables
*/
FadingTable DisplayClass::FadingBrighten;
FadingTable DisplayClass::FadingShade;
FadingTable DisplayClass::FadingWayDark;
FadingTable DisplayClass::FadingLight;
FadingTable DisplayClass::FadingGreen;
FadingTable DisplayClass::FadingYellow;
FadingTable DisplayClass::FadingRed;
TranslucencyTable<kMagicColorCount> DisplayClass::TranslucentTable;
TranslucencyTable<1> DisplayClass::WhiteTranslucentTable;
TranslucencyTable<4> DisplayClass::MouseTranslucentTable;
std::span<const std::byte> DisplayClass::TransIconset;
TranslucencyTable<kUnitShadowColorCount> DisplayClass::UnitShadow;
TranslucencyTable<kUnitShadowColorCount> DisplayClass::UnitShadowAir;
TranslucencyTable<1> DisplayClass::SpecialGhost;

std::span<const std::byte> DisplayClass::ShadowShapes;
TranslucencyTable<kShadowColorCount> DisplayClass::ShadowTrans;

std::bitset<MAP_CELL_TOTAL> DisplayClass::CellRedraw;

/*
** The main button that intercepts user input to the map
*/
DisplayClass::TacticalClass DisplayClass::TacButton;

/***********************************************************************************************
 * DisplayClass::DisplayClass -- Default constructor for display class. *
 *                                                                                             *
 *    This constructor for the display class just initializes some of the
 *display settings.    * Most settings are initialized with the correct values
 *at the time that the Init function * is called. There are some cases where
 *default values are wise and this routine fills     * those particular ones in.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/06/1994 JLB : Created. *
 *=============================================================================================*/
DisplayClass::DisplayClass() {
  ShadowShapes = {};
  TransIconset = {};

  Update_View_Dimensions(0, 8, 320 / game::kCellWidthPixels,
                         200 / game::kCellHeightPixels, false);
}

/***********************************************************************************************
 * DisplayClass::One_Time -- Performs any special one time initializations. *
 *                                                                                             *
 *    This routine is called from the game initialization process. It is to
 *perform any one    * time initializations necessary for the map display
 *system. It allocates the staging      * buffer needed for the radar map. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine must be called ONCE and only once. *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. * 05/31/1994 JLB : Handles layer system
 *now.                                                * 06/02/1994 JLB : Takes
 *care of misc display tables and data allocation.                   *
 *=============================================================================================*/
void DisplayClass::One_Time() {
  MapClass::One_Time();

  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    Layer.at(layer).One_Time();
  }

  /*
  **	Load the generic transparent icon set.
  */
  TransIconset = MixArchive::RetrieveData("TRANS.ICN");

  ShadowShapes = MixArchive::RetrieveData("SHADOW.SHP");

  Set_View_Dimensions(0, 16);
}

/***********************************************************************************************
 * DisplayClass::Init_Clear -- clears the display to a known state *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/17/1995 BRR : Created. *
 *=============================================================================================*/
void DisplayClass::Init_Clear() {
  MapClass::Init_Clear();

  /*
  ** Clear any object being placed
  */
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;
  PendingHouse = HOUSE_NONE;
  CursorSize = {};
  IsTargettingMode = SPC_NONE;
  IsRepairMode = false;
  IsRubberBand = false;
  IsTentative = false;
  IsSellMode = false;

  /*
  ** Empty all the display's layers
  */
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    Layer.at(layer).Init();
  }
}

/***********************************************************************************************
 * DisplayClass::Init_IO -- clears & re-builds the map's button list *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/17/1995 BRR : Created. *
 *=============================================================================================*/
void DisplayClass::Init_IO() {
  MapClass::Init_IO();
  /*
  ** Re-attach our buttons to the main map button list, only in non-edit mode.
  */
  if (!TheDebugState().map_editor_active()) {
    Add_A_Button(TacButton);
  }
}

// The current theater's art, registered and cached by Init_Theater().
static MixArchive* theater_data = nullptr;

/***********************************************************************************************
 * DisplayClass::Init_Theater -- Performs theater-specific initialization
 *(mixfiles, etc)      *
 *                                                                                             *
 * INPUT: * theater         new theater *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/17/1995 BRR : Created. * 05/07/1996 JLB : Added translucent
 *tables.                                                *
 *=============================================================================================*/
void DisplayClass::Init_Theater(TheaterType theater) {
  char fullname[16];
  static constexpr auto MouseCols = std::to_array<TranslucentColor>(
      {{game::kBlack, game::kBlack, 110},
       {game::kWhite, game::kWhite, 110},
       {game::kLightGrey, game::kLightGrey, 110},
       {DKGREY, DKGREY, 110}});
  static constexpr auto MagicCols = std::to_array<TranslucentColor>(
      {{engine::gfx::PaletteIndex{32}, engine::gfx::PaletteIndex{32}, 110},
       {engine::gfx::PaletteIndex{33}, engine::gfx::PaletteIndex{33}, 110},
       {engine::gfx::PaletteIndex{34}, engine::gfx::PaletteIndex{34}, 110},
       {engine::gfx::PaletteIndex{35}, engine::gfx::PaletteIndex{35}, 110},
       {engine::gfx::PaletteIndex{36}, engine::gfx::PaletteIndex{36}, 110},
       {engine::gfx::PaletteIndex{37}, engine::gfx::PaletteIndex{37}, 110},
       {engine::gfx::PaletteIndex{38}, engine::gfx::PaletteIndex{38}, 110},
       {engine::gfx::PaletteIndex{39}, engine::gfx::PaletteIndex{39}, 110},
       {game::kBlack, game::kBlack, 200},
       {game::kWhite, game::kBlack, 40},
       {game::kLightGrey, game::kBlack, 80},
       {DKGREY, game::kBlack, 140},
       {game::kLightGreen, game::kBlack, 130}});
  static constexpr auto WhiteCols = std::to_array<TranslucentColor>(
      {{engine::gfx::PaletteIndex{1}, game::kWhite, 80}});
  static constexpr auto ShadowCols = std::to_array<TranslucentColor>(
      {{engine::gfx::PaletteIndex{16}, game::kBlack, 130},
       {game::kWhite, game::kBlack, 170},
       {LTGRAY, game::kBlack, 250},
       {DKGRAY, game::kBlack, 250}});
  static constexpr auto UShadowCols =
      std::to_array<TranslucentColor>({{game::kLightGreen, game::kBlack, 130}});
  // Only the source color matters; the fade is the gold remap table.
  static constexpr auto UShadowColsAir =
      std::to_array<TranslucentColor>({{game::kLightGreen, game::kWhite, 0}});
  static constexpr auto UShadowColsSnow =
      std::to_array<TranslucentColor>({{game::kLightGreen, game::kBlack, 75}});

  /*
  **	Invoke parent's init routine.
  */
  MapClass::Init_Theater(theater);

  /*
  ** Save the new theater value
  */
  TheScenario().Theater = theater;

  /*
  ** Unload old mixfiles, and cache the new ones
  */
  absl::SNPrintF(fullname, sizeof(fullname), "%s.MIX",
                 Theaters.at(theater).Root);

  if (TheScenario().Theater != TheWorld().last_theater()) {
    if (theater_data != nullptr) {
      // The registry owns the archive. Unregistering destroys the archive that
      // holds the name, so the copy is needed.
      // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
      const std::string old_name = theater_data->Filename();
      MixArchive::Unregister(old_name);
    }

    theater_data = MixArchive::Register(fullname, &TheAssets().mix_key());

    if (!theater_data->Cache()) {
      // A theater the scenario names but whose art cannot be read leaves the
      // map with nothing to draw. Damaged data files are a user setup
      // problem, not a bug.
      LOG(QFATAL) << "Cannot read " << fullname
                  << ": the game data files are missing or damaged.";
    }
  }

  /*
  **	Load the custom palette associated with this theater.
  **	The fading palettes will have to be generated as well.
  */
  absl::SNPrintF(fullname, sizeof(fullname), "%s.PAL",
                 Theaters.at(theater).Root);
  const auto palette_data = MixArchive::RetrieveData(fullname);
  CHECK_GE(std::ssize(palette_data), engine::gfx::Palette::kFileSize)
      << fullname;
  ThePalettes().game_palette() = engine::gfx::Palette::FromVga(
      base::UnsignedBytes(palette_data)
          .first<engine::gfx::Palette::kFileSize>());

  ThePalettes().original_palette() = ThePalettes().game_palette();

  const PaletteClass& palette = ThePalettes().game_palette();
  FadingGreen = BuildFadingTable(palette, game::kGreen, 110);
  FadingYellow = BuildFadingTable(palette, game::kYellow, 140);
  FadingRed = BuildFadingTable(palette, game::kRed, 140);
  MouseTranslucentTable = BuildTranslucencyTable(palette, MouseCols);
  TranslucentTable = BuildTranslucencyTable(palette, MagicCols);
  WhiteTranslucentTable = BuildTranslucencyTable(palette, WhiteCols);
  ShadowTrans = BuildTranslucencyTable(palette, ShadowCols);

  UnitShadowAir =
      BuildTranslucencyTableWith(UShadowColsAir, [](const TranslucentColor&) {
        return std::to_array(
            ThePalettes().color_remaps().at(PCOLOR_GOLD).RemapTable);
      });
  UnitShadow = BuildShadowTranslucencyTable(
      palette, theater == THEATER_SNOW ? UShadowColsSnow : UShadowCols);

  FadingShade = BuildShadowFadingTable(palette, game::kBlack,
                                       theater == THEATER_SNOW ? 75 : 130);
  FadingLight = BuildShadowFadingTable(palette, game::kWhite, 85);

  // The aircraft shadow draws every color through one shadow fade.
  SpecialGhost = {};
  std::ranges::copy(BuildShadowFadingTable(palette, game::kBlack, 100),
                    std::span(SpecialGhost).subspan(256).begin());

  FadingBrighten = BuildClosestColorFadingTable(palette, game::kWhite, 25);
  FadingWayDark = BuildClosestColorFadingTable(palette, DKGRAY, 192);

  /*
  **	Adjust the palette according to the visual control option settings.
  */
  TheOptions().Fixup_Palette();
}

/***********************************************************************************************
 * DisplayClass::Text_Overlap_List -- Creates cell overlap list for specified
 *text string.     *
 *                                                                                             *
 *    This routine is used to create an overlap list that specifies all the
 *cells that are     * covered by the specified text string. This overlap list
 *is used to handle map  refresh   * logic. *
 *                                                                                             *
 * INPUT:   text  -- Pointer to the text that would appear on the map and must
 *have an         * overlap list generated. *
 *                                                                                             *
 *          x,y   -- The coordinates that the text would appear (upper left
 *corner).           *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to an overlap list that covers all cells
 *"under" the text   * if were displayed at the coordinates specified. The list
 *is actually a series of   * offsets from the display's upper left corner cell
 *number.                          *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/06/1994 JLB : Created. * 12/07/1994 JLB : Sidebar fixup. *
 *   08/13/1995 JLB : Optimized for variable sized help text. *
 *=============================================================================================*/
std::span<const int16_t> DisplayClass::Text_Overlap_List(const char* text,
                                                         int x, int y) const {
  static int16_t _list[60];
  int count = std::ssize(_list);

  if (text != nullptr) {
    std::span<int16_t> ptr(_list);
    // The text is help text, which HelpClass prints in the map font with
    // TPF_NOSHADOW.
    int len =
        StringPixelWidth(TextFontStyle(FontType::kMap, TPF_NOSHADOW), text) +
        game::kCellWidthPixels;
    const int right = TacPixelX + Lepton_To_Pixel(TacLeptonWidth);

    /*
    **	If the help text would spill into the sidebar, then flag this fact, but
    **	shorten the apparent length so that the icon list calculation will
    **	function correctly.
    */
    if (x + len >= TacPixelX + Lepton_To_Pixel(TacLeptonWidth)) {
      len = right - x;
      base::ConsumeFront(ptr) = kRefreshSidebar;
      count--;
    }

    /*
    **	Build the list of overlap cell offset values according to the text
    **	coordinate and the length.
    */
    if (x <= right) {
      Cell ul = Click_Cell_Calc(x, y - 1);
      const Cell lr = Click_Cell_Calc(
          x + len - 1,
          std::clamp(y + 24, TacPixelY,
                     TacPixelY + Lepton_To_Pixel(TacLeptonHeight) - 1));

      if (ul == Cell::None()) {
        ul = Click_Cell_Calc(x, y);
      }

      if (ul != Cell::None() && lr != Cell::None()) {
        for (int yy = ul.y(); yy <= lr.y(); yy++) {
          for (int xx = ul.x(); xx <= lr.x(); xx++) {
            base::ConsumeFront(ptr) =
                static_cast<int16_t>(Cell::FromXY(xx, yy).index() -
                                     Cell::Containing(TacticalCoord).index());
            count--;
            if (count < 2) {
              break;
            }
          }
          if (count < 2) {
            break;
          }
        }
      }
    }

    ptr.front() = kRefreshEol;
  }
  return _list;
}

/***********************************************************************************************
 * DisplayClass::Set_View_Dimensions -- Sets the tactical display screen
 *coordinates.          *
 *                                                                                             *
 *    Use this routine to set the tactical map screen coordinates and
 *dimensions. This routine * is typically used when the screen size or position
 *changes as a result of the sidebar    * changing position or appearance. *
 *                                                                                             *
 * INPUT:   x,y   -- The X and Y pixel position on the screen for the tactical
 *map upper left  * corner. *
 *                                                                                             *
 *          width -- The width of the tactical display (in icons). If this
 *parameter is        * omitted, then the width will be as wide as the screen
 *will allow.         *
 *                                                                                             *
 *          height-- The height of the tactical display (in icons). If this
 *parameter is       * omitted, then the width will be as wide as the screen
 *will allow.         *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/06/1994 JLB : Created. * 06/27/1995 JLB : Adjusts tactical map
 *position if necessary.                              *
 *=============================================================================================*/
void DisplayClass::Set_View_Dimensions(int x, int y, int width, int height) {
  Update_View_Dimensions(x, y, width, height, true);
}

void DisplayClass::Update_View_Dimensions(int x, int y, int width, int height,
                                          bool reposition) {
  if (width == -1) {
    TacLeptonWidth = Pixel_To_Lepton(TheScreen().visible_view().width() - x);
  } else {
    // The view size is kept as a 16-bit lepton word, wrapping as it always
    // has; Pixel_To_Lepton() above wraps the same way.
    TacLeptonWidth = static_cast<uint16_t>(width * game::kCellWidthLeptons);
  }

  if (height == -1) {
    height =
        (TheScreen().visible_view().height() - y) / game::kCellHeightPixels;
  }
  TacLeptonHeight = static_cast<uint16_t>(height * game::kCellHeightLeptons);

  /*
  **	Adjust the tactical cell if it is now in an invalid position
  **	because of the changed dimensions.
  */
  const base::Point confined =
      base::ConfineRect(base::Rect{TacticalCoord.x(), TacticalCoord.y(),
                                   TacLeptonWidth, TacLeptonHeight},
                        base::Rect{MapCellX * game::kCellWidthLeptons,
                                   MapCellY * game::kCellHeightLeptons,
                                   MapCellWidth * game::kCellWidthLeptons,
                                   MapCellHeight * game::kCellHeightLeptons})
          .position;

  if (reposition) {
    Set_Tactical_Position(Coordinate::FromLeptons(confined.x(), confined.y()));
  }

  TacPixelX = x;
  TacPixelY = y;
  base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowX) = x;
  base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowY) = y;
  base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowWidth) =
      Lepton_To_Pixel(TacLeptonWidth);
  base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowHeight) =
      Lepton_To_Pixel(TacLeptonHeight);
  IsDisplayToRedraw = true;
  Flag_To_Redraw(false);

  TacButton.set_x(TacPixelX);
  TacButton.set_y(TacPixelY);
  TacButton.set_width(Lepton_To_Pixel(TacLeptonWidth));
  TacButton.set_height(Lepton_To_Pixel(TacLeptonHeight));
}

/***********************************************************************************************
 * DisplayClass::Set_Cursor_Shape -- Changes the shape of the terrain square
 *cursor.           *
 *                                                                                             *
 *    This routine is used to set up the terrain cursor according to the size of
 *the object    * that is to be placed down. The terrain cursor looks like an
 *arbitrary collection of      * hatched square overlays. Typical use is when
 *placing buildings.                          *
 *                                                                                             *
 * INPUT:   list  -- A pointer to the list that contains offsets to the cells
 *that are to      * be marked. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1994 JLB : Created. * 06/26/1995 JLB : Puts placement cursor
 *into static buffer.                                *
 *=============================================================================================*/
void DisplayClass::Set_Cursor_Shape(std::span<const int16_t> list) {
  if (!CursorSize.empty()) {
    Cursor_Mark(ZoneCell.Offset(ZoneOffset), false);
  }
  ZoneOffset = 0;

  if (!list.empty()) {
    int w = 0;
    int h = 0;
    static int16_t _list[50];

    for (int i = 0; !i || base::At(list, base::ToSize(i - 1)) != kRefreshEol;
         i++) {
      base::At(_list, i) = base::At(list, base::ToSize(i));
    }

    CursorSize = _list;
    Get_Occupy_Dimensions(w, h, CursorSize);
    ZoneOffset = static_cast<int16_t>(-((h / 2 * MAP_CELL_W) + (w / 2)));
    Cursor_Mark(ZoneCell.Offset(ZoneOffset), true);
  } else {
    CursorSize = {};
  }
}

/***********************************************************************************************
 * DisplayClass::Passes_Proximity_Check -- Determines if building placement is
 *near friendly sq*
 *                                                                                             *
 *    This routine is used by the building placement cursor logic to determine
 *whether the     * at the current cursor position if the building would be
 *adjacent to another friendly     * building. In cases where this is not true,
 *then the building cannot be placed at all.    * This determination is returned
 *by the function.                                          *
 *                                                                                             *
 * INPUT:   object   -- The building object that the current placement system is
 *examining.    *
 *                                                                                             *
 *          house    -- The house to base the proximity check upon. Typically
 *this is the      * player's house, but in multiplay, the computer needs to
 *check for      * proximity as well. *
 *                                                                                             *
 *          list     -- Pointer to the building's offset list. *
 *                                                                                             *
 *          trycell  -- The cell to base the offset list on. *
 *                                                                                             *
 * OUTPUT:  bool; Can the pending building object be placed at the present
 *cursor location     * checking only for proximity to friendly buildings?  If
 *this isn't for a      * building type object, then this routine always returns
 *true.                 *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/06/1994 JLB : Created. * 06/07/1994 JLB : Handles concrete
 *check.                                                  * 10/11/1994 BWG :
 *Added IsProximate check for ore refineries                               *
 *=============================================================================================*/
bool DisplayClass::Passes_Proximity_Check(const ObjectTypeClass* object,
                                          HousesType house,
                                          std::span<const int16_t> list,
                                          Cell trycell) const {
  int retval = -1;
  bool noradar = false;

  /*
  ** In editor mode, the proximity check always passes.
  */
  if (TheDebugState().map_editor_active()) {
    return true;
  }

  if (list.empty() || trycell.index() == 0) {
    return true;
  }

  if (object == nullptr || object->What_Am_I() != RTTI_BUILDINGTYPE) {
    return true;
  }

  const auto* building = dynamic_cast<const BuildingTypeClass*>(object);

  /*
  **	Scan through all cells that the building foundation would cover. If any
  *adjacent *	cells to these are of friendly persuasion, then consider the
  *proximity check to *	have been a success.
  */
  std::span<const int16_t> ptr = list;
  //	ptr = CursorSize;
  Cell cell = trycell;
  //	Cell cell = ZoneCell;
  if (building->Adjacent == 1) {
    while (ptr.front() != kRefreshEol && retval == -1) {
      cell = trycell.Offset(base::ConsumeFront(ptr));
      //			cell = ZoneCell + ZoneOffset +
      //base::ConsumeFront(ptr);

      if (!In_Radar(cell)) {
        retval = 0;
        noradar = true;
        break;
      }

      for (const game::Facing facing :
           magic_enum::enum_values<game::Facing>()) {
        const Cell newcell = AdjacentCell(cell, facing);

        if (!In_Radar(newcell)) {
          continue;
        }

        TechnoClass* base = (*this).at(newcell).Cell_Techno();
        /*
        **	The special cell ownership flag allows building adjacent
        **	to friendly walls and bibs even though there is no official
        **	building located there.
        */
        // BG: Modified so only walls can be placed next to walls - buildings
        // can't. JLB: Except for bibs, in which case buildings can be placed
        // next to these.
        if ((building->IsWall ||
             ((*this).at(newcell).Smudge != SMUDGE_NONE &&
              SmudgeTypeClass::As_Reference((*this).at(newcell).Smudge)
                  .IsBib)) &&
            ((*this).at(newcell).Owner == house)) {
          retval = 1;
          break;
        }

        // we've found a building...
        if (base != nullptr && base->What_Am_I() == RTTI_BUILDING &&
            base->House->Class->House == house &&
            dynamic_cast<BuildingClass*>(base)->Class->IsBase) {
          retval = 1;
          break;
        }

        /* BG: modifications to allow buildings one cell away from other
         *buildings.
         ** This is done by scanning each cell that fails the check (hence
         *getting
         ** to this point) and looking at the n/s/e/w adjacent cells to see if
         *they
         ** have buildings in them.  If they do, and they match us, then
         * succeed.
         */
        if (retval != -1) {
          break;
        }

        for (const game::Facing newface :
             magic_enum::enum_values<game::Facing>()) {
          const Cell newercell = AdjacentCell(newcell, newface);

          if ((building->IsWall ||
               ((*this).at(newercell).Smudge != SMUDGE_NONE &&
                SmudgeTypeClass::As_Reference((*this).at(newercell).Smudge)
                    .IsBib)) &&
              ((*this).at(newercell).Owner == house)) {
            retval = 1;
            break;
          }

          const TechnoClass* newbase = (*this).at(newercell).Cell_Techno();

          // we've found a building...
          if (newbase != nullptr && newbase->What_Am_I() == RTTI_BUILDING &&
              newbase->House->Class->House == house &&
              dynamic_cast<const BuildingClass*>(newbase)->Class->IsBase) {
            retval = 1;
            break;
          }
        }
        if (retval != -1) {
          break;
        }
      }
    }
  }

  if (retval == -1) {
    retval = 0;
  }

  /*
  **	If this object has special dispensation to be placed further than one
  *cell from *	other regular buildings, then check for this case now. Only
  *bother to check if *	it hasn't already been given permission to be placed
  *down.
  */
  // For land mines, let's make it check proximity within 10 squares
  if ((!retval && !noradar && object->What_Am_I() == RTTI_BUILDINGTYPE) &&
      (building->Adjacent > 1)) {
    for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
      const BuildingClass* obj = TheObjectHeaps().building().ActiveAt(index);
      if (obj != nullptr && !obj->IsInLimbo &&
          obj->House->Class->House == house && obj->Class->IsBase) {
        int centdist = Distance(obj->Center_Coord(), cell.Center());
        centdist /= game::kCellWidthLeptons;
        centdist -= (obj->Class->Width() + obj->Class->Height()) / 2;
        if (centdist <= building->Adjacent) {
          retval = 1;
          break;
        }
      }
    }
  }

  return static_cast<bool>(retval);
}

/***********************************************************************************************
 * DisplayClass::Set_Cursor_Pos -- Controls the display and animation of the tac
 *cursor.       *
 *                                                                                             *
 *    This routine controls the location, display, and animation of the *
 *    tactical map cursor. *
 *                                                                                             *
 * INPUT:   pos   -- Position to move the cursor do. If -1 is passed then * the
 *cursor will just be hidden. If the position                           * passed
 *is the same as the last position passed in,                        * then
 *animation could occur (based on timers).                             *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/22/1991 JLB : Created. * 06/02/1994 JLB : Converted to member
 *function.                                            * 06/08/1994 JLB : If
 *position is -1, then follow mouse.                                    *
 *   02/28/1995 JLB : Forces placement cursor to fit on map. *
 *=============================================================================================*/
Cell DisplayClass::Set_Cursor_Pos(Cell pos) {
  /*
  **	Follow the mouse position if no cell number is provided.
  */
  if (pos == Cell::None()) {
    pos = Click_Cell_Calc(game::TheMouse().x(), game::TheMouse().y());
  }

  // Last position of the cursor (for jump-back reasons), the one value every
  // path returns, so it is never copied (-Wnrvo). ZoneCell keeps it until the
  // cursor moves below.
  const Cell prevpos = ZoneCell;
  if (CursorSize.empty()) {
    ZoneCell = pos;
    return prevpos;
  }

  /*
  **	Adjusts the position so that the placement cursor is never part way off
  *the *	tactical map.
  */
  int w = 0;
  int h = 0;
  Get_Occupy_Dimensions(w, h, CursorSize);

  int x = pos.Offset(ZoneOffset).x();
  int y = pos.Offset(ZoneOffset).y();

  x = std::max<int>(x, TacticalCoord.cell_x());
  y = std::max<int>(y, TacticalCoord.cell_y());
  if (x + w >= TacticalCoord.cell_x() + Lepton_To_Cell(TacLeptonWidth)) {
    x = TacticalCoord.cell_x() + Lepton_To_Cell(TacLeptonWidth) - w;
  }
  if (y + h >= TacticalCoord.cell_y() + Lepton_To_Cell(TacLeptonHeight)) {
    y = TacticalCoord.cell_y() + Lepton_To_Cell(TacLeptonHeight) - h;
  }
  pos = Cell::FromXY(x, y).Offset(-ZoneOffset);

  /*
  ** This checks to see if NO animation or drawing is to occur and, if so,
  **	exits.
  */
  if (pos == ZoneCell) {
    return prevpos;
  }

  /*
  **	If the cursor is visible, then handle the graphic update.
  **	Otherwise, just update the global position of the cursor.
  */
  if (!CursorSize.empty()) {
    /*
    ** Erase the old cursor (if it exists) AND the cursor is moving.
    */
    if (pos != ZoneCell && ZoneCell != Cell::None()) {
      Cursor_Mark(ZoneCell.Offset(ZoneOffset), false);
    }

    /*
    ** Render the cursor (could just be animation).
    */
    if (pos != Cell::None()) {
      Cursor_Mark(pos.Offset(ZoneOffset), true);
    }
  }
  ZoneCell = pos;
  ProximityCheck = Passes_Proximity_Check(
      PendingObject, PendingHouse, CursorSize, ZoneCell.Offset(ZoneOffset));

  return prevpos;
}

/***********************************************************************************************
 * DisplayClass::Get_Occupy_Dimensions -- computes width & height of the given
 *occupy list     *
 *                                                                                             *
 * INPUT: * w      ptr to fill in with height * h      ptr to fill in with width
 **
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/31/1995 BRR : Created. *
 *=============================================================================================*/
void DisplayClass::Get_Occupy_Dimensions(int& w, int& h,
                                         std::span<const int16_t> list) {
  int min_x = MAP_CELL_W;
  int max_x = -MAP_CELL_W;
  int min_y = MAP_CELL_H;
  int max_y = -MAP_CELL_H;

  w = 0;
  h = 0;

  if (list.empty()) {
    /*
    ** Loop through all cell offsets, accumulating max & min x- & y-coords
    */
    while (list.front() != kRefreshEol) {
      /*
      ** Compute x & y coords of the current cell offset.  We can't use
      ** Cell::x() & Cell::y(), because they use masks to compute the values,
      ** and if the offset is negative we'll get a bogus coordinate!
      */
      const int x = list.front() % MAP_CELL_W;
      const int y = list.front() / MAP_CELL_H;

      max_x = std::max(max_x, x);
      min_x = std::min(min_x, x);
      max_y = std::max(max_y, y);
      min_y = std::min(min_y, y);

      list = list.subspan(1);
    }

    w = std::max(1, max_x - min_x + 1);
    h = std::min(1, max_y - min_y + 1);
  }
}

/***********************************************************************************************
 * DisplayClass::Cursor_Mark -- Set or resets the cursor display flag bits. *
 *                                                                                             *
 *    This routine will clear or set the cursor display bits on the map. * If
 *the bit is set, then the cursor will be rendered on that map * icon. *
 *                                                                                             *
 * INPUT:   pos   -- Position of the upper left corner of the cursor. *
 *                                                                                             *
 *          on    -- Should the bit be turned on? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Be sure that every call to set the bits is matched by a *
 *             corresponding call to clear the bits. *
 *                                                                                             *
 * HISTORY: * 09/04/1991 JLB : Created. * 06/02/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void DisplayClass::Cursor_Mark(Cell pos, bool on) {
  CellClass* cellptr = nullptr;

  if (pos == Cell::None()) {
    return;
  }

  /*
  **	For every cell in the CursorSize list, invoke its Redraw_Objects and
  **	toggle its IsCursorHere flag
  */
  std::span<const int16_t> ptr = CursorSize;
  while (ptr.front() != kRefreshEol) {
    const Cell cell = pos.Offset(base::ConsumeFront(ptr));
    if (In_Radar(cell)) {
      cellptr = &(*this).at(cell);
      cellptr->Redraw_Objects();
      cellptr->IsCursorHere = on;
    }
  }

  /*
  **	For every cell in the PendingObjectPtr's Overlap_List, invoke its
  **	Redraw_Objects routine.
  */
  if (PendingObjectPtr) {
    ptr = PendingObjectPtr->Overlap_List();
    while (ptr.front() != kRefreshEol) {
      const Cell cell = pos.Offset(base::ConsumeFront(ptr));
      if (In_Radar(cell)) {
        cellptr = &(*this).at(cell);
        cellptr->Redraw_Objects();
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::AI -- Handles the maintenance tasks for the map display. *
 *                                                                                             *
 *    This routine is called once per game display frame (15 times per second).
 *It handles     * the mouse shape tracking and map scrolling as necessary. *
 *                                                                                             *
 * INPUT:   input -- The next key just fetched from the input queue. *
 *                                                                                             *
 *          x,y   -- Mouse coordinates. *
 *                                                                                             *
 * OUTPUT:  Modifies the input code if necessary. When the input code is
 *consumed, it gets     * set to 0. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/01/1994 JLB : Created. * 06/02/1994 JLB : Filters mouse click
 *input.                                               * 06/07/1994 JLB : Fixed
 *so template click will behave right.                               *
 *   10/14/1994 JLB : Changing cursor shape over target. * 12/31/1994 JLB :
 *Takes mouse coordinates as parameters.                                   *
 *   06/27/1995 JLB : Breaks out of rubber band mode if mouse leaves map. *
 *=============================================================================================*/
void DisplayClass::AI(engine::window::InputEvent& event, int x, int y) {
  if (IsRubberBand &&
      (game::TheMouse().x() < TacPixelX || game::TheMouse().y() < TacPixelY ||
       game::TheMouse().x() >= TacPixelX + Lepton_To_Pixel(TacLeptonWidth) ||
       game::TheMouse().y() >= TacPixelY + Lepton_To_Pixel(TacLeptonHeight))) {
    Mouse_Left_Release(Cell::None(), game::TheMouse().x(), game::TheMouse().y(),
                       nullptr, ACTION_NONE);
  }

  MapClass::AI(event, x, y);
}

/***********************************************************************************************
 * DisplayClass::Submit -- Adds a game object to the map rendering system. *
 *                                                                                             *
 *    This routine is used to add an arbitrary (but tangible) game object to the
 *map. It will  * be rendered (made visible) once it is submitted to this
 *function. This function builds   * the list of game objects that get rendered
 *each frame as necessary. It is possible to    * submit the game object to
 *different rendering layers. All objects in a layer get drawn   * at the same
 *time. Using this layer method it becomes possible to have objects "below"    *
 *    other objects. *
 *                                                                                             *
 * INPUT:   object   -- Pointer to the object to add. *
 *                                                                                             *
 *          layer    -- The layer to add the object to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. * 05/31/1994 JLB : Improved layer
 *system.                                                   * 05/31/1994 JLB :
 *Sorts object position if this is for the ground layer.                   *
 *=============================================================================================*/
void DisplayClass::Submit(ObjectClass* object, LayerType layer) {
  if (object) {
    Layer.at(layer).Submit(object, layer == LAYER_GROUND);
  }
}

/***********************************************************************************************
 * DisplayClass::Remove -- Removes a game object from the rendering system. *
 *                                                                                             *
 *    Every object that is to disappear from the map must be removed from the
 *rendering        * system. *
 *                                                                                             *
 * INPUT:   object   -- The object to remove. *
 *                                                                                             *
 *          layer    -- The layer to remove it from. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. * 05/31/1994 JLB : Improved layer
 *system.                                                   *
 *=============================================================================================*/
void DisplayClass::Remove(ObjectClass* object, LayerType layer) {
  DCHECK(object != nullptr);
  DCHECK(object->IsActive);

  if (object) {
    Layer.at(layer).Delete(object);
  }
}

/***********************************************************************************************
 * DisplayClass::Click_Cell_Calc -- Determines cell from screen X & Y. *
 *                                                                                             *
 *    This routine is used to determine the cell that is located at the * screen
 *pixel coordinates given. Typical use is when the player * clicks with the
 *mouse on the tactical map.                                               *
 *                                                                                             *
 * INPUT:   x,y   -- Screen pixel coordinates. *
 *                                                                                             *
 * OUTPUT:  Returns with cell that is under the coordinates specified. * If the
 *coordinate specified is outside of the tactical                             *
 *          map, then -1 is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/27/1994 JLB : Created. *
 *=============================================================================================*/
Cell DisplayClass::Click_Cell_Calc(int x, int y) const {
  x -= TacPixelX;
  x = Pixel_To_Lepton(x);
  y -= TacPixelY;
  y = Pixel_To_Lepton(y);

  if (std::cmp_less(x, TacLeptonWidth) && std::cmp_less(y, TacLeptonHeight)) {
    const Coordinate tcoord = Coordinate::FromLeptons(
        Pixel_To_Lepton(Lepton_To_Pixel(TacticalCoord.x())),
        Pixel_To_Lepton(Lepton_To_Pixel(TacticalCoord.y())));
    return Cell::Containing(tcoord + Coordinate::FromLeptons(x, y));
  }
  return Cell::None();
}

/***********************************************************************************************
 * DisplayClass::Scroll_Map -- Scroll the tactical map in desired direction. *
 *                                                                                             *
 *    This routine is used to scroll the tactical map view in the desired *
 *    direction. It can also be used to determine if scrolling would be * legal
 *without actually performing any scrolling action. *
 *                                                                                             *
 * INPUT:   facing   -- The direction to scroll the tactical map. *
 *                                                                                             *
 *          distance -- The distance in leptons to scroll the map. *
 *                                                                                             *
 *          really   -- Should the map actually be scrolled?  If false, * then
 *only the legality of a scroll is checked.                         *
 *                                                                                             *
 * OUTPUT:  bool; Would scrolling in the desired direction be possible? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. * 05/20/1994 JLB : Converted to member
 *function.                                            * 08/09/1995 JLB : Added
 *distance parameter.                                                *
 *   08/10/1995 JLB : Any direction scrolling. *
 *=============================================================================================*/
bool DisplayClass::Scroll_Map(game::Direction facing, int& distance,
                              bool really) {
  /*
  **	If the distance is invalid then no further checking is required. Bail
  **	with a no-can-do flag.
  */
  if (distance == 0) {
    return false;
  }
  const game::Facing crude = game::NearestFacing(facing);

  if (TacticalCoord.x() == Cell_To_Lepton(MapCellX) &&
      crude != game::Facing::kWest) {
    if (crude == game::Facing::kSouthWest) {
      facing = game::Direction::kSouth;
    }
    if (crude == game::Facing::kNorthWest) {
      facing = game::Direction::kNorth;
    }
  }
  if (TacticalCoord.y() == Cell_To_Lepton(MapCellY) &&
      crude != game::Facing::kNorth) {
    if (crude == game::Facing::kNorthWest) {
      facing = game::Direction::kWest;
    }
    if (crude == game::Facing::kNorthEast) {
      facing = game::Direction::kEast;
    }
  }
  if (TacticalCoord.x() + TacLeptonWidth ==
          Cell_To_Lepton(MapCellX + MapCellWidth) &&
      crude != game::Facing::kEast) {
    if (crude == game::Facing::kNorthEast) {
      facing = game::Direction::kNorth;
    }
    if (crude == game::Facing::kSouthEast) {
      facing = game::Direction::kSouth;
    }
  }
  if (TacticalCoord.y() + TacLeptonHeight ==
          Cell_To_Lepton(MapCellY + MapCellHeight) &&
      crude != game::Facing::kSouth) {
    if (crude == game::Facing::kSouthEast) {
      facing = game::Direction::kEast;
    }
    if (crude == game::Facing::kSouthWest) {
      facing = game::Direction::kWest;
    }
  }

  /*
  **	Determine the position that it wants to scroll to.
  */
  // Moved as plain ints rather than with Coord_Move(): a scroll past the top
  // or left of the world stays negative here, where the 16-bit lepton
  // component of a Coordinate would wrap around.
  const base::Point offset =
      base::MoveOffset(static_cast<uint8_t>(facing.value()), distance);

  /*
  **	Clip the new position to the edges of the game world.
  */
  const base::ConfinedRect confined = base::ConfineRect(
      base::Rect{TacticalCoord.x() + offset.x(), TacticalCoord.y() + offset.y(),
                 TacLeptonWidth, TacLeptonHeight},
      base::Rect{Cell_To_Lepton(MapCellX), Cell_To_Lepton(MapCellY),
                 Cell_To_Lepton(MapCellWidth), Cell_To_Lepton(MapCellHeight)});
  const bool shifted = confined.bounded;
  const Coordinate coord =
      Coordinate::FromLeptons(confined.position.x(), confined.position.y());

  /*
  **	If the desired scroll was bound by the edge of the map, then adjust the
  *distance to more accurately *	reflect the actual distance moved.
  */
  if (shifted) {
    distance = Distance(TacticalCoord, coord);
  }

  /*
  **	If the new coordinate is the same as the old, then no scrolling would
  *occur.
  */
  if (!distance || coord == TacticalCoord) {
    return false;
  }

  /*
  **	Since the new coordinate is different than the old one, possibly adjust
  *the real *	tactical map accordingly.
  */
  if (really) {
    Set_Tactical_Position(coord);
    IsDisplayToRedraw = true;
    Flag_To_Redraw(false);

    /*
    **	Scrolled map REQUIRES all top layer units to be redrawn.
    */
    for (int index = 0; index < Layer.at(LAYER_TOP).Count(); index++) {
      Layer.at(LAYER_TOP).at(index)->Mark(MARK_CHANGE);
    }
    for (int index = 0; index < Layer.at(LAYER_AIR).Count(); index++) {
      Layer.at(LAYER_AIR).at(index)->Mark(MARK_CHANGE);
    }
  }
  return true;
}

/***********************************************************************************************
 * DisplayClass::Refresh_Cells -- Redraws all cells in list. *
 *                                                                                             *
 *    This routine is used to flag all cells in the specified list for *
 *    redrawing. *
 *                                                                                             *
 * INPUT:   cell  -- The origin cell that the list is offset from. *
 *                                                                                             *
 *          list  -- Pointer to a list of offsets from the origin cell. * Each
 *cell so specified is flagged for redraw.                             *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine is rather slow (by definition). *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. * 08/01/1994 JLB : Simplified. *
 *=============================================================================================*/
void DisplayClass::Refresh_Cells(Cell cell, std::span<const int16_t> list) {
  int16_t tlist[36];

  if (list.front() == kRefreshSidebar) {
    list = list.subspan(1);
  }

  List_Copy(list, std::ssize(tlist), tlist);
  std::span<const int16_t> tt = tlist;
  while (tt.front() != kRefreshEol) {
    const Cell newcell = cell.Offset(base::ConsumeFront(tt));
    if (In_Radar(newcell)) {
      (*this).at(newcell).Redraw_Objects();
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Cell_Shadow   -- Determine what shadow icon to use for the
 *cell.              *
 *                                                                                             *
 *    This routine will examine the specified cell and adjacent cells to *
 *    determine what shadow icon to use. *
 *                                                                                             *
 * INPUT:   cell     -- The cell to examine. *
 *                                                                                             *
 * OUTPUT:  Returns with the shadow icon to use. -2= all black. * -1= map cell.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/01/1994 JLB : Created. * 04/04/1994 JLB : Revamped for new
 *shadow icon method.                                     * 04/30/1994 JLB :
 *Converted to member function.                                            *
 *=============================================================================================*/
int DisplayClass::Cell_Shadow(Cell cell) const {
  static const int _shadow[256] = {
      -1, 33, 2,  2,  34, 37, 2,  2,  4,  26, 6,  6,  4,  26, 6,  6,
      35, 45, 17, 17, 38, 41, 17, 17, 4,  26, 6,  6,  4,  26, 6,  6,
      8,  21, 10, 10, 27, 31, 10, 10, 12, 23, 14, 14, 12, 23, 14, 14,
      8,  21, 10, 10, 27, 31, 10, 10, 12, 23, 14, 14, 12, 23, 14, 14,

      32, 36, 25, 25, 44, 40, 25, 25, 19, 30, 20, 20, 19, 30, 20, 20,
      39, 43, 29, 29, 42, 46, 29, 29, 19, 30, 20, 20, 19, 30, 20, 20,
      8,  21, 10, 10, 27, 31, 10, 10, 12, 23, 14, 14, 12, 23, 14, 14,
      8,  21, 10, 10, 27, 31, 10, 10, 12, 23, 14, 14, 12, 23, 14, 14,

      1,  1,  3,  3,  16, 16, 3,  3,  5,  5,  7,  7,  5,  5,  7,  7,
      24, 24, 18, 18, 28, 28, 18, 18, 5,  5,  7,  7,  5,  5,  7,  7,
      9,  9,  11, 11, 22, 22, 11, 11, 13, 13, -2, -2, 13, 13, -2, -2,
      9,  9,  11, 11, 22, 22, 11, 11, 13, 13, -2, -2, 13, 13, -2, -2,

      1,  1,  3,  3,  16, 16, 3,  3,  5,  5,  7,  7,  5,  5,  7,  7,
      24, 24, 18, 18, 28, 28, 18, 18, 5,  5,  7,  7,  5,  5,  7,  7,
      9,  9,  11, 11, 22, 22, 11, 11, 13, 13, -2, -2, 13, 13, -2, -2,
      9,  9,  11, 11, 22, 22, 11, 11, 13, 13, -2, -2, 13, 13, -2, -2};

  uint32_t index = 0;  // One bit per unmapped neighbor, 0x80 = north.
  int value = -1;

  /*
  **	Don't map cells that are at the top or bottom edge. This solves
  **	problem of accessing cells off the top or bottom of the map and into
  **	who-knows-what memory.
  */
  if (static_cast<unsigned>(cell.y() - 1) >= MAP_CELL_H - 2) {
    return -1;
  }
  // if ((unsigned)(cell.y()-1) > MAP_CELL_H-2) return(-2);

  const CellClass* cellptr = &(*this).at(cell);

  /*
  **	Presume solid black if that is what is here already.
  */
  if (!cellptr->IsVisible && !cellptr->IsMapped) {
    value = -2;
  }

  if (cellptr->IsMapped /*&& !cellptr->IsVisible*/) {
    /*
    ** Build an index into the lookup table using all 8 surrounding cells.
    ** We're mapping a revealed cell and we only care about the existence
    ** of black cells.  Bit numbering starts at the upper-right corner and
    ** goes around the cell clockwise, so 0x80 = directly north.
    */
    const auto unmapped = [this, cell](int delta) {
      const int neighbor = cell.index() + delta;
      return neighbor < 0 || neighbor >= MAP_CELL_TOTAL ||
             !base::At(cells_, neighbor).IsMapped;
    };
    if (unmapped(-MAP_CELL_W - 1)) {
      index |= 0x40;
    }
    if (unmapped(-MAP_CELL_W)) {
      index |= 0x80;
    }
    if (unmapped(-MAP_CELL_W + 1)) {
      index |= 0x01;
    }
    if (unmapped(-1)) {
      index |= 0x20;
    }
    if (unmapped(1)) {
      index |= 0x02;
    }
    if (unmapped(MAP_CELL_W - 1)) {
      index |= 0x10;
    }
    if (unmapped(MAP_CELL_W)) {
      index |= 0x08;
    }
    if (unmapped(MAP_CELL_W + 1)) {
      index |= 0x04;
    }

    value = base::At(_shadow, index);
  }
  return value;
}

/***********************************************************************************************
 * DisplayClass::Map_Cell -- Mark specified cell as having been mapped. *
 *                                                                                             *
 *    This routine maps the specified cell. The cell must not already * have
 *been mapped and the mapping player must be the human. * This routine will
 *update any adjacent cell map icon as appropriate.                      *
 *                                                                                             *
 * INPUT:   cell  -- The cell to be mapped. *
 *                                                                                             *
 *          house -- The player that is doing the mapping. *
 *                                                                                             *
 * OUTPUT:  bool; Was action taken to map this cell? *
 *                                                                                             *
 * WARNINGS:   none. *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. * 04/30/1994 JLB : Converted to member
 *function.                                            * 05/24/1994 JLB : Takes
 *pointer to HouseClass.                                             *
 *   02/20/1996 JLB : Allied units reveal the map for the player. *
 *=============================================================================================*/
bool DisplayClass::Map_Cell(Cell cell, HouseClass* house) {
  /*
  ** First check for the condition where we're spying on a house's radar
  ** facility, to see if his mapping is applicable to us.
  */
  if (house && house != ThePlayer()) {
    if (house->RadarSpied & base::Bit<uint32_t>(ThePlayer()->Class->House)) {
      house = ThePlayer();
    }
    if (TheSession().Type == GAME_NORMAL && house->Is_Ally(ThePlayer())) {
      house = ThePlayer();
    }
  }

  if (house != ThePlayer() || !In_Radar(cell)) {
    return false;
  }

  CellClass* cellptr = &(*this).at(cell);

  /*
  **	Don't bother remapping this cell if it is already mapped.
  */
  if (cellptr->IsMapped) {
    if (!cellptr->IsVisible) {
      cellptr->Redraw_Objects();
    }
    return false;
  }

  /*
  ** Mark the cell as being mapped. This must be done first because
  **	if the IsVisible flag must be set, then it might affect the
  **	adjacent cell processing.
  */
  cellptr->IsMapped = true;
  cellptr->Redraw_Objects();
  if (Cell_Shadow(cell) == -1) {
    cellptr->IsVisible = true;
  }

  /*
  **	Check out all adjacent cells to see if they need
  **	to be mapped as well. This is necessary because of the
  **	"unique" method of showing shadowed cells. Many combinations
  **	are not allowed, and to fix this, just map the cells until
  **	all is ok.
  */
  for (const game::Facing dir : magic_enum::enum_values<game::Facing>()) {
    const Cell c = AdjacentCell(cell, dir);
    CellClass* cptr = &(*this).at(c);
    cptr->Redraw_Objects();

    if (c != cell && !cptr->IsVisible) {
      const int shadow = Cell_Shadow(c);

      if (shadow == -1) {
        if (!cptr->IsMapped) {
          Map_Cell(c, house);
        } else {
          cptr->IsVisible = true;
        }
      } else {
        if (shadow != -2 && !cptr->IsMapped) {
          Map_Cell(c, house);
        }
      }
    }
  }

  TechnoClass* tech = (*this).at(cell).Cell_Techno();
  if (tech) {
    tech->Revealed(house);
  }
  return true;
}

/***********************************************************************************************
 * DisplayClass::Coord_To_Pixel -- Determines X and Y pixel coordinates. *
 *                                                                                             *
 *    This is the routine that figures out the location on the screen for * a
 *specified coordinate. It is one of the fundamental routines * necessary for
 *rendering the game objects. It performs some quick                         *
 *    tests to see if the coordinate is in a visible region and returns * this
 *check as a boolean value. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to check. *
 *                                                                                             *
 *          x,y   -- Reference to the pixel coordinates that this * coordinate
 *would be when rendered.                                        *
 *                                                                                             *
 * OUTPUT:  bool; Is this coordinate in a visible portion of the map? *
 *                                                                                             *
 * WARNINGS:   If the coordinate is not in a visible portion of the * map, then
 *this X and Y parameters are not set.                                  *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. * 12/15/1994 JLB : Converted to member
 *function.                                            * 01/07/1995 JLB : Uses
 *inline functions to extract coord components.                       *
 *   08/09/1995 JLB : Uses new coordinate system. *
 *=============================================================================================*/
#define EDGE_ZONE (game::kCellWidthLeptons * 2)
bool DisplayClass::Coord_To_Pixel(Coordinate coord, int& x, int& y) const {
  if (coord) {
    const int xtac = Pixel_To_Lepton(Lepton_To_Pixel(TacticalCoord.x()));
    int xoff = Pixel_To_Lepton(Lepton_To_Pixel(coord.x()));

    xoff = xoff + EDGE_ZONE - xtac;
    if (static_cast<unsigned>(xoff) <=
        static_cast<unsigned>(TacLeptonWidth + (EDGE_ZONE * 2))) {
      const int ytac = Pixel_To_Lepton(Lepton_To_Pixel(TacticalCoord.y()));
      int yoff = Pixel_To_Lepton(Lepton_To_Pixel(coord.y()));

      yoff = yoff + EDGE_ZONE - ytac;
      if (static_cast<unsigned>(yoff) <=
          static_cast<unsigned>(TacLeptonHeight + (EDGE_ZONE * 2))) {
        x = Lepton_To_Pixel(xoff) - (game::kCellWidthPixels * 2);
        y = Lepton_To_Pixel(yoff) - (game::kCellHeightPixels * 2);
        return true;
      }
    }
  }
  return false;
}

/***********************************************************************************************
 * DisplayClass::Push_Onto_TacMap -- Moves x & y coords to being on tactical map
 **
 *                                                                                             *
 * This routine expects a line to be drawn between SOURCE & DEST, so it pushes
 *the coords to   * be within the region bounded by TacMapX,Y - + TacMapW,H. *
 *                                                                                             *
 * INPUT:   source, dest -- References to the coordinates to check. *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:  bool; Are these coordinates in a visible portion of the map? *
 *                Returns true if the pushed source & dest are visible, but if
 *neither are     * within the map, then it returns false. *
 *                                                                                             *
 *                                                                                             *
 * HISTORY: * 03/27/1995 BWG : Created. *
 *=============================================================================================*/
bool DisplayClass::Push_Onto_TacMap(Coordinate& source,
                                    Coordinate& dest) const {
  if (!source || !dest) {
    return false;
  }

  int x1 = source.x();
  int y1 = source.y();
  int x2 = dest.x();
  int y2 = dest.y();
  const int left = TacticalCoord.x();
  const int right = TacticalCoord.x() + TacLeptonWidth;
  const int top = TacticalCoord.y();
  const int bottom = TacticalCoord.y() + TacLeptonHeight;

  if (x1 < left && x2 < left) {
    return false;
  }
  if (x1 > right && x2 > right) {
    return false;
  }
  if (y1 < top && y2 < top) {
    return false;
  }
  if (y1 > bottom && y2 > bottom) {
    return false;
  }

  x1 = std::clamp(x1, left, right);
  x2 = std::clamp(x2, left, right);
  y1 = std::clamp(y1, top, bottom);
  y2 = std::clamp(y2, top, bottom);

  source = Coordinate::FromLeptons(x1, y1);
  dest = Coordinate::FromLeptons(x2, y2);
  return true;
}

/***********************************************************************************************
 * DisplayClass::Cell_Object -- Determines what has been clicked on. *
 *                                                                                             *
 *    This routine is used to determine what the player has clicked on. * It is
 *passed the cell that the click was on and it then examines * the cell and
 *returns with a pointer to the object that is there.                         *
 *                                                                                             *
 * INPUT:   cell  -- The cell that has been clicked upon. *
 *                                                                                             *
 *          x,y   -- Optional offsets from the upper left corner of the cell to
 *be used in     * determining exactly which object in the cell is desired. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the object that is "clickable" in * the
 *specified cell. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. *
 *=============================================================================================*/
ObjectClass* DisplayClass::Cell_Object(Cell cell, int x, int y) const {
  return (*this).at(cell).Cell_Object(x, y);
}

// Nothing is kept from the previous frame: repainting the whole view costs
// about 0.3 ms, and a partial repaint can only be right if every object
// touching a repainted cell is repainted with it, in order, and nothing else
// is. See docs/TACTICAL_REDRAW_PLAN.md.
void DisplayClass::Draw_It(PixelView& view, bool forced) {
  MapClass::Draw_It(view, forced);

  if (!IsDisplayToRedraw && !forced) {
    return;
  }
  IsDisplayToRedraw = false;

  // The vortex animates without anything else changing; flagging its cells
  // asks for the next frame's repaint.
  TheWorld().chronal_vortex().Set_Redraw();

  if (DesiredTacticalCoord != TacticalCoord) {
    const bool moved = Lepton_To_Pixel(DesiredTacticalCoord.x()) !=
                           Lepton_To_Pixel(TacticalCoord.x()) ||
                       Lepton_To_Pixel(DesiredTacticalCoord.y()) !=
                           Lepton_To_Pixel(TacticalCoord.y());
    TheWorld().scenario_init()++;
    Set_Tactical_Position(DesiredTacticalCoord);
    TheWorld().scenario_init()--;
    // The map moved under the mouse: move the placement cursor to the cell
    // now under it, as moving the mouse would.
    if (moved) {
      Set_Cursor_Pos(Cell::None());
    }
  }

  CellRedraw.set();

  if (view.Lock()) {
    view.FillRect(
        base::Rect{TacPixelX, TacPixelY, Lepton_To_Pixel(TacLeptonWidth),
                   Lepton_To_Pixel(TacLeptonHeight)},
        game::kBlack);
    Redraw_Icons(view);
    TheWorld().chronal_vortex().Render(view);
    RenderObjects(view);
    Redraw_Shadow(view);

    if (IsRubberBand) {
      view.DrawRect(
          base::Rect::FromCorners(BandX + TacPixelX, BandY + TacPixelY,
                                  NewX + TacPixelX, NewY + TacPixelY),
          game::kWhite);
    }
    view.Unlock();
  }

  CellRedraw.reset();

  if constexpr (config::kScenarioEditorEnabled) {
    // The object being placed in the editor, drawn at the cell under the
    // cursor. Coord_Fixup() moves it to where that type is anchored:
    // terrain, buildings and aircraft by their upper left corner rather than
    // the cell's center.
    if (TheDebugState().map_editor_active() && PendingObjectPtr) {
      PendingObjectPtr->Coord = PendingObjectPtr->Class_Of().Coord_Fixup(
          ZoneCell.Offset(ZoneOffset).Center());
      PendingObjectPtr->Render(view, true);
    }
  }
}

void DisplayClass::SortForDrawing(const LayerType layer,
                                  std::vector<ObjectClass*>& objects) {
  if (layer == LAYER_GROUND) {
    // Sort_Y() compares y and then x, so the order is total apart from objects
    // at the very same coordinate.
    std::ranges::stable_sort(
        objects, [](const ObjectClass* left, const ObjectClass* right) {
          return *left < *right;
        });
  } else if (layer == LAYER_TOP) {
    // The layer's order is the order its members joined it in, which puts a
    // paratrooper, a top-layer object for the first third of its fall, after
    // the plane it left.
    std::ranges::stable_partition(
        objects, [](const ObjectClass* object) { return object->IsFalling; });
  }
}

// Every object is flagged before any is drawn, so an object that draws another
// can clear that one's IsToDisplay and keep it from being drawn twice: a
// building draws the unit docked with it (BuildingClass::Draw_It). Flags left
// over by objects that chose not to draw are cleared at the end, so the next
// Mark(MARK_CHANGE) is not ignored.
void DisplayClass::RenderObjects(PixelView& view) {
  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    for (int index = 0; index < Layer.at(layer).Count(); index++) {
      Layer.at(layer).at(index)->IsToDisplay = true;
    }
  }

  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    draw_list_.clear();
    for (int index = 0; index < Layer.at(layer).Count(); index++) {
      ObjectClass* object = Layer.at(layer).at(index);
      DCHECK(object->IsActive);
      draw_list_.push_back(object);
    }
    // The layer's own order is game state (logic walks it), so the draw
    // order is sorted in a copy.
    SortForDrawing(layer, draw_list_);
    for (ObjectClass* object : draw_list_) {
      object->Render(view, false);
    }
  }

  for (const LayerType layer : magic_enum::enum_values<LayerType>()) {
    for (int index = 0; index < Layer.at(layer).Count(); index++) {
      Layer.at(layer).at(index)->IsToDisplay = false;
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Redraw_Icons -- Draws all terrain icons necessary. *
 *                                                                                             *
 *    This routine will redraw all of the terrain icons that are flagged * to be
 *redrawn. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none. *
 *                                                                                             *
 * HISTORY: * 02/14/1994 JLB : Created. * 05/01/1994 JLB : Converted to member
 *function.                                            * 06/20/1994 JLB : Uses
 *cell drawing support function.                                      *
 *   12/06/1994 JLB : Scans tactical view in separate row/column loops *
 *   12/24/1994 JLB : Uses the cell bit flag array to determine what to redraw.
 **
 *=============================================================================================*/
void DisplayClass::Redraw_Icons(PixelView& view) {
  IsShadowPresent = false;
  for (int y = -TacticalCoord.sub_y(); std::cmp_less_equal(y, TacLeptonHeight);
       y += game::kCellHeightLeptons) {
    for (int x = -TacticalCoord.sub_x(); std::cmp_less_equal(x, TacLeptonWidth);
         x += game::kCellWidthLeptons) {
      Coordinate coord = TacticalCoord + Coordinate::FromLeptons(x, y);
      const Cell cell = Cell::Containing(coord);
      coord = cell.Center().Whole();

      /*
      **	Only cells flagged to be redraw are examined.
      */
      if (In_View(cell) && Is_Cell_Flagged(cell)) {
        int xpixel = 0;
        int ypixel = 0;

        if (Coord_To_Pixel(coord, xpixel, ypixel)) {
          const CellClass* cellptr = &(*this).at(coord);

          /*
          **	If there is a portion of the underlying icon that could be
          *visible, *	then draw it.  Also draw the cell if the shroud is off.
          */
          if (cellptr->IsMapped || TheDebugState().unshroud()) {
            cellptr->Draw_It(view, xpixel, ypixel);
          }

          /*
          **	If any cell is not fully mapped, then flag it so that the shadow
          *drawing *	process will occur.  Only draw the shadow if
          *TheDebugState().unshroud() is false.
          */
          if (!cellptr->IsVisible && !TheDebugState().unshroud()) {
            IsShadowPresent = true;
          }
        }
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Redraw_Shadow -- Draw the shadow overlay. *
 *                                                                                             *
 *    This routine is called after all other tactical map rendering takes place.
 *It draws      * the shadow map over the tactical map. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/01/1995 JLB : Created. * 08/06/1995 JLB : Clips the fill rect
 *if necessary.                                        *
 *=============================================================================================*/
void DisplayClass::Redraw_Shadow(PixelView& view) {
  if (IsShadowPresent) {
    for (int y = -TacticalCoord.sub_y();
         std::cmp_less_equal(y, TacLeptonHeight);
         y += game::kCellHeightLeptons) {
      for (int x = -TacticalCoord.sub_x();
           std::cmp_less_equal(x, TacLeptonWidth);
           x += game::kCellWidthLeptons) {
        Coordinate coord = TacticalCoord + Coordinate::FromLeptons(x, y);
        const Cell cell = Cell::Containing(coord);
        coord = cell.Center().Whole();

        /*
        **	Only cells flagged to be redrawn are examined.
        */
        if (In_View(cell) && Is_Cell_Flagged(cell)) {
          int xpixel = 0;
          int ypixel = 0;

          if (Coord_To_Pixel(coord, xpixel, ypixel)) {
            const CellClass* cellptr = &(*this).at(coord);
            if (cellptr->IsVisible) {
              continue;
            }
            int shadow = -2;
            if (cellptr->IsMapped) {
              shadow = Cell_Shadow(cell);
            }
            if (shadow >= 0) {
              CC_Draw_Shape(view, ShadowShapes, shadow, xpixel, ypixel,
                            WINDOW_TACTICAL, engine::gfx::ShapeFlags::kGhost,
                            {}, ShadowTrans);
            } else {
              if (shadow != -1) {
                const base::Rect visible = base::ClipRect(
                    base::Rect{xpixel, ypixel, game::kCellWidthPixels,
                               game::kCellHeightPixels},
                    Lepton_To_Pixel(TacLeptonWidth),
                    Lepton_To_Pixel(TacLeptonHeight));
                if (visible.IsValid()) {
                  view.FillRect(base::Rect{TacPixelX + visible.x(),
                                           TacPixelY + visible.y(),
                                           visible.width(), visible.height()},
                                game::kBlack);
                }
              }
            }
          }
        }
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Next_Object -- Searches for next object on display. *
 *                                                                                             *
 *    This utility routine is used to find the "next" object from the object
 *specified. This   * is typically used when <TAB> is pressed and the current
 *object shifts.                   *
 *                                                                                             *
 * INPUT:   object   -- The current object to base the "next" calculation off
 *of.              *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the next object. If there is no objects
 *available,       * then nullptr is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/20/1994 JLB : Created. *
 *=============================================================================================*/
ObjectClass* DisplayClass::Next_Object(ObjectClass* object) {
  ObjectClass* firstobj = nullptr;
  bool foundmatch = false;

  if (object == nullptr) {
    foundmatch = true;
  }
  for (int uindex = 0; uindex < Layer.at(LAYER_GROUND).Count(); uindex++) {
    ObjectClass* obj = Layer.at(LAYER_GROUND).at(uindex);

    /*
    **	Verify that the object can be selected by and is owned by the player.
    */
    if (obj != nullptr && obj->Is_Players_Army()) {
      if (firstobj == nullptr) {
        firstobj = obj;
      }
      if (foundmatch) {
        return obj;
      }
      if (object == obj) {
        foundmatch = true;
      }
    }
  }
  return firstobj;
}

/***********************************************************************************************
 * DisplayClass::Prev_Object -- Searches for the previous object on the map. *
 *                                                                                             *
 *    This routine will search for the previous object. Previous is defined as
 *the one listed  * before the specified object in the ground layer. If there is
 *no specified object, then   * the last object in the ground layer is returned.
 **
 *                                                                                             *
 * INPUT:   object   -- Pointer to the object that "previous" is to be defined
 *from.           *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the object previous to the specified one.
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/24/1995 JLB : Created. *
 *=============================================================================================*/
ObjectClass* DisplayClass::Prev_Object(ObjectClass* object) {
  ObjectClass* firstobj = nullptr;
  bool foundmatch = false;

  if (object == nullptr) {
    foundmatch = true;
  }
  for (int uindex = Layer.at(LAYER_GROUND).Count() - 1; uindex >= 0; uindex--) {
    ObjectClass* obj = Layer.at(LAYER_GROUND).at(uindex);

    /*
    **	Verify that the object can be selected by and is owned by the player.
    */
    if (obj != nullptr && obj->Is_Players_Army()) {
      if (firstobj == nullptr) {
        firstobj = obj;
      }
      if (foundmatch) {
        return obj;
      }
      if (object == obj) {
        foundmatch = true;
      }
    }
  }

  return firstobj;
}

/***********************************************************************************************
 * DisplayClass::Pixel_To_Coord -- converts screen coord to Coordinate *
 *                                                                                             *
 * INPUT: * x,y      pixel coordinates to convert *
 *                                                                                             *
 * OUTPUT: * Coordinate of pixel *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/09/1994 BR : Created. * 12/06/1994 JLB : Uses map dimension
 *variables in display class.                           * 12/10/1994 JLB : Uses
 *union to speed building coordinate value.                           *
 *=============================================================================================*/
Coordinate DisplayClass::Pixel_To_Coord(int x, int y) const {
  /*
  **	Normalize the pixel coordinates to be relative to the upper left corner
  **	of the tactical map. The coordinates are expressed in leptons.
  */
  x -= TacPixelX;
  x = Pixel_To_Lepton(x);
  y -= TacPixelY;
  y = Pixel_To_Lepton(y);

  /*
  **	If pixel coordinate is over the tactical map, then translate it into a
  *coordinate *	value. If not, then just return with nullptr.
  */
  if (std::cmp_less(x, TacLeptonWidth) && std::cmp_less(y, TacLeptonHeight)) {
    return TacticalCoord + Coordinate::FromLeptons(x, y);
  }
  return Coordinate{};
}

/***********************************************************************************************
 * DisplayClass::Calculated_Cell -- Fetch a map cell based on specified method.
 **
 *                                                                                             *
 *    Find a cell meeting the specified requirements. This function is * used
 *for scenario reinforcements. *
 *                                                                                             *
 * INPUT:   dir   -- Method of picking a map cell. *
 *                                                                                             *
 *          waypoint -- Closest waypoint to use for finding appropriate map
 *edge.              *
 *                                                                                             *
 *          cell  -- Cell to find closest edge to if waypoint not specified. *
 *                                                                                             *
 *          loco  -- The locomotion of the reinforcements that are trying to
 *enter.            *
 *                                                                                             *
 *          zonecheck   -- Is zone checking required? *
 *                                                                                             *
 *          mzone    -- The movement zone type to check against (only if zone
 *checking).       *
 *                                                                                             *
 * OUTPUT:  Returns with the calculated cell. If 0, then this indicates * that
 *no legal cell was found. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. * 04/11/1994 JLB : Revamped. *
 *   05/18/1994 JLB : Converted to member function. * 12/18/1995 JLB : Handles
 *edge preference scan.                                            * 06/24/1996
 *JLB : Removed Dune II legacy code. * 06/25/1996 JLB : Rewrote and greatly
 *simplified.                                          * 10/05/1996 JLB : Checks
 *for zone and crushable status.                                    *
 *=============================================================================================*/
Cell DisplayClass::Calculated_Cell(SourceType dir, WAYPOINT waypoint, Cell cell,
                                   SpeedType loco, bool zonecheck,
                                   MZoneType mzone) const {
  bool vert = false;
  bool horz = false;
  int x = 0;
  int y = 0;
  int zone = -1;  // Tentative zone for legality checking.

  /*
  **	Waypoint edge detection for ground based reinforcements that have a
  *waypoint origin are *	determined by finding the closest map edge to
  *the waypoint. Reinforcement location *	scanning starts from that
  *position.
  */
  Cell trycell = Cell::None();
  if (waypoint != -1) {
    trycell = base::At(TheScenario().Waypoint, waypoint);
  }
  if (trycell == Cell::None()) {
    trycell = cell;
  }

  /*
  **	If zone checking is requested, then find the correct zone to use.
  */
  if (zonecheck && trycell != Cell::None()) {
    zone = (*this).at(trycell).Zones.at(mzone);
  }

  /*
  **	If the cell or waypoint specified as been detected as legal, then set up
  *the map edge *	scanning values accordingly.
  */
  if (trycell != Cell::None()) {
    x = trycell.x() - MapCellX;
    x = std::min(x, -trycell.x() + (MapCellX + MapCellWidth));

    y = trycell.y() - MapCellY;
    y = std::min(y, -trycell.y() + (MapCellY + MapCellHeight));

    if (x < y) {
      vert = true;
      horz = false;
      if (trycell.x() - MapCellX < MapCellWidth / 2) {
        x = -1;
      } else {
        x = MapCellWidth;
      }
      y = trycell.y() - MapCellY;

    } else {
      vert = false;
      horz = true;
      if (trycell.y() - MapCellY < MapCellHeight / 2) {
        y = -1;
      } else {
        y = MapCellHeight;
      }
      x = trycell.x() - MapCellX;
    }
  }

  /*
  **	If no map edge can be inferred from the waypoint, then go with the
  **	map edge specified by the edge parameter.
  */
  if (!vert && !horz) {
    switch (dir) {
      case SourceType::SOURCE_NONE:
      case SourceType::SOURCE_AIR:
      default:
      case SOURCE_NORTH:
        horz = true;
        y = -1;
        x = RandomPick(0, MapCellWidth - 1);
        break;

      case SOURCE_SOUTH:
        horz = true;
        y = MapCellHeight;
        x = RandomPick(0, MapCellWidth - 1);
        break;

      case SOURCE_EAST:
        vert = true;
        x = MapCellWidth;
        y = RandomPick(0, MapCellHeight - 1);
        break;

      case SOURCE_WEST:
        vert = true;
        x = -1;
        y = RandomPick(0, MapCellHeight - 1);
        break;
    }
  }

  /*
  **	Scan through the vertical and horizontal edges of the map looking for
  **	a relatively clear cell for object placement. The cell scanned is
  **	from the edge position specified by the X and Y variables.
  */
  if (vert) {
    const int modifier = x > MapCellX ? -1 : 1;

    for (int index = 0; index < MapCellHeight; index++) {
      const Cell edge_cell =
          Cell::FromXY(x + MapCellX, ((y + index) % MapCellHeight) + MapCellY);

      if (Good_Reinforcement_Cell(edge_cell, edge_cell.Offset(modifier), loco,
                                  zone, mzone)) {
        return edge_cell;
      }
    }
  }

  if (horz) {
    const int modifier = y > MapCellY ? -MAP_CELL_W : MAP_CELL_W;

    for (int index = 0; index < MapCellWidth; index++) {
      const Cell edge_cell =
          Cell::FromXY(((x + index) % MapCellWidth) + MapCellX, y + MapCellY);

      if (Good_Reinforcement_Cell(edge_cell, edge_cell.Offset(modifier), loco,
                                  zone, mzone)) {
        return edge_cell;
      }
    }
  }

  // If there was no success in finding a suitable reinforcement edge cell,
  // punt: return the edge cell X and Y picked above (the scans leave them
  // alone). A prvalue, since the scans return other named cells (-Wnrvo).
  return Cell::FromXY(x + MapCellX, y + MapCellY);
}

/***********************************************************************************************
 * DisplayClass::Good_Reinforcement_Cell -- Checks cell for renforcement
 *legality.             *
 *                                                                                             *
 *    This routine will check the secified cell (given the specified conditions)
 *and determine * if that is a good cell for reinforcement purposes. It checks
 *for passability of the cell * as well as zone and whether blocking walls can
 *be crushed.                               *
 *                                                                                             *
 * INPUT:   outcell  -- The cell that is just outside the edge of the map. *
 *                                                                                             *
 *          incell   -- The cell that is just inside the edge of the map. *
 *                                                                                             *
 *          loco     -- The locomotion type of the reinforcement. *
 *                                                                                             *
 *          zone     -- The zone that the eventual movement destination lies. A
 *reinforcement  * edge must fall within the same zone. *
 *                                                                                             *
 *          mzone    -- The zone check type to check against (if zone checking
 *required)       *
 *                                                                                             *
 * OUTPUT:  bool; Is the specified cell good for reinforcement purposes? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/05/1996 JLB : Created. *
 *=============================================================================================*/
bool DisplayClass::Good_Reinforcement_Cell(Cell outcell, Cell incell,
                                           SpeedType loco, int zone,
                                           MZoneType mzone) const {
  /*
  **	If the map edge location is not clear for object placement, then this is
  *not *	a good cell for reinforcement purposes.
  */
  if (!(*this).at(outcell).Is_Clear_To_Move(loco, false, false)) {
    return false;
  }

  /*
  **	If it looks like the on-map cell cannot be driven on to, then return
  *with *	the failure code.
  */
  if (!(*this).at(incell).Is_Clear_To_Move(loco, false, false, zone, mzone)) {
    return false;
  }

  /*
  **	If the reinforcement cell is already occupied, then return a failure
  *code.
  */
  if ((*this).at(outcell).Cell_Techno() != nullptr) {
    return false;
  }
  if ((*this).at(incell).Cell_Techno() != nullptr) {
    return false;
  }

  /*
  **	All tests have passed, return with success code.
  */
  return true;
}

/***********************************************************************************************
 * DisplayClass::Select_These -- All selectable objects in region are selected.
 **
 *                                                                                             *
 *    Use this routine to simultaneously select all objects within the
 *coordinate region       * specified. This routine is used by the multi-select
 *rubber band handler.                 *
 *                                                                                             *
 * INPUT:   coord1   -- Coordinate of one corner of the selection region. *
 *                                                                                             *
 *          coord2   -- The opposite corner of the selection region. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. * 04/25/1995 JLB : Limited to
 *non-building type.                                            * 03/06/1996 JLB
 *: Allows selection of aircraft with bounding box.                          *
 *=============================================================================================*/
// Not const: changes which objects are selected.
// NOLINTNEXTLINE(readability-make-member-function-const)
void DisplayClass::Select_These(Coordinate coord1, Coordinate coord2) {
  const Coordinate tcoord =
      TacticalCoord;  // TacticalCell.Center() & 0xFF00FF00L;

  coord1 = tcoord + coord1;
  coord2 = tcoord + coord2;
  int x1 = coord1.x();
  int x2 = coord2.x();
  int y1 = coord1.y();
  int y2 = coord2.y();

  /*
  **	Ensure that coordinate number one represents the upper left corner
  **	and coordinate number two represents the lower right corner.
  */
  if (x1 > x2) {
    const int temp = x1;
    x1 = x2;
    x2 = temp;
  }
  if (y1 > y2) {
    const int temp = y1;
    y1 = y2;
    y2 = temp;
  }

  /*
  **	Sweep through all ground layer objects and select the ones within the
  **	bounding box.
  */
  Unselect_All();
  TheGameState().allow_voice() = true;
  for (int index = 0; index < Layer.at(LAYER_GROUND).Count(); index++) {
    ObjectClass* obj = Layer.at(LAYER_GROUND).at(index);
    const Coordinate ocoord = obj->Center_Coord();
    const int x = ocoord.x();
    const int y = ocoord.y();

    /*
    **	Only try to select objects that are owned by the player, are allowed to
    *be *	selected, and are within the bounding box.
    */
    const HouseClass* hptr = HouseClass::As_Pointer(obj->Owner());
    if ((hptr != nullptr && hptr->IsPlayerControl &&
         obj->Class_Of().IsSelectable && obj->What_Am_I() != RTTI_BUILDING &&
         x >= x1 && x <= x2 && y >= y1 && y <= y2) &&
        obj->Select()) {
      TheGameState().allow_voice() = false;
    }
  }

  /*
  **	Select any aircraft with the bounding box.
  */
  for (int air_index = 0; air_index < TheObjectHeaps().aircraft().Count();
       air_index++) {
    AircraftClass* aircraft = TheObjectHeaps().aircraft().ActiveAt(air_index);
    const Coordinate ocoord = aircraft->Center_Coord();
    const int x = ocoord.x();
    const int y = ocoord.y();

    /*
    **	Only try to select objects that are owned by the player, are allowed to
    *be *	selected, and are within the bounding box.
    */
    if ((aircraft->House->IsPlayerControl && aircraft->Class->IsSelectable &&
         !aircraft->IsSelected && x >= x1 && x <= x2 && y >= y1 && y <= y2) &&
        aircraft->Select()) {
      TheGameState().allow_voice() = false;
    }
  }

  TheGameState().allow_voice() = true;
}

/***********************************************************************************************
 * DisplayClass::Refresh_Band -- Causes all cells under the rubber band to be
 *redrawn.         *
 *                                                                                             *
 *    Use this routine to flag all cells that are covered in some fashion by the
 *multi-unit    * select "rubber band" to be redrawn. This is necessary whenever
 *the rubber band changes   * size or is being removed. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Refresh_Band() {
  if (IsRubberBand) {
    /*
    **	In rubber band mode, mark all cells under the "rubber band" to be
    **	redrawn.
    */
    int x1 = BandX + TacPixelX;
    int y1 = BandY + TacPixelY;
    int x2 = NewX + TacPixelX;
    int y2 = NewY + TacPixelY;

    if (x1 > x2) {
      const int temp = x1;
      x1 = x2;
      x2 = temp;
    }
    if (y1 > y2) {
      const int temp = y1;
      y1 = y2;
      y2 = temp;
    }

    Cell cell{};
    for (int y = y1; y <= y2 + game::kCellHeightPixels;
         y += game::kCellHeightPixels) {
      cell = Click_Cell_Calc(
          x1, std::clamp(y, 0, TacPixelY + Lepton_To_Pixel(TacLeptonHeight)));
      if (cell != Cell::None()) {
        (*this).at(cell).Redraw_Objects();
      }

      cell = Click_Cell_Calc(
          x2, std::clamp(y, 0, TacPixelY + Lepton_To_Pixel(TacLeptonHeight)));
      if (cell != Cell::None()) {
        (*this).at(cell).Redraw_Objects();
      }
    }

    for (int x = x1; x <= x2 + game::kCellWidthPixels;
         x += game::kCellWidthPixels) {
      cell = Click_Cell_Calc(
          std::clamp(x, 0, TacPixelX + Lepton_To_Pixel(TacLeptonWidth)), y1);
      if (cell != Cell::None()) {
        (*this).at(cell).Redraw_Objects();
      }

      cell = Click_Cell_Calc(
          std::clamp(x, 0, TacPixelX + Lepton_To_Pixel(TacLeptonWidth)), y2);
      if (cell != Cell::None()) {
        (*this).at(cell).Redraw_Objects();
      }
    }

    /*
    **	Stretching the rubber band requires all objects to be redrawn.
    */
    for (int index = 0; index < Layer.at(LAYER_TOP).Count(); index++) {
      Layer.at(LAYER_TOP).at(index)->Mark(MARK_CHANGE);
    }
    for (int index = 0; index < Layer.at(LAYER_AIR).Count(); index++) {
      Layer.at(LAYER_AIR).at(index)->Mark(MARK_CHANGE);
    }
  }
}

/***********************************************************************************************
 * DisplayClass::TacticalClass::Action -- Processes input for the tactical map.
 **
 *                                                                                             *
 *    This routine handles the input directed at the tactical map. Since input,
 *in this        * regard, includes even the presence of the mouse over the
 *tactical map, this routine      * is called nearly every game frame. It
 *handles adjusting the mouse shape as well as       * giving orders to units. *
 *                                                                                             *
 * INPUT:   flags -- The gadget event flags that triggered the call to this
 *function.          *
 *                                                                                             *
 *          key   -- A reference to the keyboard event (if any). *
 *                                                                                             *
 * OUTPUT:  bool; Should processing be aborted on any succeeding buttons in the
 *chain?         *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/17/1995 JLB : Created. *
 *=============================================================================================*/
bool DisplayClass::TacticalClass::Action(InputFlags flags,
                                         engine::window::KeyNumber& key, int x,
                                         int y) {
  ObjectClass* object = nullptr;
  ActionType action =
      ACTION_NONE;  // Action possible with currently selected object.

  const bool edge = y == 0 || x == 0 ||
                    x == TheScreen().visible_view().width() - 1 ||
                    y == TheScreen().visible_view().height() - 1;
  const Coordinate coord = TheMap().Pixel_To_Coord(x, y);
  const Cell cell = Cell::Containing(coord);
  if (coord) {
    const bool shadow =
        !TheMap().at(cell).IsMapped && !TheDebugState().unshroud();
    x -= TheMap().TacPixelX;
    y -= TheMap().TacPixelY;

    /*
    ** Cause any displayed cursor to move along with the mouse cursor.
    */
    if (cell != TheMap().ZoneCell) {
      TheMap().Set_Cursor_Pos(cell);
    }

    /*
    **	Determine the object that the mouse is currently over.
    */
    if (!shadow) {
      object = TheMap().Close_Object(coord);

      /*
      **	Special case check to ignore cloaked object if not owned by the
      *player.
      */
      if (object != nullptr && object->Is_Techno() &&
          !dynamic_cast<TechnoClass*>(object)->IsOwnedByPlayer &&
          (dynamic_cast<TechnoClass*>(object)->Cloak == game::CLOAKED ||
           dynamic_cast<TechnoClass*>(object)
               ->Techno_Type_Class()
               ->IsInvisible)) {
        object = nullptr;
      }
    }

    /*
    **	If there is a currently selected object, then the action to perform if
    **	the left mouse button were clicked must be determined.
    */
    if (std::ssize(TheWorld().current_object())) {
      if (object != nullptr) {
        action = base::At(TheWorld().current_object(), 0)->What_Action(object);
      } else {
        action = base::At(TheWorld().current_object(), 0)->What_Action(cell);
      }

    } else {
      if (object && object->Class_Of().IsSelectable) {
        action = ACTION_SELECT;
      }

      if (TheMap().IsRepairMode) {
        if (object && object->Owner() == ThePlayer()->Class->House &&
            object->Can_Repair()) {
          action = ACTION_REPAIR;
        } else {
          action = ACTION_NO_REPAIR;
        }
      }

      if (TheMap().IsSellMode) {
        if (object && object->Owner() == ThePlayer()->Class->House &&
            object->Can_Demolish()) {
          if (object->What_Am_I() == RTTI_BUILDING) {
            action = ACTION_SELL;
          } else {
            action = ACTION_SELL_UNIT;
          }
        } else {
          /*
          **	Check to see if the cursor is over an owned wall.
          */
          if (TheMap().at(cell).Overlay != OVERLAY_NONE &&
              OverlayTypeClass::As_Reference(TheMap().at(cell).Overlay)
                  .IsWall &&
              TheMap().at(cell).Owner == ThePlayer()->Class->House) {
            action = ACTION_SELL;
          } else {
            action = ACTION_NO_SELL;
          }
        }
      }

      if (TheMap().IsTargettingMode == SPC_NUCLEAR_BOMB) {
        action = ACTION_NUKE_BOMB;
      }

      if (TheMap().IsTargettingMode == SPC_PARA_BOMB) {
        action = ACTION_PARA_BOMB;
      }

      if (TheMap().IsTargettingMode == SPC_PARA_INFANTRY) {
        action = ACTION_PARA_INFANTRY;
      }

      if (TheMap().IsTargettingMode == SPC_SPY_MISSION) {
        action = ACTION_SPY_MISSION;
      }

      if (TheMap().IsTargettingMode == SPC_IRON_CURTAIN) {
        action = ACTION_IRON_CURTAIN;
      }

      if (TheMap().IsTargettingMode == SPC_CHRONOSPHERE) {
        action = ACTION_CHRONOSPHERE;
      }

      if (TheMap().IsTargettingMode == kSpcChrono2) {
        action = ACTION_CHRONO2;
        if (shadow) {
          action = ACTION_NOMOVE;
        }
        const ObjectClass* tobject = ThePlayer()->UnitToTeleport.AsObject();

        /*
        **	Determine if the object can be teleported to the destination
        *cell.
        */
        if (tobject != nullptr && tobject->Is_Techno()) {
          const auto* uobject = dynamic_cast<const TechnoClass*>(tobject);
          if (!uobject->Can_Teleport_Here(cell)) {
            //					if (((UnitClass
            //*)PlayerPtr->UnitToTeleport.AsObject())->Can_Enter_Cell(cell,
            // Facing::kNone) != MOVE_OK) {
            action = ACTION_NOMOVE;
          }

        } else {  // If the object is no longer valid, cancel targetting mode.
          action = ACTION_NOMOVE;
          TheMap().IsTargettingMode = SPC_NONE;
        }
      }

      if (TheMap().PendingObject) {
        action = ACTION_NONE;
      }
    }

    /*
    **	Move any cursor displayed.
    */
    if (cell != TheMap().ZoneCell) {
      TheMap().Set_Cursor_Pos(cell);
    }

    /*
    **	A right mouse button press cancels the current action or selection.
    */
    if (base::Any(flags & InputFlags::kRightPress)) {
      TheMap().Mouse_Right_Press();
    }

    /*
    **	Make sure that if the mouse button has been released and the map doesn't
    *know about it, *	then it must be informed. Do this by faking a mouse
    *release event.
    */
    if (base::Any(flags & InputFlags::kLeftUp) && TheMap().IsRubberBand) {
      flags |= InputFlags::kLeftRelease;
    }

    /*
    **	When the mouse buttons aren't pressed, only the mouse cursor shape is
    *processed. *	The shape changes depending on what object the mouse is
    *currently over and what *	object is currently selected.
    */
    if ((!edge) && base::Any(flags & InputFlags::kLeftUp)) {
      TheMap().Mouse_Left_Up(cell, shadow, object, action);
    }

    /*
    **	Normal actions occur when the mouse button is released. The press event
    *is *	intercepted and possible rubber-band mode is flagged.
    */
    if (base::Any(flags & InputFlags::kLeftRelease)) {
      TheMap().Mouse_Left_Release(cell, x, y, object, action);
    }

    /*
    **	When the mouse is first pressed on the map, then record the mouse
    **	position so that a proper check before going into rubber band
    **	mode can be made. Rubber band mode starts when the mouse is
    **	held down and moved a certain minimum distance.
    */
    if (!edge && base::Any(flags & InputFlags::kLeftPress)) {
      TheMap().Mouse_Left_Up(cell, shadow, object, action);
      TheMap().Mouse_Left_Press(x, y);
    }

    /*
    **	While the mouse is being held down, determine if rubber band mode should
    **	start. If rubber band mode is already active, then update the size
    **	and flag the map to redraw it.
    */
    if (base::Any(flags & InputFlags::kLeftHeld)) {
      TheMap().Mouse_Left_Held(x, y);
    }
  }

  return game::Gadget::Action(InputFlags::kNone, key, x, y);
}

/***********************************************************************************************
 * DisplayClass::Mouse_Right_Press -- Handles the right mouse button press. *
 *                                                                                             *
 *    This routine is called when the right mouse button is pressed. This action
 *is supposed   * to cancel whatever mode or process is active. If there is
 *nothing to cancel, then it     * will default to unselecting any units that
 *might be currently selected.                  *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/24/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Mouse_Right_Press() {
  if (PendingObjectPtr && PendingObjectPtr->Is_Techno()) {
    // PendingObjectPtr->Transmit_Message(RADIO_OVER_OUT);
    PendingObjectPtr = nullptr;
    PendingObject = nullptr;
    PendingHouse = HOUSE_NONE;
    Set_Cursor_Shape({});
  } else {
    if (IsRepairMode) {
      IsRepairMode = false;
    } else {
      if (IsSellMode) {
        IsSellMode = false;
      } else {
        if (IsTargettingMode != SPC_NONE) {
          IsTargettingMode = SPC_NONE;
        } else {
          Unselect_All();
        }
      }
    }
  }

  // If it breaks... call 228.
  Set_Default_Mouse(MOUSE_NORMAL, TheMap().IsSmall);
}

/***********************************************************************************************
 * DisplayClass::Mouse_Left_Up -- Handles the left mouse "cruising" over the
 *map.              *
 *                                                                                             *
 *    This routine is called continuously while the mouse is over the tactical
 *map but there   * are no mouse buttons pressed. Typically, this adjusts the
 *mouse shape and the pop-up     * help text. *
 *                                                                                             *
 * INPUT:   shadow   -- Is the mouse hovering over shadowed terrain? *
 *                                                                                             *
 *          object   -- Pointer to the object that the mouse is currently over
 *(may be nullptr).  *
 *                                                                                             *
 *          action   -- This is the action that the currently selected object
 *(if any) will    * perform if the left mouse button were clicked at this
 *location.        *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/24/1995 JLB : Created. * 07/05/1995 JLB : Removed pop up help
 *text for shadow and terrain after #3.                *
 *=============================================================================================*/
void DisplayClass::Mouse_Left_Up(Cell cell, bool shadow, ObjectClass* object,
                                 ActionType action, bool wsmall) {
  IsTentative = false;

  Target target = Target{};
  if (object != nullptr) {
    target = object->As_Target();
  } else {
    if (cell != Cell::None()) {
      target = Target::FromCell(cell);
    }
  }

  /*
  **	Don't allow selection of an object that is located in shadowed terrain.
  **	In fact, just show the normal move cursor in order to keep the shadowed
  **	terrain a mystery.
  */
  if (shadow) {
    switch (action) {
      case ACTION_NO_DEPLOY:
        Set_Default_Mouse(MOUSE_NO_DEPLOY, wsmall);
        break;

      case ACTION_NO_ENTER:
        Set_Default_Mouse(MOUSE_NO_ENTER, wsmall);
        break;

      case ACTION_NO_GREPAIR:
        Set_Default_Mouse(MOUSE_NO_GREPAIR, wsmall);
        break;

      case ACTION_DAMAGE:
      case ACTION_GREPAIR:
        Set_Default_Mouse(MOUSE_NORMAL, wsmall);
        break;

      case ACTION_GUARD_AREA:
        Set_Default_Mouse(MOUSE_AREA_GUARD, wsmall);
        break;

      case ACTION_NONE:
        Set_Default_Mouse(MOUSE_NORMAL, wsmall);
        break;

      case ACTION_NO_SELL:
      case ACTION_SELL:
      case ACTION_SELL_UNIT:
        Set_Default_Mouse(MOUSE_NO_SELL_BACK, wsmall);
        break;

      case ACTION_NO_REPAIR:
      case ACTION_REPAIR:
        Set_Default_Mouse(MOUSE_NO_REPAIR, wsmall);
        break;

      case ACTION_NUKE_BOMB:
        Set_Default_Mouse(MOUSE_NUCLEAR_BOMB, wsmall);
        break;

      case ACTION_AIR_STRIKE:
      case ACTION_PARA_BOMB:
      case ACTION_PARA_INFANTRY:
      case ACTION_SPY_MISSION:
      case ACTION_IRON_CURTAIN:
        Set_Default_Mouse(MOUSE_AIR_STRIKE, wsmall);
        break;

      case ACTION_CHRONOSPHERE:
        Set_Default_Mouse(MOUSE_CHRONO_SELECT, wsmall);
        break;

      case ACTION_CHRONO2:
        Set_Default_Mouse(MOUSE_CHRONO_DEST, wsmall);
        break;

      case ACTION_HEAL:
        Set_Default_Mouse(MOUSE_HEAL, wsmall);
        break;

      case ACTION_NOMOVE:
        if (std::ssize(TheWorld().current_object()) &&
            base::At(TheWorld().current_object(), 0)->What_Am_I() ==
                RTTI_AIRCRAFT) {
          Set_Default_Mouse(MOUSE_NO_MOVE, wsmall);
          break;
        }
        [[fallthrough]];
        // Fall into next case for non aircraft object types.

      case ActionType::ACTION_MOVE:
      case ActionType::ACTION_ENTER:
      case ActionType::ACTION_SELF:
      case ActionType::ACTION_ATTACK:
      case ActionType::ACTION_HARVEST:
      case ActionType::ACTION_SELECT:
      case ActionType::ACTION_TOGGLE_SELECT:
      case ActionType::ACTION_CAPTURE:
      case ActionType::ACTION_SABOTAGE:
      case ActionType::ACTION_PARA_SABOTEUR:
      default:
        Set_Default_Mouse(MOUSE_CAN_MOVE, wsmall);
        break;
    }
  } else {
    /*
    **	Change the mouse shape according to the default action that will occur
    **	if the mouse button were clicked at this location.
    */
    switch (action) {
      case ACTION_NO_DEPLOY:
        Set_Default_Mouse(MOUSE_NO_DEPLOY, wsmall);
        break;

      case ACTION_NO_ENTER:
        Set_Default_Mouse(MOUSE_NO_ENTER, wsmall);
        break;

      case ACTION_NO_GREPAIR:
        Set_Default_Mouse(MOUSE_NO_GREPAIR, wsmall);
        break;

      case ACTION_DAMAGE:
        Set_Default_Mouse(MOUSE_DAMAGE, wsmall);
        break;

      case ACTION_GREPAIR:
        Set_Default_Mouse(MOUSE_GREPAIR, wsmall);
        break;

      case ACTION_TOGGLE_SELECT:
      case ACTION_SELECT:
        Set_Default_Mouse(MOUSE_CAN_SELECT, wsmall);
        break;

      case ACTION_MOVE:
        Set_Default_Mouse(MOUSE_CAN_MOVE, wsmall);
        break;

      case ACTION_GUARD_AREA:
        Set_Default_Mouse(MOUSE_AREA_GUARD, wsmall);
        break;

      case ACTION_ATTACK:
        if (target && std::ssize(TheWorld().current_object()) == 1 &&
            base::At(TheWorld().current_object(), 0)->Is_Techno() &&
            dynamic_cast<TechnoClass*>(base::At(TheWorld().current_object(), 0))
                ->In_Range(target, 0)) {
          Set_Default_Mouse(MOUSE_STAY_ATTACK, wsmall);
          break;
        }
        [[fallthrough]];

      case ACTION_HARVEST:
        Set_Default_Mouse(MOUSE_CAN_ATTACK, wsmall);
        break;

      case ACTION_SABOTAGE:
        Set_Default_Mouse(MOUSE_DEMOLITIONS, wsmall);
        break;

      case ACTION_ENTER:
      case ACTION_CAPTURE:
        Set_Default_Mouse(MOUSE_ENTER, wsmall);
        break;

      case ACTION_NOMOVE:
        Set_Default_Mouse(MOUSE_NO_MOVE, wsmall);
        break;

      case ACTION_NO_SELL:
        Set_Default_Mouse(MOUSE_NO_SELL_BACK, wsmall);
        break;

      case ACTION_NO_REPAIR:
        Set_Default_Mouse(MOUSE_NO_REPAIR, wsmall);
        break;

      case ACTION_SELF:
        Set_Default_Mouse(MOUSE_DEPLOY, wsmall);
        break;

      case ACTION_REPAIR:
        Set_Default_Mouse(MOUSE_REPAIR, wsmall);
        break;

      case ACTION_SELL_UNIT:
        Set_Default_Mouse(MOUSE_SELL_UNIT, wsmall);
        break;

      case ACTION_SELL:
        Set_Default_Mouse(MOUSE_SELL_BACK, wsmall);
        break;

      case ACTION_NUKE_BOMB:
        Set_Default_Mouse(MOUSE_NUCLEAR_BOMB, wsmall);
        break;

      case ACTION_AIR_STRIKE:
      case ACTION_PARA_BOMB:
      case ACTION_PARA_INFANTRY:
      case ACTION_SPY_MISSION:
      case ACTION_IRON_CURTAIN:
        Set_Default_Mouse(MOUSE_AIR_STRIKE, wsmall);
        break;

      case ACTION_CHRONOSPHERE:
        Set_Default_Mouse(MOUSE_CHRONO_SELECT, wsmall);
        break;

      case ACTION_CHRONO2:
        Set_Default_Mouse(MOUSE_CHRONO_DEST, wsmall);
        break;

      case ACTION_HEAL:
        Set_Default_Mouse(MOUSE_HEAL, wsmall);
        break;

      case ActionType::ACTION_NONE:
      case ActionType::ACTION_PARA_SABOTEUR:
      default:
        Set_Default_Mouse(MOUSE_NORMAL, wsmall);
        break;
    }
  }

  /*
  **	Never display help text if the mouse is held over the radar map.
  */
  if (wsmall) {
    return;
  }

  /*
  **	Give a generic help message when over shadow terrain.
  */
  if (shadow) {
    //		if (Scen.Scenario < 4) {
    Help_Text(TXT_SHADOW);
    //		} else {
    //			Help_Text(TXT_NONE);
    //		}
  } else {
    /*
    **	If the mouse is held over objects on the map, then help text may
    **	pop up that tells what the object is. This call informs the help
    **	system of the text name for the object under the mouse.
    */
    if (object != nullptr) {
      engine::gfx::PaletteIndex color = game::kLightGrey;

      /*
      **	Fetch the appropriate background color for help text.
      */
      if (ThePlayer()->Is_Ally(object)) {
        color = game::kGreen;
      } else {
        if (object->Owner() == HOUSE_NONE || object->Owner() == HOUSE_NEUTRAL) {
          color = game::kLightGrey;
        } else {
          color = game::kPink;
        }
      }

      /*
      **	Fetch the name of the object. If it is an enemy object, then
      **	the exact identity is glossed over with a generic text.
      */
      int text = object->Full_Name();
      if ((object->Is_Techno() &&
           !dynamic_cast<const TechnoTypeClass&>(object->Class_Of())
                .IsNominal) &&
          (!dynamic_cast<TechnoClass*>(object)->House->Is_Ally(ThePlayer()))) {
        //				if (!PlayerPtr->Is_Ally(object)) {
        switch (object->What_Am_I()) {
          case RTTI_INFANTRY:
            text = TXT_ENEMY_SOLDIER;
            break;

          case RTTI_UNIT:
            text = TXT_ENEMY_VEHICLE;
            break;

          case RTTI_BUILDING:
            text = TXT_ENEMY_STRUCTURE;
            break;
          case RTTIType::RTTI_NONE:
          case RTTIType::RTTI_AIRCRAFT:
          case RTTIType::RTTI_AIRCRAFTTYPE:
          case RTTIType::RTTI_ANIM:
          case RTTIType::RTTI_ANIMTYPE:
          case RTTIType::RTTI_BUILDINGTYPE:
          case RTTIType::RTTI_BULLET:
          case RTTIType::RTTI_BULLETTYPE:
          case RTTIType::RTTI_CELL:
          case RTTIType::RTTI_FACTORY:
          case RTTIType::RTTI_HOUSE:
          case RTTIType::RTTI_HOUSETYPE:
          case RTTIType::RTTI_INFANTRYTYPE:
          case RTTIType::RTTI_OVERLAY:
          case RTTIType::RTTI_OVERLAYTYPE:
          case RTTIType::RTTI_SMUDGE:
          case RTTIType::RTTI_SMUDGETYPE:
          case RTTIType::RTTI_SPECIAL:
          case RTTIType::RTTI_TEAM:
          case RTTIType::RTTI_TEAMTYPE:
          case RTTIType::RTTI_TEMPLATE:
          case RTTIType::RTTI_TEMPLATETYPE:
          case RTTIType::RTTI_TERRAIN:
          case RTTIType::RTTI_TERRAINTYPE:
          case RTTIType::RTTI_TRIGGER:
          case RTTIType::RTTI_TRIGGERTYPE:
          case RTTIType::RTTI_UNITTYPE:
          case RTTIType::RTTI_VESSEL:
          case RTTIType::RTTI_VESSELTYPE:
          default:
            break;
        }
      }

      if (/*Scen.Scenario > 3 ||*/ object->What_Am_I() != RTTI_TERRAIN) {
        Help_Text(text, -1, -1, color);
      } else {
        Help_Text(TXT_NONE);
      }
    } else {
      if ((*this).at(cell).Land_Type() == LAND_TIBERIUM) {
        Help_Text(TXT_MINERALS);
      } else {
        Help_Text(TXT_NONE);
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Mouse_Left_Release -- Handles the left mouse button release. *
 *                                                                                             *
 *    This routine is called when the left mouse button is released over the
 *tactical map.     * The release event is the workhorse of the game. Most
 *actions occur at the moment of      * mouse release. *
 *                                                                                             *
 * INPUT:   cell     -- The cell that the mouse is over. *
 *                                                                                             *
 *          x,y      -- The mouse pixel coordinate. *
 *                                                                                             *
 *          object   -- Pointer to the object that the mouse is over. *
 *                                                                                             *
 *          action   -- The action that the currently selected object (if any)
 *will            * perform. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/24/1995 JLB : Created. * 03/27/1995 JLB : Handles sell and
 *repair actions.                                         *
 *=============================================================================================*/
void DisplayClass::Mouse_Left_Release(Cell cell, int x, int y,
                                      ObjectClass* object, ActionType action,
                                      bool wsmall) {
  if (PendingObjectPtr) {
    /*
    **	Try to place the pending object onto the map.
    */
    if (ProximityCheck) {
      // The sum is an int, so this has always been the (type, object, id)
      // constructor: the cell travels as the low half of Specific.ID, where
      // the PLACE handler reads Place.Cell. Kept for the event's bytes.
      TheNetwork().out_list().Add(EventClass(EventClass::PLACE,
                                             PendingObjectPtr->What_Am_I(),
                                             cell.index() + ZoneOffset));
    } else {
      Speak(VOX_DEPLOY);
    }

  } else {
    if (IsRubberBand) {
      Refresh_Band();
      Select_These(CoordFromPixels(BandX, BandY), CoordFromPixels(x, y));

      Set_Default_Mouse(MOUSE_NORMAL, wsmall);
      IsRubberBand = false;
      IsTentative = false;
      TheMap().IsDisplayToRedraw = true;
      TheMap().Flag_To_Redraw(false);

    } else {
      /*
      **	Toggle the select state of the object.
      */
      if (action == ACTION_TOGGLE_SELECT) {
        if (!object || !std::ssize(TheWorld().current_object()) ||
            base::At(TheWorld().current_object(), 0)->Owner() !=
                ThePlayer()->Class->House) {
          action = ACTION_SELECT;
        } else {
          if (object->IsSelected) {
            object->Unselect();
          } else {
            object->Select();
          }
        }
      }

      /*
      **	Selection of other object action.
      */
      if (object != nullptr &&
          (action == ACTION_SELECT ||
           (action == ACTION_NONE && object->Class_Of().IsSelectable &&
            !object->IsSelected))) {
        Unselect_All();
        object->Select();
        Set_Default_Mouse(MOUSE_NORMAL, wsmall);
      }

      /*
      **	If an action was detected as possible, then pass this action
      *event *	to all selected objects.
      */
      if (action != ACTION_NONE && action != ACTION_SELECT) {
        /*
        **	Pass the action to all the selected objects. But first,
        *redetermine *	what action that object should perform. This, seemingly
        *redundant *	process, is necessary since multiple objects could be
        *selected and each *	might perform a different action when the click
        *occurs.
        */
        TheGameState().allow_voice() = true;
        TheWorld().form_move() = false;
        TheWorld().form_speed() = SPEED_WHEEL;
        TheWorld().form_max_speed() = game::kMphLightSpeed;

        if ((action == ACTION_MOVE || action == ACTION_NOMOVE) &&
            std::ssize(TheWorld().current_object())) {
          /*
          ** Scan all units.  If any are selected that shouldn't be, or aren't
          ** selected but should be, then this is not a formation move.
          */
          int group = 254;  // init to invalid group #

          if (base::At(TheWorld().current_object(), 0)->Is_Foot()) {
            group = dynamic_cast<FootClass*>(
                        base::At(TheWorld().current_object(), 0))
                        ->Group;
          }

          /*
          **	Presume this is a formation move unless something is detected
          **	that will prevent it.
          */
          TheWorld().form_move() = true;

          /*
          **	First scan through all the selected units to make sure that they
          **	are all of the same team and have been assigned a particular
          *formation
          */
          for (int index = 0; index < std::ssize(TheWorld().current_object());
               index++) {
            const ObjectClass* tobject =
                base::At(TheWorld().current_object(), index);

            /*
            **	Only moveable (i.e., FootClass) objects can ever be in a
            *formation *	so if a selected object isn't of a FootClass
            *type then it can't be *	a formation move.
            */
            if (!tobject->Is_Foot()) {
              TheWorld().form_move() = false;
              break;
            }

            /*
            **	If the object is not part of the same team as the rest of the
            **	selected group, or it just plain has never been assigned a
            **	formation offset, then it can't be a formation move.
            */
            const auto* foot = dynamic_cast<const FootClass*>(tobject);
            if (std::cmp_not_equal(foot->Group, group) ||
                foot->XFormOffset == kNoFormationOffset) {
              TheWorld().form_move() = false;
              break;
            }

            /*
            **	Determine the formation speed on the presumption that this
            **	will turn out to be a formation move.
            */
            TheWorld().form_max_speed() = foot->Techno_Type_Class()->MaxSpeed;
            TheWorld().form_speed() = foot->Techno_Type_Class()->Speed;
          }

          /*
          **	Loop through all objects (that can theoretically be part of a
          *team) and *	if there are any that are part of the currently selected
          *team, but *	are not currently selected themselves, then this will
          *force this move *	to NOT be a formation move.
          */
          if (TheWorld().form_move()) {
            for (int index = 0; index < TheWorld().logic().Count(); index++) {
              const ObjectClass* obj = TheWorld().logic().at(index);

              /*
              **	If the object is selected, then it has already been
              *scanned *	by the previous loop.
              */
              if (obj->IsSelected) {
                continue;
              }

              /*
              **	Only consider footclass objects.
              */
              if (!obj->Is_Foot()) {
                continue;
              }

              const auto* foot = dynamic_cast<const FootClass*>(obj);

              /*
              **	Only consider objects that are owned by the player.
              */
              if (!foot->IsOwnedByPlayer) {
                continue;
              }

              /*
              **	If another member of this team has been discovered and
              **	it isn't selected, then the formation move cannot take
              **	place.
              */
              if (std::cmp_equal(foot->Group, group)) {
                TheWorld().form_move() = false;
                break;
              }
            }
          }
        }

        for (int index = 0; index < std::ssize(TheWorld().current_object());
             index++) {
          ObjectClass* tobject = base::At(TheWorld().current_object(), index);

          if (object != nullptr) {
            tobject->Active_Click_With(tobject->What_Action(object), object);
          } else {
            /*
            ** Trap for formation moves: if this unit is part of a
            ** formation (being part of a team qualifies) and they're
            ** told to move, adjust the target destination so they stay
            ** in formation when they arrive.
            */
            Cell newmove = cell;
            if (action == ACTION_MOVE && tobject->Is_Foot()) {
              auto* foot = dynamic_cast<FootClass*>(tobject);
              const bool oldisform = foot->IsFormationMove;
              foot->IsFormationMove = TheWorld().form_move();
              if (TheWorld().form_move() && foot->Group != kNoGroup) {
                newmove = foot->Adjust_Dest(cell);
              }
              foot->IsFormationMove = oldisform;
            }
            tobject->Active_Click_With(tobject->What_Action(cell), newmove);
          }
          TheGameState().allow_voice() = false;
        }
        TheGameState().allow_voice() = true;
        TheWorld().form_move() = false;

        if (object != nullptr && action == ACTION_REPAIR &&
            object->What_Am_I() == RTTI_BUILDING) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::REPAIR, Target::Of(object)));
        }
        if (action == ACTION_SELL_UNIT && object) {
          switch (object->What_Am_I()) {
            case RTTI_AIRCRAFT:
            case RTTI_UNIT:
              TheNetwork().out_list().Add(
                  EventClass(EventClass::SELL, Target::Of(object)));
              break;

            case RTTIType::RTTI_NONE:
            case RTTIType::RTTI_AIRCRAFTTYPE:
            case RTTIType::RTTI_ANIM:
            case RTTIType::RTTI_ANIMTYPE:
            case RTTIType::RTTI_BUILDING:
            case RTTIType::RTTI_BUILDINGTYPE:
            case RTTIType::RTTI_BULLET:
            case RTTIType::RTTI_BULLETTYPE:
            case RTTIType::RTTI_CELL:
            case RTTIType::RTTI_FACTORY:
            case RTTIType::RTTI_HOUSE:
            case RTTIType::RTTI_HOUSETYPE:
            case RTTIType::RTTI_INFANTRY:
            case RTTIType::RTTI_INFANTRYTYPE:
            case RTTIType::RTTI_OVERLAY:
            case RTTIType::RTTI_OVERLAYTYPE:
            case RTTIType::RTTI_SMUDGE:
            case RTTIType::RTTI_SMUDGETYPE:
            case RTTIType::RTTI_SPECIAL:
            case RTTIType::RTTI_TEAM:
            case RTTIType::RTTI_TEAMTYPE:
            case RTTIType::RTTI_TEMPLATE:
            case RTTIType::RTTI_TEMPLATETYPE:
            case RTTIType::RTTI_TERRAIN:
            case RTTIType::RTTI_TERRAINTYPE:
            case RTTIType::RTTI_TRIGGER:
            case RTTIType::RTTI_TRIGGERTYPE:
            case RTTIType::RTTI_UNITTYPE:
            case RTTIType::RTTI_VESSEL:
            case RTTIType::RTTI_VESSELTYPE:
            default:
              break;
          }
        }
        if (action == ACTION_SELL) {
          if (object) {
            TheNetwork().out_list().Add(
                EventClass(EventClass::SELL, Target::Of(object)));
          } else {
            TheNetwork().out_list().Add(EventClass(EventClass::SELLCELL, cell));
            //						OutList.Add(EventClass(EventClass::SELL,
            //ra::As_Target(cell)));
          }
        }

        // Where in the cell the player aimed, for the weapons that act on the
        // unit nearest it. A click on the radar has no such spot, so it aims
        // at the cell's center.
        const Coordinate where =
            wsmall ? Coordinate::CellCenterOffset()
                   : Pixel_To_Coord(x + TacPixelX, y + TacPixelY).Fraction();
        if (action == ACTION_NUKE_BOMB) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_NUCLEAR_BOMB), cell, where));
        }

        if (action == ACTION_PARA_BOMB) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_PARA_BOMB), cell, where));
        }
        if (action == ACTION_PARA_INFANTRY) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_PARA_INFANTRY), cell, where));
        }
        if (action == ACTION_SPY_MISSION) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_SPY_MISSION), cell, where));
        }
        if (action == ACTION_IRON_CURTAIN) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_IRON_CURTAIN), cell, where));
        }
        if (action == ACTION_CHRONOSPHERE) {
          TheNetwork().out_list().Add(
              EventClass(EventClass::SPECIAL_PLACE,
                         static_cast<int>(SPC_CHRONOSPHERE), cell, where));
        }
        if (action == ACTION_CHRONO2) {
          TheNetwork().out_list().Add(EventClass(EventClass::SPECIAL_PLACE,
                                                 static_cast<int>(kSpcChrono2),
                                                 cell, where));
        }
      }

      IsTentative = false;
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Mouse_Left_Press -- Handles the left mouse button press. *
 *                                                                                             *
 *    Handle the left mouse button press while over the tactical map. If it
 *isn't is           * repair or sell mode, then a tentative transition to
 *rubber band mode is flagged. If the  * mouse moves a sufficient distance from
 *this recorded position, then rubber band mode     * is officially started. *
 *                                                                                             *
 * INPUT:   x,y   -- The mouse coordinates at the time of the press. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/24/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Mouse_Left_Press(int x, int y) {
  if (!IsRepairMode && !IsSellMode && IsTargettingMode == SPC_NONE &&
      !PendingObject) {
    IsTentative = true;
    BandX = x;
    BandY = y;
    NewX = x;
    NewY = y;
  }
}

/***********************************************************************************************
 * DisplayClass::Mouse_Left_Held -- Handles the left button held down. *
 *                                                                                             *
 *    This routine is called continuously while the left mouse button is held
 *down over        * the tactical map. This handles the rubber band mode
 *detection and dragging.              *
 *                                                                                             *
 * INPUT:   x,y   -- The mouse coordinate. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/24/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Mouse_Left_Held(int x, int y) {
  if (IsRubberBand) {
    if (x != NewX || y != NewY) {
      x = std::clamp(x, 0, Lepton_To_Pixel(TacLeptonWidth) - 1);
      y = std::clamp(y, 0, Lepton_To_Pixel(TacLeptonHeight) - 1);
      Refresh_Band();
      NewX = x;
      NewY = y;
      IsDisplayToRedraw = true;
      Flag_To_Redraw(false);
    }
  } else {
    /*
    **	If the mouse is still held down while a tentative extended select is
    *possible, then *	check to see if the mouse has moved a sufficient
    *distance in order to activate *	extended select mode.
    */
    /*
    **	The mouse must have moved a minimum distance before rubber band
    *mode can be *	initiated.
    */
    if (IsTentative && (std::abs(x - BandX) > 4 || std::abs(y - BandY) > 4)) {
      IsRubberBand = true;
      x = std::clamp(x, 0, Lepton_To_Pixel(TacLeptonWidth) - 1);
      y = std::clamp(y, 0, Lepton_To_Pixel(TacLeptonHeight) - 1);
      NewX = x;
      NewY = y;
      IsDisplayToRedraw = true;
      Flag_To_Redraw(false);

      /*
      **	Stretching the rubber band requires all objects to be redrawn.
      */
      for (int index = 0; index < Layer.at(LAYER_TOP).Count(); index++) {
        Layer.at(LAYER_TOP).at(index)->Mark(MARK_CHANGE);
      }
      for (int index = 0; index < Layer.at(LAYER_AIR).Count(); index++) {
        Layer.at(LAYER_AIR).at(index)->Mark(MARK_CHANGE);
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Set_Tactical_Position -- Sets the tactical view position. *
 *                                                                                             *
 *    This routine is used to set the tactical view position. The requested
 *position is        * clipped to the map dimensions as necessary. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate desired for the upper left corner. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/13/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Set_Tactical_Position(Coordinate coord) {
  /*
  **	Bound the desired location to fit the legal map edges.
  */
  const base::Point confined =
      base::ConfineRect(
          base::Rect{coord.x(), coord.y(), TacLeptonWidth, TacLeptonHeight},
          base::Rect{Cell_To_Lepton(MapCellX), Cell_To_Lepton(MapCellY),
                     Cell_To_Lepton(MapCellWidth),
                     Cell_To_Lepton(MapCellHeight)})
          .position;
  coord = Coordinate::FromLeptons(confined.x(), confined.y());

  if (TheWorld().scenario_init()) {
    TacticalCoord = coord;
  }
  DesiredTacticalCoord = coord;

  IsDisplayToRedraw = true;
  Flag_To_Redraw(false);
}

/***********************************************************************************************
 * DisplayClass::Compute_Start_Pos -- Computes player's start pos from unit
 *coords.            *
 *                                                                                             *
 * Use this function in multiplayer games, to compute the scenario starting
 *Tactical Pos.      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/28/1995 JLB : Commented. * 06/26/1995 JLB : Fixed building
 *loop.                                                     * 10/20/1996 JLB :
 *Doesn't wrap.                                                            *
 *=============================================================================================*/
void DisplayClass::Compute_Start_Pos() {
  /*
  **	Find the summation cell-x & cell-y for all the player's units, infantry,
  **	and buildings.  Buildings are weighted so that they count 16 times more
  **	than units or infantry.
  */
  int32_t x = 0;
  int32_t y = 0;
  int32_t num = 0;
  for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
    const InfantryClass* infp = TheObjectHeaps().infantry().ActiveAt(i);
    if (!infp->IsInLimbo && infp->IsOwnedByPlayer) {
      x += infp->Coord.cell_x();
      y += infp->Coord.cell_y();
      num++;
    }
  }

  for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
    const UnitClass* unitp = TheObjectHeaps().unit().ActiveAt(i);
    if (!unitp->IsInLimbo && unitp->IsOwnedByPlayer) {
      x += unitp->Coord.cell_x();
      y += unitp->Coord.cell_y();
      num++;
    }
  }

  for (int i = 0; i < TheObjectHeaps().building().Count(); i++) {
    const BuildingClass* bldgp = TheObjectHeaps().building().ActiveAt(i);
    if (!bldgp->IsInLimbo && bldgp->IsOwnedByPlayer) {
      x += bldgp->Coord.cell_x() * 16;
      y += bldgp->Coord.cell_y() * 16;
      num += 16;
    }
  }

  for (int i = 0; i < TheObjectHeaps().vessel().Count(); i++) {
    const VesselClass* bldgp = TheObjectHeaps().vessel().ActiveAt(i);
    if (!bldgp->IsInLimbo && bldgp->IsOwnedByPlayer) {
      x += bldgp->Coord.cell_x();
      y += bldgp->Coord.cell_y();
      num++;
    }
  }

  /*
  **	Divide each coord by 'num' to compute the average value
  */
  if (num > 0) {
    x /= num;
  } else {
    x = 0;
  }

  if (num > 0) {
    y /= num;
  } else {
    y = 0;
  }

  /*
  **	Tactical position is based on the cell of the upper left corner. Make
  *adjustments *	and bound the calculated location to the map dimensions.
  */
  //	x -= 5 * 2;
  //	y -= 4 * 2;
  x = std::clamp<int32_t>(x, MapCellX + 10, MapCellX + MapCellWidth - 10);
  y = std::clamp<int32_t>(y, MapCellY + 8, MapCellY + MapCellHeight - 8);

  base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
      base::At(TheScenario().Views, 0) = base::At(TheScenario().Views, 1) =
          base::At(TheScenario().Views, 2) = base::At(TheScenario().Views, 3) =
              Cell::FromXY(static_cast<int>(x), static_cast<int>(y));

  TheMap().Set_Tactical_Position(base::At(TheScenario().Views, 0)
                                     .Offset(-(MAP_CELL_W * 8) - 10)
                                     .Center()
                                     .Whole());
  //	Set_Tactical_Position(Cell::FromXY(x, y).Center());
}

/***********************************************************************************************
 * DisplayClass::Sell_Mode_Control -- Controls the sell mode. *
 *                                                                                             *
 *    This routine will control the sell mode for the player. *
 *                                                                                             *
 * INPUT:   control  -- The mode to set the sell state to. * 0  = Turn sell mode
 *off.                                               * 1  = Turn sell mode on. *
 *                      -1 = Toggle sell mode. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Sell_Mode_Control(int control) {
  bool mode = IsSellMode;
  switch (control) {
    case 0:
      mode = false;
      break;

    case -1:
      mode = !static_cast<bool>(IsSellMode);
      break;

    case 1:
      mode = true;
      break;
    default:
      break;
  }

  if (mode != IsSellMode && !PendingObject) {
    IsRepairMode = false;
    if (mode && ThePlayer()->BScan) {
      IsSellMode = true;
      Unselect_All();
    } else {
      IsSellMode = false;
      Revert_Mouse_Shape();
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Repair_Mode_Control -- Controls the repair mode. *
 *                                                                                             *
 *    This routine is used to control the repair mode for the player. *
 *                                                                                             *
 * INPUT:   control  -- The mode to set the repair to. * 0 = Turn repair off. *
 *                      1 = Turn repair on. * -1= Toggle repair state. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Repair_Mode_Control(int control) {
  bool mode = IsRepairMode;
  switch (control) {
    case 0:
      mode = false;
      break;

    case -1:
      mode = !static_cast<bool>(IsRepairMode);
      break;

    case 1:
      mode = true;
      break;
    default:
      break;
  }

  if (mode != IsRepairMode && !PendingObject) {
    IsSellMode = false;
    if (mode && ThePlayer()->BScan) {
      IsRepairMode = true;
      Unselect_All();
    } else {
      IsRepairMode = false;
      Revert_Mouse_Shape();
    }
  }
}

/***********************************************************************************************
 * DisplayClass::In_View -- Determines if cell is visible on screen. *
 *                                                                                             *
 *    Use this routine to determine if the specified cell is visible on * the
 *display. This is a useful fact, since many display operations * can be skipped
 *if the cell is not visible.                                               *
 *                                                                                             *
 * INPUT:   cell  -- The cell number to check. *
 *                                                                                             *
 * OUTPUT:  bool; Is this cell visible on the display? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/30/1994 JLB : Created. * 04/30/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
bool DisplayClass::In_View(Cell cell) const {
  if (cell.index() < 0 || cell.index() >= MAP_CELL_TOTAL) {
    return false;
  }

  const Coordinate coord = cell.Center().Whole();
  const Coordinate tcoord = TacticalCoord.Whole();

  if (static_cast<unsigned>(coord.x() - tcoord.x()) >
      static_cast<unsigned>(TacLeptonWidth + game::kCellWidthLeptons - 1)) {
    return false;
  }
  if (static_cast<unsigned>(coord.y() - tcoord.y()) >
      static_cast<unsigned>(TacLeptonHeight + game::kCellHeightLeptons - 1)) {
    return false;
  }
  return true;
}

/***********************************************************************************************
 * DisplayClass::Closest_Free_Spot -- Finds the closest cell sub spot that is
 *free.            *
 *                                                                                             *
 *    Use this routine to find the sub cell spot closest to the coordinate
 *specified that is   * free from occupation. Typical use of this is for
 *infantry destination calculation.       *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to use as the starting point when finding
 *the closest      * free spot. *
 *                                                                                             *
 *          any   -- Ignore occupation and just return the closest sub cell
 *spot?              *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate of the closest free (possibly) sub cell
 *location.      *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/22/1995 JLB : Created. *
 *=============================================================================================*/
Coordinate DisplayClass::Closest_Free_Spot(Coordinate coord, bool any) {
  if (coord.packed() & kHighCoordMask) {
    return Coordinate::CellCenterOffset();
  }
  return TheMap().at(coord).Closest_Free_Spot(coord, any);
}

/***********************************************************************************************
 * DisplayClass::Is_Spot_Free -- Determines if cell sub spot is free of
 *occupation.            *
 *                                                                                             *
 *    Use this routine to determine if the coordinate (rounded to the nearest
 *sub cell         * position) is free for placement. Typical use of this would
 *be for infantry placement.    *
 *                                                                                             *
 * INPUT:   coord -- The coordinate to examine for "freeness". The coordinate is
 *rounded to    * the nearest free sub cell spot. *
 *                                                                                             *
 * OUTPUT:  Is the sub spot indicated by the coordinate free from previous
 *occupation?         *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/22/1995 JLB : Created. *
 *=============================================================================================*/
bool DisplayClass::Is_Spot_Free(Coordinate coord) {
  if (coord.packed() & kHighCoordMask) {
    return true;
  }
  return TheMap().at(coord).Is_Spot_Free(CellClass::Spot_Index(coord));
}

/***********************************************************************************************
 * DisplayClass::Center_Map -- Centers the map about the currently selected
 *objects            *
 *                                                                                             *
 *    This routine will average the position of all the selected objects and
 *then center       * the map about those objects. *
 *                                                                                             *
 * INPUT:   center   -- The is an optional center about override coordinate. If
 *specified,     * then the map will be centered about that coordinate.
 *Otherwise it      * will center about the average location of all selected
 *objects.        *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The map position changes by this routine. *
 *                                                                                             *
 * HISTORY: * 08/22/1995 JLB : Created. * 09/16/1996 JLB : Takes coordinate to
 *center about (as override).                          *
 *=============================================================================================*/
void DisplayClass::Center_Map(Coordinate center) {
  int x = 0;
  //	unsigned x = 0;
  int y = 0;
  //	unsigned y = 0;
  bool centerit = false;

  if (std::ssize(TheWorld().current_object())) {
    for (int index = 0; index < std::ssize(TheWorld().current_object());
         index++) {
      const Coordinate coord =
          base::At(TheWorld().current_object(), index)->Center_Coord();

      x += coord.x();
      y += coord.y();
    }

    x = static_cast<int>(x / std::ssize(TheWorld().current_object()));
    y = static_cast<int>(y / std::ssize(TheWorld().current_object()));
    centerit = true;
  }

  if (center) {
    x = center.x();
    y = center.y();
    centerit = true;
  }

  if (centerit) {
    x = x - (TacLeptonWidth / 2);
    x = std::max<int>(x, Cell_To_Lepton(MapCellX));

    y = y - (TacLeptonHeight / 2);
    y = std::max<int>(y, Cell_To_Lepton(MapCellY));

    Set_Tactical_Position(Coordinate::FromLeptons(x, y));
  }
}

/***********************************************************************************************
 * DisplayClass::Encroach_Shadow -- Causes the shadow to creep back by one cell.
 **
 *                                                                                             *
 *    This routine will cause the shadow to creep back by one cell. Multiple
 *calls to this     * routine will result in the shadow becoming more and more
 *invasive until only the sight   * range of player controlled units will keep
 *the shadow pushed back.                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/16/1995 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Encroach_Shadow() {
  for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
    const Cell cell = Cell::FromIndex(cell_index);
    if (!In_Radar(cell)) {
      continue;
    }

    CellClass* cellptr = &(*this).at(cell);
    if (cellptr->IsVisible || !cellptr->IsMapped) {
      continue;
    }

    cellptr->IsToShroud = true;
  }

  /*
  **	Mark all shadow edge cells to be fully shrouded. All adjacent mapped
  **	cell should become partially shrouded.
  */
  for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
    const Cell cell = Cell::FromIndex(cell_index);
    if (!In_Radar(cell)) {
      continue;
    }

    if ((*this).at(cell).IsToShroud) {
      (*this).at(cell).IsToShroud = false;
      Shroud_Cell(cell);
    }
  }

  All_To_Look();

  Flag_To_Redraw(true);
}

/***********************************************************************************************
 * DisplayClass::Shroud_Cell -- Returns the specified cell into the shrouded
 *condition.        *
 *                                                                                             *
 *    This routine is called to add the shroud back to the cell specified.
 *Typical of this     * would be when the shroud is to regenerate. *
 *                                                                                             *
 * INPUT:   cell  -- The cell that the shroud is to be regenerated upon. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Adjacent cells might be affected by this routine. The affect is
 *determined      * according to the legality of the partial shadow artwork. In
 *the illegal cases   * the adjacent cell might become shrouded as well. *
 *                                                                                             *
 * HISTORY: * 10/17/1995 JLB : Created. * 06/17/1996 JLB : Modified to handle
 *the new shadow pieces.                                *
 *=============================================================================================*/
void DisplayClass::Shroud_Cell(Cell cell /*KO, bool shadeit*/) {
  if (ThePlayer()->IsGPSActive &&
      ((*this).at(cell).Jammed &
       base::Bit<uint16_t>(ThePlayer()->Class->House))) {
    return;
  }

  if (!In_Radar(cell)) {
    return;
  }

  CellClass* cellptr = &(*this).at(cell);
  if (cellptr->IsMapped) {
    cellptr->IsMapped = false;
    cellptr->IsVisible = false;
    cellptr->Redraw_Objects();

    /*
    **	Check adjacent cells. There might be some weird combination of
    **	shrouded cells such that more cells must be shrouded in order for
    **	this to work.
    */
    for (const game::Facing dir : magic_enum::enum_values<game::Facing>()) {
      const Cell c = AdjacentCell(cell, dir);
      CellClass* cptr = &(*this).at(c);

      /*
      **	If this adjacent cell must be completely shrouded as a result
      **	of the map change, yet it isn't already shrouded, then
      *recursively *	shroud that cell.
      */
      if (c != cell) {
        cptr->IsVisible = false;
      }

      /*
      **	Always redraw the cell because, more than likely, the shroud
      **	edge will change shape because of the map change.
      */
      cptr->Redraw_Objects();
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Read_INI -- Reads map control data from INI file. *
 *                                                                                             *
 *    This routine is used to read the map control data from the INI * file. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the loaded INI file data. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The TriggerClass INI data must have been read before calling this
 *function.     *
 *                                                                                             *
 * HISTORY: * 05/27/1994 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Read_INI(engine::file::IniFile& ini) {
  /*
  **	Read the map dimensions.
  */
  const char* const name = "Map";
  const int x = ini.GetInt(name, "X", 1);
  const int y = ini.GetInt(name, "Y", 1);
  const int w = ini.GetInt(name, "Width", MAP_CELL_W - 2);
  const int h = ini.GetInt(name, "Height", MAP_CELL_H - 2);

  Set_Map_Dimensions(x, y, w, h);

  /*
  **	The theater is determined at this point. There is specific data that
  **	is custom to this data. Load the custom data (as it related to terrain)
  **	at this point.
  */
  TheScenario().Theater =
      GetTheaterType(ini, name, "Theater", THEATER_TEMPERATE);

  /*
  **	Now that the theater is known, init the entire map hierarchy
  */
  Init(TheScenario().Theater);

  /*
  **	Special initializations occur when the theater is known.
  */
  TerrainTypeClass::Init(TheScenario().Theater);
  TemplateTypeClass::Init(TheScenario().Theater);
  OverlayTypeClass::Init(TheScenario().Theater);
  UnitTypeClass::Init(TheScenario().Theater);
  InfantryTypeClass::Init(TheScenario().Theater);
  BuildingTypeClass::Init(TheScenario().Theater);
  BulletTypeClass::Init(TheScenario().Theater);
  AnimTypeClass::Init(TheScenario().Theater);
  AircraftTypeClass::Init(TheScenario().Theater);
  VesselTypeClass::Init(TheScenario().Theater);
  SmudgeTypeClass::Init(TheScenario().Theater);

  /*
  **	Read the Waypoint entries.
  */
  for (int i = 0; i < ScenarioClass::kWaypointCount; i++) {
    char buf[20];
    absl::SNPrintF(buf, sizeof(buf), "%d", i);
    base::At(TheScenario().Waypoint, i) =
        Cell::FromIndex(ini.GetInt("Waypoints", buf, -1));

    if (base::At(TheScenario().Waypoint, i) != Cell::None()) {
      (*this).at(base::At(TheScenario().Waypoint, i)).IsWaypoint = true;
    }
  }

  /*
  **	Set the starting position (do this after Init(), which clears the cells'
  **	IsWaypoint flags).
  */
  if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) ==
      Cell::None()) {
    base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
        Cell::FromXY(MapCellX + 10, MapCellY + 8);
  }

  base::At(TheScenario().Views, 0) = base::At(TheScenario().Views, 1) =
      base::At(TheScenario().Views, 2) = base::At(TheScenario().Views, 3) =
          base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint);
  Set_Tactical_Position(
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
          .Offset(-(MAP_CELL_W * 8) - 10)
          .Center());

  /*
  **	Loop through all CellTrigger entries.
  */
  const int trigger_count = ini.EntryCount("CellTriggers");
  for (int index = 0; index < trigger_count; index++) {
    /*
    **	Get a cell trigger and cell assignment.
    */
    const std::string_view cellentry = ini.EntryName("CellTriggers", index);
    TriggerType* tp = GetTriggerType(ini, "CellTriggers", cellentry);
    const Cell cell =
        Cell::FromIndex(base::ParseIntegerOr<int16_t>(cellentry, 0));

    if (tp != nullptr && (*this).at(cell).Trigger == nullptr) {
      TriggerClass* tt = Find_Or_Make(tp);
      if (tt) {
        tt->AttachCount++;
        tt->Cell = cell;
        (*this).at(cell).Trigger = tt;
      }
    }
  }

  /*
  **	Read the map template data.
  */
  static const char* const MAPPACK = "MapPack";
  const std::vector<std::byte> map_pack = ini.GetBase64Block(MAPPACK);
  SpanSource bstraw(map_pack);
  TheMap().Read_Binary(bstraw);

  TheWorld().last_theater() = TheScenario().Theater;
}

/***********************************************************************************************
 * DisplayClass::Write_INI -- Write the map data to the INI file specified. *
 *                                                                                             *
 *    This routine will output all the data of this map to the INI database
 *specified.         *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI handler to store the map data to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Any existing map data in the INI database will be replaced by
 *this function.    *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Write_INI(engine::file::IniFile& ini) {
  char entry[20];

  /*
  **	Save the map parameters.
  */
  static const char* const NAME = "Map";
  ini.Clear(NAME);
  PutTheaterType(ini, NAME, "Theater", TheScenario().Theater);
  ini.PutInt(NAME, "X", MapCellX);
  ini.PutInt(NAME, "Y", MapCellY);
  ini.PutInt(NAME, "Width", MapCellWidth);
  ini.PutInt(NAME, "Height", MapCellHeight);

  /*
  **	Save the Waypoint entries.
  */
  static const char* const WAYNAME = "Waypoints";
  ini.Clear(WAYNAME);
  for (int i = 0; i < ScenarioClass::kWaypointCount; i++) {
    if (base::At(TheScenario().Waypoint, i) != Cell::None()) {
      absl::SNPrintF(entry, sizeof(entry), "%d", i);
      ini.PutInt(WAYNAME, entry, base::At(TheScenario().Waypoint, i).index());
    }
  }

  /*
  **	Save the cell's triggers.
  */
  static const char* const CELLTRIG = "CellTriggers";
  ini.Clear(CELLTRIG);
  for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
    const Cell cell = Cell::FromIndex(cell_index);
    if ((*this).at(cell).Trigger != nullptr) {
      const TriggerClass* tp = (*this).at(cell).Trigger;
      if (tp != nullptr) {
        /*
        **	Generate entry name.
        */
        absl::SNPrintF(entry, sizeof(entry), "%d", cell.index());

        /*
        **	Save entry.
        */
        PutTriggerType(ini, CELLTRIG, entry, tp->Class);
      }
    }
  }

  /*
  **	Write the map template data out to the ini file.
  */
  static const char* const MAPPACK = "MapPack";
  SpanSink bpipe(std::as_writable_bytes(std::span(map_pack_buffer)));
  TheMap().Write_Binary(bpipe);
  const auto len = static_cast<int>(bpipe.bytes_written());
  ini.Clear(MAPPACK);
  if (len) {
    ini.PutBase64Block(
        MAPPACK, base::ObjectBytes(map_pack_buffer).first(base::ToSize(len)));
  }
}

/***********************************************************************************************
 * DisplayClass::All_To_Look -- Direct all objects to look around for the
 *player.              *
 *                                                                                             *
 *    This routine will scan through all objects and tell them to look if they
 *are supposed    * to be able to reveal the map for the player. This routine
 *may be necessary in cases      * of gap generator reshroud logic. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/23/1996 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::All_To_Look(bool units_only) {
  for (int index = 0; index < Layer.at(LAYER_GROUND).Count(); index++) {
    ObjectClass* object = Layer.at(LAYER_GROUND).at(index);
    if (object != nullptr && object->Is_Techno()) {
      auto* tech = dynamic_cast<TechnoClass*>(object);

      if (tech->What_Am_I() == RTTI_BUILDING && units_only) {
        continue;
      }

      if (tech->House->IsPlayerControl) {
        if (tech->IsDiscoveredByPlayer) {
          object->Look();
        }
      } else {
        if (tech->What_Am_I() == RTTI_BUILDING && TheRules().IsAllyReveal &&
            tech->House->Is_Ally(ThePlayer())) {
          tech->Look();
        }
      }
    }
  }
}

void DisplayClass::Constrained_Look(Coordinate center, int distance) {
  // The distance was a 16-bit lepton word; a scenario's oversized gap radius
  // still wraps the way it always did.
  const int range = static_cast<uint16_t>(distance);
  for (int index = 0; index < Layer.at(LAYER_GROUND).Count(); index++) {
    ObjectClass* object = Layer.at(LAYER_GROUND).at(index);
    if (object != nullptr && object->Is_Techno()) {
      auto* tech = dynamic_cast<TechnoClass*>(object);

      //			if (tech->What_Am_I() == RTTI_BUILDING &&
      // units_only) continue;

      if (tech->House->IsPlayerControl) {
        if (tech->IsDiscoveredByPlayer &&
            Distance(tech->Center_Coord(), center) <=
                (tech->Techno_Type_Class()->SightRange *
                 game::kCellWidthLeptons) +
                    range) {
          object->Look();
        }
      } else {
        if (tech->What_Am_I() == RTTI_BUILDING && TheRules().IsAllyReveal &&
            tech->House->Is_Ally(ThePlayer()) &&
            Distance(tech->Center_Coord(), center) <=
                (tech->Techno_Type_Class()->SightRange *
                 game::kCellWidthLeptons) +
                    range) {
          tech->Look();
        }
      }
    }
  }
}

/***********************************************************************************************
 * DisplayClass::Flag_Cell -- Flag the specified cell to be redrawn. *
 *                                                                                             *
 *    This will flag the cell to be redrawn. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to be flagged. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/20/1996 JLB : Created. *
 *=============================================================================================*/
void DisplayClass::Flag_Cell(Cell cell) {
  Flag_To_Redraw(false);
  IsDisplayToRedraw = true;
  CellRedraw.set(base::ToSize(cell.index()));
}

}  // namespace ra
