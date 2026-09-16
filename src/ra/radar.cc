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

/* $Header: /CounterStrike/RADAR.CPP 3     3/12/97 2:35p Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : RADAR.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/15/94 *
 *                                                                                             *
 *                  Last Update : September 16, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Get_Multi_Color -- Get the multi color offset number *
 *   RadarClass::AI -- Processes radar input (non-tactical). *
 *   RadarClass::Cell_On_Radar -- Determines if a cell is currently visible on
 *radar.          * RadarClass::Click_Cell_Calc -- Determines what cell the
 *pixel coordinate is over.         * RadarClass::Click_In_Radar -- Check to see
 *if a click is in radar map                     * RadarClass::Click_In_Radar --
 *Converts a radar click into cell X and Y coordinate.        *
 *   RadarClass::Draw_It -- Displays the radar map of the terrain. *
 *   RadarClass::Draw_Names -- draws players' names on the radar map *
 *   RadarClass::Get_Jammed -- Fetch the current radar jammed state for the
 *player.            * RadarClass::Init_Clear -- Sets the radar map to a known
 *state                             * RadarClass::Is_Radar_Active -- Determines
 *if the radar map is currently being displayed.  *
 *   RadarClass::Is_Radar_Existing -- Queries to see if radar map is available.
 ** RadarClass::Is_Zoomable -- Determines if the map can be zoomed. *
 *   RadarClass::Map_Cell -- Updates radar map when a cell becomes mapped. *
 *   RadarClass::One_Time -- Handles one time processing for the radar map. *
 *   RadarClass::Player_Names -- toggles the Player-Names mode of the radar map
 ** RadarClass::Plot_Radar_Pixel -- Updates the radar map with a terrain pixel.
 ** RadarClass::RTacticalClass::Action -- I/O function for the radar map. *
 *   RadarClass::RadarClass -- Default constructor for RadarClass object. *
 *   RadarClass::Radar_Activate -- Controls radar activation. *
 *   RadarClass::Radar_Anim -- Renders current frame of radar animation *
 *   RadarClass::Radar_Cursor -- Adjust the position of the radar map cursor. *
 *   RadarClass::Radar_Pixel -- Mark a cell to be rerendered on the radar map. *
 *   RadarClass::Radar_Position -- Returns with the current position of the
 *radar map.         * RadarClass::Refresh_Cells -- Intercepts refresh request
 *and updates radar if needed       * RadarClass::Render_Infantry -- Displays
 *objects on the radar map.                         * RadarClass::Render_Overlay
 *-- Renders an icon for given overlay                           *
 *   RadarClass::Render_Terrain -- Render the terrain over the given cell *
 *   RadarClass::Set_Map_Dimensions -- Sets the tactical map dimensions. *
 *   RadarClass::Set_Radar_Position -- Sets the radar position to center around
 *specified cell.* RadarClass::Set_Tactical_Position -- Called when setting the
 *tactical display position.   * RadarClass::Set_Tactical_Position -- Called
 *when setting the tactical display position.   *
 *   RadarClass::Set_Tactical_Position -- Sets the map's tactical position and
 *adjusts radar to* RadarClass::Zoom_Mode() -- Handles toggling zoom on the
 *map                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/radar.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <span>
#include <string_view>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "ra/bench_util.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/compat.h"
#include "ra/conquer.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/display.h"
#include "ra/display_constants.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/infantry.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/map.h"
#include "ra/mapedit.h"
#include "ra/mouse.h"
#include "ra/object.h"
#include "ra/session.h"
#include "ra/shapebtn.h"
#include "ra/sidebar.h"
#include "ra/techno.h"
#include "ra/terrain.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "ra/ww_audio.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/disk_file.h"
#include "tech/mix_archive.h"

// void const * RadarClass::CoverShape;
RadarClass::RTacticalClass RadarClass::RadarButton;

std::span<const std::byte> RadarClass::RadarAnim = {};
std::span<const std::byte> RadarClass::RadarPulse = {};
std::span<const std::byte> RadarClass::RadarFrame = {};

static bool FullRedraw = false;

static GraphicBufferClass IconStage(3, 3);
static GraphicBufferClass TileStage(24, 24);

/***********************************************************************************************
 * RadarClass::RadarClass -- Default constructor for RadarClass object. *
 *                                                                                             *
 *    This default constructor merely sets the radar specific values to default
 *settings. The  * radar must be deliberately activated in order for it to be
 *displayed.                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/16/1994 JLB : Created. *
 *=============================================================================================*/
RadarClass::RadarClass() = default;

/***********************************************************************************************
 * RadarClass::One_Time -- Handles one time processing for the radar map. *
 *                                                                                             *
 *    This routine handles any one time processing required in order for the
 *radar map to      * function. This actually only requires an allocation of the
 *radar staging buffer. This    * buffer is needed for those cases where the
 *radar area of the page is being destroyed     * and it needs to be destroyed.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Be sure to call this routine only ONCE. *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
void RadarClass::One_Time() {
  RadWidth = 160;
  RadHeight = 140;
  RadX = SeenBuff.Get_Width() - RadWidth;
  RadY = 14;
  RadPWidth = 128;
  RadPHeight = 128;
  RadOffX = 6;
  RadOffY = 7;
  RadIWidth = 128 + 18;  //************
  RadIHeight = 128 + 2;  //************

  DisplayClass::One_Time();
  RadarButton.X = RadX;
  RadarButton.Y = RadY;
  RadarButton.Width = RadWidth;
  RadarButton.Height = RadHeight;
}

/***********************************************************************************************
 * RadarClass::Init_Clear -- Sets the radar map to a known state. *
 *                                                                                             *
 *    This routine is used to initialize the radar map at the start of the
 *scenario. It        * sets the radar map position and starts it in the
 *disabled state.                         *
 *                                                                                             *
 * INPUT:   theater  -- The theater that the scenario is starting (unused by
 *this routine).    *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Init_Clear() {
  DisplayClass::Init_Clear();
  IsRadarActive = false;
  IsRadarToRedraw = true;
  RadarCursorRedraw = true;
  IsRadarActivating = false;
  IsRadarDeactivating = false;
  DoesRadarExist = false;
  PixelPtr = 0;
  IsPlayerNames = false;

  /*
  ** If we have a valid map lets make sure that we set it correctly
  */
  if (MapCellWidth || MapCellHeight) {
    IsZoomed = false;
    Zoom_Mode(Coord_Cell(Map.TacticalCoord));
  }
}

/***********************************************************************************************
 * RadarClass::Radar_Activate -- Controls radar activation. *
 *                                                                                             *
 *    Use this routine to turn the radar map on or off. *
 *                                                                                             *
 * INPUT:   control  -- What to do with the radar map: * 0 = Turn radar off. *
 *                      1 = Turn radar on. * 2 = Remove Radar Gadgets * 3 = Add
 *Radar Gadgets                                                  * 4 = Remove
 *radar.                                                      * -1= Toggle radar
 *on or off.                                            *
 *                                                                                             *
 * OUTPUT:  bool; Was the radar map already on? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/11/1994 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Radar_Activate(int control) {
  const bool old = IsRadarActive;

  switch (control) {
    /*
    ** Toggle the state of the radar map on or off.
    */
    case -1: {
      const bool temp = !static_cast<bool>(IsRadarActive);
      if (temp) {
        Radar_Activate(1);
      } else {
        Radar_Activate(0);
      }
    } break;

    /*
    ** Turn the radar map off properly.
    */
    case 0:
      if (Map.IsSidebarActive) {
        if (IsRadarActive && !IsRadarDeactivating) {
          Sound_Effect(VOC_RADAR_OFF);
          IsRadarDeactivating = true;
          IsRadarActive = false;
          if (static_cast<bool>(IsRadarActivating)) {
            IsRadarActivating = false;
          } else {
            RadarAnimFrame = kRadarActivatedFrame;
          }
        }
      } else {
        Radar_Activate(2);
      }
      return old;

    case 1:
      if (Map.IsSidebarActive) {
        if (!IsRadarActivating && !IsRadarActive) {
          Sound_Effect(VOC_RADAR_ON);
          IsRadarActivating = true;
          if (static_cast<bool>(IsRadarDeactivating)) {
            IsRadarDeactivating = false;
          } else {
            if (DoesRadarExist) {
              RadarAnimFrame = kMaxRadarFrames;
            } else {
              RadarAnimFrame = 0;
            }
          }
        }
      } else {
        Radar_Activate(3);
      }
      return old;

    case 2:
      if (Session.Type == GAME_NORMAL) {
        SidebarClass::Zoom.Disable();
      } else {
        SidebarClass::Zoom.Enable();
      }
      IsRadarActive = false;
      IsRadarActivating = false;
      IsRadarDeactivating = false;
      break;

    case 3:
      if (Session.Type == GAME_NORMAL && Is_Zoomable()) {
        SidebarClass::Zoom.Enable();
      }
      IsRadarActive = true;
      IsRadarActivating = false;
      IsRadarDeactivating = false;
      break;

    case 4:
      IsRadarActive = false;
      IsRadarActivating = false;
      IsRadarDeactivating = false;
      DoesRadarExist = false;
      Flag_To_Redraw(false);
      IsRadarToRedraw = true;
      break;

    default:
      break;
  }

  if (IsRadarActive != old) {
    IsRadarToRedraw = true;
    Flag_To_Redraw(false);
  }
  FullRedraw = IsRadarActive;
  return old;
}

/***********************************************************************************************
 * RadarClass::Draw_It -- Displays the radar map of the terrain. *
 *                                                                                             *
 *    This is used to display the radar map that appears in the lower * right
 *corner. The main changes to this map are the vehicles and * structure pixels.
 **
 *                                                                                             *
 * INPUT:      none *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/24/1991 JLB : Created. * 05/08/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void RadarClass::Draw_It(bool forced) {
  DisplayClass::Draw_It(forced);

  static const char* _hiresradarnames[] = {
      "natoradr.shp",  // HOUSE_SPAIN,
      "natoradr.shp",  // HOUSE_GREECE,
      "ussrradr.shp",  // HOUSE_USSR,
      "natoradr.shp",  // HOUSE_ENGLAND,
      "ussrradr.shp",  // HOUSE_UKRAINE,
      "natoradr.shp",  // HOUSE_GERMANY,
      "natoradr.shp",  // HOUSE_FRANCE,
      "natoradr.shp",  // HOUSE_TURKEY,
      "natoradr.shp",  // HOUSE_GOOD
      "ussrradr.shp",  // HOUSE_BAD
  };
  static const char* _frames[] = {
      "nradrfrm.shp",  // HOUSE_SPAIN,
      "nradrfrm.shp",  // HOUSE_GREECE,
      "uradrfrm.shp",  // HOUSE_USSR,
      "nradrfrm.shp",  // HOUSE_ENGLAND,
      "uradrfrm.shp",  // HOUSE_UKRAINE,
      "nradrfrm.shp",  // HOUSE_GERMANY,
      "nradrfrm.shp",  // HOUSE_FRANCE,
      "nradrfrm.shp",  // HOUSE_TURKEY,
      "nradrfrm.shp",  // HOUSE_GOOD
      "uradrfrm.shp",  // HOUSE_BAD
  };

  /*
  **	Don't perform any rendering if none is requested.
  */
  if (!forced && !IsRadarToRedraw && !FullRedraw) {
    return;
  }

  BStart(BENCH_RADAR);

  static HousesType _house = HOUSE_NONE;

  if (PlayerPtr->ActLike != _house) {
    char name[kMaxFname + kMaxExt];

    //		port::SafeCopy(name, "NATORADR.SHP" );
    //		if (Session.Type == GAME_NORMAL) {
    port::SafeCopy(
        name, base::At(_hiresradarnames, static_cast<int>(PlayerPtr->ActLike)));
    //		}
#ifndef NDEBUG
    DiskFile file(name);
    if (file.IsAvailable()) {
      RadarAnim = Load_Alloc_Data(file);
    } else {
      RadarAnim = MixArchive::RetrieveData(name);
    }
    port::SafeCopy(name, "PULSE.SHP");
    DiskFile file2(name);
    if (file2.IsAvailable()) {
      RadarPulse = Load_Alloc_Data(file2);
    } else {
      RadarPulse = MixArchive::RetrieveData(name);
    }
    port::SafeCopy(name,
                   base::At(_frames, static_cast<int>(PlayerPtr->ActLike)));
    DiskFile file3(name);
    if (file3.IsAvailable()) {
      RadarFrame = Load_Alloc_Data(file3);
    } else {
      RadarFrame = MixArchive::RetrieveData(
          base::At(_frames, static_cast<int>(PlayerPtr->ActLike)));
    }
#else
    RadarAnim = MixArchive::RetrieveData(name);
    port::SafeCopy(name, "PULSE.SHP");
    DiskFile file3(name);
    if (file3.IsAvailable()) {
      RadarPulse = Load_Alloc_Data(file3);
    } else {
      RadarPulse = MixArchive::RetrieveData(name);
    }
    RadarFrame = MixArchive::RetrieveData(_frames[PlayerPtr->ActLike]);
#endif
    _house = PlayerPtr->ActLike;
  }

  /*
  ** If in player name mode, just draw player names
  */
  if (IsPlayerNames) {
    Draw_Names();
    IsRadarToRedraw = false;
    BEnd(BENCH_RADAR);
    return;
  }

  /*
  ** If in spy-on-radar facility mode, draw the appropriate info.
  */
  if (IsHouseSpy) {
    IsRadarToRedraw = false;
    if (Draw_House_Info()) {
      BEnd(BENCH_RADAR);
      return;
    }
  }

  if (IsRadarActivating || IsRadarDeactivating || IsRadarJammed) {
    Radar_Anim();
    MouseClass::Repair.Draw_Me(true);
    MouseClass::Upgrade.Draw_Me(true);
    MouseClass::Zoom.Draw_Me(true);
    IsRadarToRedraw = false;
    BEnd(BENCH_RADAR);
    return;
  }

  if (Map.IsSidebarActive) {
    if (IsRadarActive) {
      /*
      **	If only a few of the radar pixels need to be redrawn, then find
      *and redraw *	only these.
      */
      if (!forced && IsRadarToRedraw && !FullRedraw && !IsPulseActive) {
        IsRadarToRedraw = false;

        if (PixelPtr) {
          /*
          **	Render all pixels in the "to redraw" stack.
          */
          if (LogicPage->Lock()) {
            for (int index = 0; index < PixelPtr; index++) {
              const CELL cell = base::At(PixelStack, index);
              if (Cell_On_Radar(cell)) {
                (*this)[cell].IsPlot = false;
                Plot_Radar_Pixel(cell);
                RadarCursorRedraw |= (*this)[cell].IsRadarCursor;
              }
            }
            LogicPage->Unlock();
          }

          /*
          **	Refill the stack if there is pending pixels yet to be plotted.
          **	This should only process in sections for speed reasons
          */
          if (PixelPtr == kPixelstack) {
            PixelPtr = 0;

            for (int y = 0; y < MapCellHeight; y++) {
              for (int x = 0; x < MapCellWidth; x++) {
                const CELL cell = XY_Cell(MapCellX + x, MapCellY + y);
                if (Cell_On_Radar(cell) && (*this)[cell].IsPlot) {
                  base::At(PixelStack, PixelPtr++) = cell;
                  IsRadarToRedraw = true;
                  if (PixelPtr == kPixelstack) {
                    break;
                  }
                }
              }
              if (PixelPtr == kPixelstack) {
                break;
              }
            }
          } else {
            PixelPtr = 0;
          }
        }

        Radar_Cursor(RadarCursorRedraw);

      } else {
        GraphicViewPortClass* oldpage = Set_Logic_Page(HidPage);

        CC_Draw_Shape(RadarFrame, 1, RadX, RadY + 2, WINDOW_MAIN,
                      SHAPE_NORMAL);
        if (BaseX || BaseY) {
          if (!IsZoomed && BaseX && BaseY && RadarWidth < RadIWidth - 1 &&
              RadarHeight < RadIHeight - 1) {
            LogicPage->Draw_Rect(
                RadX + RadOffX + BaseX - 1, RadY + RadOffY + BaseY - 1,
                RadX + RadOffX + BaseX + RadarWidth,
                //													RadX
                //+ RadOffX + BaseX + RadarWidth +1,
                RadY + RadOffY + BaseY + RadarHeight,
                //													RadY
                //+ RadOffY + BaseY + RadarHeight +1,
                kWhite);
          }
        } else {
          LogicPage->Fill_Rect(RadX + RadOffX, RadY + RadOffY,
                               RadX + RadOffX + RadIWidth - 1,
                               RadY + RadOffY + RadIHeight - 1, kBlack);
        }

        /*
        ** Draw the entire radar map.
        */
        if (LogicPage->Lock()) {
          for (int index = 0; index < MAP_CELL_TOTAL; index++) {
            if (In_Radar(static_cast<CELL>(index)) && Cell_On_Radar(static_cast<CELL>(index))) {
              Plot_Radar_Pixel(static_cast<CELL>(index));
            }
          }
          if (IsPulseActive) {
            CC_Draw_Shape(RadarPulse, RadarPulseFrame++, RadX + RadOffX,
                          RadY + 2, WINDOW_MAIN, SHAPE_NORMAL);
          }
          LogicPage->Unlock();
        }

        Radar_Cursor(true);
        FullRedraw = false;
        IsRadarToRedraw = false;

        MouseClass::Repair.Draw_Me(true);
        MouseClass::Upgrade.Draw_Me(true);
        MouseClass::Zoom.Draw_Me(true);

        if (oldpage == &SeenBuff) {
          Hide_Mouse();
          LogicPage->Blit(SeenBuff, RadX, RadY, RadX, RadY, RadWidth,
                          RadHeight);
          Show_Mouse();
        }

        Set_Logic_Page(oldpage);
      }

    } else {
      /*
      **	If the radar is not active, then only draw the cover plate if
      *forced to do so.
      */
      const int val = DoesRadarExist ? kMaxRadarFrames : 0;
      CC_Draw_Shape(RadarAnim, val, RadX, RadY + 2, WINDOW_MAIN,
                    SHAPE_NORMAL);
      FullRedraw = false;
      IsRadarToRedraw = false;

      /*
      **	Display the country name on the cover plate when in multi play
      *only.
      */
      if (Session.Type != GAME_NORMAL) {
        Fancy_Text_Print(
            Text_String(
                HouseTypeClass::As_Reference(PlayerPtr->ActLike).Full_Name()),
            RadX + (RadWidth / 2), RadY + RadHeight - 20,
            &ColorRemaps[PlayerPtr->RemapColor], kTBlack,
            TPF_CENTER | kTpfText | TPF_DROPSHADOW);
      }

      MouseClass::Repair.Draw_Me(true);
      MouseClass::Upgrade.Draw_Me(true);
      MouseClass::Zoom.Draw_Me(true);
    }
  }
  BEnd(BENCH_RADAR);
}

/***************************************************************************
 * RadarClass::Render_Terrain -- Render the terrain over the given cell    *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/12/1995 PWG : Created.                                             *
 *=========================================================================*/
void RadarClass::Render_Terrain(CELL cell, int x, int y, int size) const {
  TerrainClass* list[4] = {nullptr, nullptr, nullptr, nullptr};
  int listidx = 0;

  ObjectClass* obj = Map[cell].Cell_Occupier();

  /*
  ** If the cell is occupied by a terrain type, add it to the sortable
  ** list.
  */
  if (obj && obj->What_Am_I() == RTTI_TERRAIN) {
    base::At(list, listidx++) = dynamic_cast<TerrainClass*>(obj);
  }

  /*
  ** Now loop through all the occupiers and add them to the list if they
  ** are terrain type.
  */
  for (int lp = 0; lp < std::ssize(Map[cell].Overlappers); lp++) {
    obj = base::At(Map[cell].Overlappers, lp);
    if (obj && obj->What_Am_I() == RTTI_TERRAIN) {
      base::At(list, listidx++) = dynamic_cast<TerrainClass*>(obj);
    }
  }

  /*
  ** If there are no entries in our list then just get out.
  */
  if (!listidx) {
    return;
  }

  /*
  **	If there is terrain in this cell then draw a dark pixel to
  ** represent it.
  */
  if (size == 1) {
    LogicPage->Put_Pixel(x, y, 21);
    //		LogicPage->Put_Pixel(x, y, 60);
    return;
  }

  /*
  ** Sort the list by its sort Y value so that we can render in the proper
  ** order.
  */
  for (int lp = 0; lp < listidx - 1; lp++) {
    for (int lp2 = lp + 1; lp2 < listidx; lp2++) {
      if (base::At(list, lp)->Sort_Y() > base::At(list, lp2)->Sort_Y()) {
        TerrainClass* terrain = base::At(list, lp);
        base::At(list, lp) = base::At(list, lp2);
        base::At(list, lp2) = terrain;
      }
    }
  }

  /*
  ** loop through the list and take care of rendering the correct icon.
  */
  for (int lp = 0; lp < listidx; lp++) {
    const auto icon = base::At(list, lp)->Radar_Icon(cell);
    if (icon.empty()) {
      continue;
    }
    Buffer_To_Page(0, 0, 3, 3, icon, IconStage);
    IconStage.Scale(*LogicPage, 0, 0, x, y, 3, 3, ZoomFactor, ZoomFactor, true,
                    FadingBrighten);
  }
}

/***********************************************************************************************
 * RadarClass::Render_Infantry -- Displays objects on the radar map. *
 *                                                                                             *
 *    This routine will display an object imagery at the location specified
 *according to the   * condition of the specified cell. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to use as reference when drawing the radar pixel.
 **
 *                                                                                             *
 *          x,y   -- The pixel coordinate to render the radar "pixel" at. *
 *                                                                                             *
 *          size  -- The size of the "pixel". When zoomed in, this value will be
 *"3".          *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/17/1995 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Render_Infantry(CELL cell, int x, int y, int size) {
  ObjectClass* obj = Map[cell].Cell_Occupier();
  while (obj) {
    if (obj->Is_Techno() &&
        dynamic_cast<TechnoClass*>(obj)->Is_Visible_On_Radar()) {
      unsigned char color =
          ColorRemaps[dynamic_cast<TechnoClass*>(obj)->House->RemapColor].Bar;
      int xoff = 0;
      int yoff = 0;
      const int subsize = std::max(1, size / 3);

      switch (obj->What_Am_I()) {
        case RTTI_INFANTRY:
          xoff = (Coord_XLepton(obj->Coord) / (CELL_LEPTON_W / (size + 1))) -
                 (subsize / 2);
          xoff = std::max(xoff, 0);
          xoff = std::min(xoff, size - subsize);
          yoff = (Coord_YLepton(obj->Coord) / (CELL_LEPTON_H / (size + 1))) -
                 (subsize / 2);
          yoff = std::max(yoff, 0);
          yoff = std::min(yoff, size - subsize);

          /*
          ** Draw the infantryman's pixel.  If he's a spy, draw in my house
          *color
          */
          if (*dynamic_cast<InfantryClass*>(obj) == INFANTRY_SPY) {
            color = ColorRemaps[PlayerPtr->RemapColor].Bar;
          }
          LogicPage->Fill_Rect(x + xoff, y + yoff, x + xoff + (subsize - 1),
                               y + yoff + (subsize - 1), color);
          break;

        case RTTI_UNIT:
        case RTTI_VESSEL:
        case RTTI_AIRCRAFT:
          LogicPage->Fill_Rect(x, y, x + size - 1, y + size - 1, color);
          break;

        default:
          break;
      }
    }
    obj = obj->Next;
  }
}

/***************************************************************************
 * RadarClass::Render_Overlay -- Renders an icon for given overlay         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/18/1995 PWG : Created.                                             *
 *=========================================================================*/
void RadarClass::Render_Overlay(CELL cell, int x, int y, int size) {
  // int lpx,lpy;

  const OverlayType overlay = (*this)[cell].Overlay;
  if (overlay != OVERLAY_NONE) {
    const OverlayTypeClass* otype = &OverlayTypeClass::As_Reference(overlay);

    if (otype->IsRadarVisible) {
      const auto icon = otype->Radar_Icon((*this)[cell].OverlayData);
      if (icon.empty()) {
        return;
      }
      Buffer_To_Page(0, 0, 3, 3, icon, IconStage);
      if (otype->IsTiberium) {
        if (size == 1) {
          LogicPage->Put_Pixel(x, y, DKGREY);

          //					_IconStage.Scale(*LogicPage, 0,
          // 0, x, y, 3, 3, size, size, true, (char *)&FadingShade[0]);
        } else {
          IconStage.Scale(*LogicPage, 0, 0, x, y, 3, 3, size, size, true,
                          FadingYellow);
        }
        //				_IconStage.Scale(*LogicPage, 0, 0, x, y,
        // 3, 3, size, size, true, (char *)&FadingGreen[0]);
        // } else { 				_IconStage.Scale(*LogicPage, 0,
        // 0, x, y, 3, 3, size, size, true, (char *)&FadingBrighten[0]);
      }

    }
  }
}

/***************************************************************************
 * RadarClass::Zoom_Mode -- Handles toggling zoom on the map               *
 *                                                                         *
 * INPUT:      none                                                        *
 *                                                                         *
 * OUTPUT:  	none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/29/1995 PWG : Created.                                             *
 *=========================================================================*/
void RadarClass::Zoom_Mode(CELL cell) {
  int map_c_width = 0;
  int map_c_height = 0;

  /*
  ** Set all of the initial zoom mode variables to the correct
  ** setting.
  */
  if (Is_Zoomable()) {
    IsZoomed = !IsZoomed;
  } else {
    IsZoomed = true;
  }
  BaseX = 0;
  BaseY = 0;

  /*
  ** Figure out exactly what size we need to zoom the map to.
  */
  if (!IsZoomed) {
    const int xfactor = RadIWidth / MapCellWidth;
    const int yfactor = RadIHeight / MapCellHeight;
    ZoomFactor = std::max(std::min(xfactor, yfactor), 1);
    map_c_width = MapCellWidth;
    map_c_height = MapCellHeight;
  } else {
    ZoomFactor = 3;
    //		ZoomFactor			= 6;
    map_c_width = RadIWidth / ZoomFactor;
    map_c_height = RadIHeight / ZoomFactor;
  }

  /*
  ** Make sure we do not show more cells than are on the map.
  */
  map_c_width = std::min(map_c_width, RadIWidth);
  map_c_width = std::min(map_c_width, MapCellWidth);
  map_c_height = std::min(map_c_height, RadIHeight);
  map_c_height = std::min(map_c_height, MapCellHeight);

  /*
  ** Find the amount of remainder because this will let us calculate
  ** how to center the thing.
  */
  const int rem_x = RadIWidth - (map_c_width * ZoomFactor);
  const int rem_y = RadIHeight - (map_c_height * ZoomFactor);

  /*
  ** Finally mark the map so it shows just as much as it is supposed
  ** to.
  */
  BaseX = rem_x / 2;
  BaseY = rem_y / 2;
  RadarCellWidth = map_c_width;
  RadarCellHeight = map_c_height;
  RadarWidth = RadIWidth - rem_x;
  RadarHeight = RadIHeight - rem_y;

  /*
  ** Set the radar position to the current cell.
  */
  Set_Radar_Position(cell);

  /*
  ** When zoom mode changes then we need to redraw the radar
  ** area.
  */
  IsRadarToRedraw = true;

  /*
  ** Notify the map that we need to redraw a portion
  */
  Flag_To_Redraw(false);

  /*
  ** Since we have made a vast change we must redraw everything
  */
  FullRedraw = true;
}

/***********************************************************************************************
 * RadarClass::Is_Zoomable -- Determines if the map can be zoomed. *
 *                                                                                             *
 *    This will check to see if the zoomed mode of the map would be just the
 *same size as      * the non-zoomed mode. If this is true, then zooming would
 *have no effect, so return       * false indicating that zooming is not
 *allowed.                                            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is zooming allowed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/16/1996 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Is_Zoomable() const {
  CHECK_NE(MapCellWidth, 0);
  CHECK_NE(MapCellHeight, 0);
  const int xfactor = RadIWidth / MapCellWidth;
  const int yfactor = RadIHeight / MapCellHeight;
  const int factor = std::max(std::min(xfactor, yfactor), 1);
  return factor != 3;
}

/***********************************************************************************************
 * RadarClass::Plot_Radar_Pixel -- Updates the radar map with a terrain pixel. *
 *                                                                                             *
 *    This will update the radar map with a pixel. It is used to display *
 *    vehicle positions on the radar map. *
 *                                                                                             *
 * INPUT:   unit  -- Pointer to unit to render at the given position. If * NULL
 *is passed in, then the underlying terrain is                         *
 *                   displayed instead. *
 *                                                                                             *
 *          pos   -- Position on the map to update. *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   This routine does NOT hide the mouse. It is up to you to * do so.
 **
 *                                                                                             *
 * HISTORY: * 06/04/1991 JLB : Created. * 06/21/1991 JLB : Large blips for units
 *& buildings.                                       * 02/14/1994 JLB :
 *Revamped.                                                                *
 *   04/17/1995 PWG : Created. * 04/18/1995 PWG : Created. *
 *=============================================================================================*/
void RadarClass::Plot_Radar_Pixel(CELL cell) {
  if (cell == -1) {
    cell = 1;
  }


  /*
  **	Perform any clipping on the cell coordinate.
  */
  if (!IsRadarActive || static_cast<unsigned>(cell) > MAP_CELL_TOTAL) {
    return;
  }

  if (!In_Radar(cell) || !Cell_On_Radar(cell)) {
    return;
  }

  /*
  ** If we are zoomed in then calculate the pixel based off of the portion
  ** of the map the radar is viewing.
  */
  int x = Cell_X(cell) - RadarX;
  int y = Cell_Y(cell) - RadarY;  // Coordinate of cell location.
  if (static_cast<unsigned>(x) >= static_cast<unsigned>(RadarCellWidth) ||
      static_cast<unsigned>(y) >= static_cast<unsigned>(RadarCellHeight)) {
    return;
  }

  bool usjamming = false;
  if (LogicPage->Lock()) {
    const CellClass* cellptr = &(*this)[cell];
    x = RadX + RadOffX + BaseX + (x * ZoomFactor);
    y = RadY + RadOffY + BaseY + (y * ZoomFactor);

    /*
    **	Determine what (if any) vehicle or unit should be rendered in this blip.
    */
    int color = kTBlack;  // Color of the pixel to plot.
    const auto housebit = base::Bit<uint16_t>(PlayerPtr->Class->House);
    const uint16_t celljammed = (*this)[cell].Jammed;
    const auto jammed =
        static_cast<uint16_t>(celljammed & static_cast<uint16_t>(~housebit));
    if (!jammed && ((*this)[cell].IsMapped || Debug_Unshroud)) {
      // 		if (!jammed && ((*this)[cell].IsVisible ||
      // Debug_Unshroud)) {
      color = cellptr->Cell_Color(true);
      if (celljammed & housebit && color == kTBlack) {
        color = kBlack;  // FadingWayDark[color];
        usjamming = true;
      }
    } else {
      color = kBlack;
    }

    /*
    **	If no color override occurs for this cell, then render the underlying
    **	terrain.
    */
    if (color == kTBlack) {
      if (ZoomFactor > 1) {
        std::span<const std::byte> ptr;
        int icon = 0;

        /*
        **	Fetch the template pointer and template icon number for the
        **	specified cell.
        */
        if (cellptr->TType != TEMPLATE_NONE &&
            cellptr->TType != static_cast<TemplateType>(255)) {
          ptr =
              TemplateTypeClass::As_Reference(cellptr->TType).Get_Image_Data();
          icon = cellptr->TIcon;
        }

        /*
        **	If the template pointer is still NULL, then this means either a
        *clear *	template or an illegal one. Setup for a clear template.
        */
        if (ptr.empty()) {
          ptr =
              TemplateTypeClass::As_Reference(TEMPLATE_CLEAR1).Get_Image_Data();
          icon = cellptr->Clear_Icon();
        }

        const IconsetClass iconset(ptr);
        const auto icondata = iconset.Icon_Data();

        /*
        **	Convert the logical icon number into the actual icon number.
        */
        icon %= 256;
        const auto iconmap = iconset.Map_Data();
        if (icon < 0 || static_cast<size_t>(icon) >= iconmap.size()) {
          LogicPage->Unlock();
          return;
        }
        icon = iconmap[static_cast<size_t>(icon)];

        const size_t offset = static_cast<size_t>(icon) * 24 * 24;
        if (offset > icondata.size() ||
            icondata.size() - offset < size_t{24} * 24) {
          LogicPage->Unlock();
          return;
        }
        const auto data = icondata.subspan(offset, size_t{24} * 24);
        Buffer_To_Page(0, 0, 24, 24, data, TileStage);
        TileStage.Scale(*LogicPage, 0, 0, x, y, 24, 24, ZoomFactor, ZoomFactor,
                        true);
      } else {
        //				LogicPage->Fill_Rect(x, y,
        // x+ZoomFactor-1, y+ZoomFactor-1, cellptr->Cell_Color(false));
        /*BG*/ LogicPage->Put_Pixel(
            x, y, static_cast<unsigned char>(cellptr->Cell_Color(false)));
      }
    } else {
      LogicPage->Fill_Rect(x, y, x + ZoomFactor - 1, y + ZoomFactor - 1,
                           static_cast<unsigned char>(color));
      ///*BG*/		LogicPage->Put_Pixel(x, y, color);
    }
    if (color != kBlack) {
      Render_Overlay(cell, x, y, ZoomFactor);
      Render_Terrain(cell, x, y, ZoomFactor);
      Render_Infantry(cell, x, y, ZoomFactor);
    } else {
      if (usjamming) {
        Render_Infantry(cell, x, y, ZoomFactor);
      }
    }
    LogicPage->Unlock();
  }
}

/***********************************************************************************************
 * RadarClass::Radar_Pixel -- Mark a cell to be rerendered on the radar map. *
 *                                                                                             *
 *    This routine is used to inform the system that a pixel needs to be *
 *    rerendered on the radar map. The pixel(s) will be rendered the * next time
 *the map is refreshed. *
 *                                                                                             *
 * INPUT:   cell  -- The map cell to be rerendered. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/12/1992 JLB : Created. * 05/08/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void RadarClass::Radar_Pixel(CELL cell) {
  if (IsRadarActive && Map.IsSidebarActive && Cell_On_Radar(cell)) {
    IsRadarToRedraw = true;
    (*this)[cell].IsPlot = true;
    if (PixelPtr < kPixelstack) {
      base::At(PixelStack, PixelPtr++) = cell;
    }
  }
}

/***********************************************************************************************
 * RadarClass::Click_In_Radar -- Converts a radar click into cell X and Y
 *coordinate.          *
 *                                                                                             *
 *    This routine will examine the X and Y coordinate and convert them into the
 *X and Y       * cell coordinate value that corresponds to the location. *
 *                                                                                             *
 * INPUT:   x,y   -- The X and Y mouse coordinate already normalized to the
 *radar upper left   * corner. *
 *                                                                                             *
 * OUTPUT:  Returns with success rating in addition, the X and Y values will now
 *hold the      * cell coordinates of the cell the pixel offsets indicated. *
 *             Result 1 = click was in radar region * Result 0 = click was
 *outside radar region completely                            * Result-1 = click
 *in radar area but not on clickable region of radar.            *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/30/1995 PWG : Created. * 07/16/1995 JLB : Recognizes when
 *sidebar is closed now.                                   *
 *=============================================================================================*/
int RadarClass::Click_In_Radar(int& ptr_x, int& ptr_y, bool change) const {
  int x = ptr_x;
  int y = ptr_y;

  /*
  ** If radar is not active the click could have been on a radar point
  */
  if (!IsRadarActive || !Map.IsSidebarActive) {
    return 0;
  }

  x -= RadX + RadOffX;
  y -= RadY + RadOffY;
  if (static_cast<unsigned>(x) < static_cast<unsigned>(RadIWidth) &&
      static_cast<unsigned>(y) < static_cast<unsigned>(RadIHeight)) {
    x -= BaseX;
    y -= BaseY;

    if (static_cast<unsigned>(x) <
            static_cast<unsigned>(RadarWidth + (ZoomFactor - 1)) &&
        static_cast<unsigned>(y) <
            static_cast<unsigned>(RadarHeight + (ZoomFactor - 1))) {
      //		if ((unsigned)x < RadarWidth && (unsigned)y <
      // RadarHeight) {
      x = RadarX + (x / ZoomFactor);
      y = RadarY + (y / ZoomFactor);
      if (change) {
        ptr_x = x;
        ptr_y = y;
      }
      return 1;
    }
    return -1;
  }
  return 0;
}

/***********************************************************************************************
 * RadarClass::Click_Cell_Calc -- Determines what cell the pixel coordinate is
 *over.           *
 *                                                                                             *
 *    This routine will examine the pixel coordinate provided and determine what
 *cell it       * represents. If the radar map is not active or the coordinates
 *are not positioned over    * the radar map, then it will fall into the base
 *class corresponding routine.              *
 *                                                                                             *
 * INPUT:   x,y   -- The pixel coordinate to convert into a cell number. *
 *                                                                                             *
 * OUTPUT:  Returns with the cell number that the coordinate is over or -1 if
 *not over any     * cell. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
CELL RadarClass::Click_Cell_Calc(int x, int y) const {
  const int result = Click_In_Radar(x, y, true);
  switch (result) {
    case 1:
      return XY_Cell(x, y);

    case -1:
      return -1;

    default:
      break;
  }
  return DisplayClass::Click_Cell_Calc(x, y);
}

/***********************************************************************************************
 * RadarClass::Map_Cell -- Updates radar map when a cell becomes mapped. *
 *                                                                                             *
 *    This routine will update the radar map if a cell becomes mapped. *
 *                                                                                             *
 * INPUT:   cell  -- The cell that is being mapped. *
 *                                                                                             *
 *          house -- The house that is doing the mapping. *
 *                                                                                             *
 * OUTPUT:  bool; Was the cell mapped (for the first time) by this routine? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Map_Cell(CELL cell, HouseClass* house) {
  if (DisplayClass::Map_Cell(cell, house)) {
    Radar_Pixel(cell);
    return true;
  }
  return false;
}

void RadarClass::Cursor_Cell(CELL cell, bool value) {
  /*
  ** If this cell is not on the radar don't bother doing anything.
  */
  if (Cell_On_Radar(cell)) {
    const bool temp = (*this)[cell].IsRadarCursor;

    if (temp != value) {
      /*
      **	Record the new state of this cell.
      */
      (*this)[cell].IsRadarCursor = value;

      /*
      **	If we are erasing then erase the cell.
      */
      if (!value) {
        Plot_Radar_Pixel(cell);
      }
    }
  }
}

void RadarClass::Mark_Radar(int x1, int y1, int x2, int y2, bool value,
                            int barlen) {
  /*
  ** First step is to convert pixel coordinates back to a CellX and CellY.
  */
  x1 = RadarX + (x1 / ZoomFactor);
  y1 = RadarY + (y1 / ZoomFactor);
  x2 = RadarX + (x2 / ZoomFactor);
  y2 = RadarY + (y2 / ZoomFactor);

  /*
  ** Now we need to convert the Pixel length to a cell length.
  */
  barlen = (barlen / ZoomFactor) + 1;

  /*
  ** Now lets loop through and mark the map with the proper value.
  */
  for (int lp = 0; lp <= barlen; lp++) {
    /*
    ** Do Horizontal action to upper and lower left corners.
    */
    int x = x1 + lp;
    Cursor_Cell(XY_Cell(x, y1), value);
    Cursor_Cell(XY_Cell(x, y2), value);
    /*
    ** Do Horizontal Action to upper and lower right corners
    */
    x = x2 - lp;
    Cursor_Cell(XY_Cell(x, y1), value);
    Cursor_Cell(XY_Cell(x, y2), value);
    /*
    ** Do Vertical Action to left and right upper corners
    */
    int y = y1 + lp;
    Cursor_Cell(XY_Cell(x1, y), value);
    Cursor_Cell(XY_Cell(x2, y), value);

    /*
    ** Do Vertical action to left and right lower corners.
    */
    y = y2 - lp;
    Cursor_Cell(XY_Cell(x1, y), value);
    Cursor_Cell(XY_Cell(x2, y), value);
  }
}

/***********************************************************************************************
 * RadarClass::Cell_XY_To_Radar_Pixel-- Adjust the position of the radar map
 *cursor.           *
 *                                                                                             *
 *    This routine will adjust the location (and visibility) of the radar * map
 *cursor. It handles all restoration, drawing, and flashing. *
 *                                                                                             *
 * INPUT:   pos   - Cell position for the cursor. If the value is -1 then * the
 *cursor will be hidden. If the value is equal to                        * the
 *last value passed in then cursor flashing will                         * be
 *maintained.                                                             *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/22/1991 JLB : Created. * 11/17/1995 PWG : Created. *
 *=============================================================================================*/
void RadarClass::Cell_XY_To_Radar_Pixel(int cellx, int celly, int& x,
                                        int& y) const {
  x = (cellx - RadarX) * ZoomFactor;
  y = (celly - RadarY) * ZoomFactor;
}

/***********************************************************************************************
 * RadarClass::Jam_Cell -- Updates radar map when a cell becomes jammed. *
 *                                                                                             *
 *    This routine will update the radar map if a cell becomes jammed. *
 *                                                                                             *
 * INPUT:   cell  -- The cell that is being jammed. *
 *                                                                                             *
 *          house -- The house that is doing the jamming. *
 *                                                                                             *
 * OUTPUT:
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/09/1995 BWG : Created. *
 *=============================================================================================*/
bool RadarClass::Jam_Cell(CELL cell, HouseClass* house /*KO, bool shadeit*/) {
  const auto jam = base::Bit<uint16_t>(house->Class->House);
  (*this)[cell].Jammed |= jam;
  if (house != PlayerPtr) {
    Shroud_Cell(cell /*KO, shadeit*/);
  }
  Radar_Pixel(cell);
  return true;
}

/***********************************************************************************************
 * RadarClass::UnJam_Cell -- Updates radar map when a cell becomes jammed. *
 *                                                                                             *
 *    This routine will update the radar map if a cell becomes jammed. *
 *                                                                                             *
 * INPUT:   cell  -- The cell that is being jammed. *
 *                                                                                             *
 *          house -- The house that is doing the jamming. *
 *                                                                                             *
 * OUTPUT:
 **
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/09/1995 BWG : Created. *
 *=============================================================================================*/
bool RadarClass::UnJam_Cell(CELL cell, HouseClass* house) {
  const auto jam = base::Bit<uint16_t>(house->Class->House);
  (*this)[cell].Redraw_Objects();
  (*this)[cell].Jammed &= static_cast<uint16_t>(~jam);
  Radar_Pixel(cell);
  return true;
}

/***********************************************************************************************
 * RadarClass::Radar_Cursor -- Adjust the position of the radar map cursor. *
 *                                                                                             *
 *    This routine will adjust the location (and visibility) of the radar * map
 *cursor. It handles all restoration, drawing, and flashing. *
 *                                                                                             *
 * INPUT:   pos   - Cell position for the cursor. If the value is -1 then * the
 *cursor will be hidden. If the value is equal to                        * the
 *last value passed in then cursor flashing will                         * be
 *maintained.                                                             *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/22/1991 JLB : Created. * 11/17/1995 PWG : Created. *
 *=============================================================================================*/
void RadarClass::Radar_Cursor(bool forced) {
  static int _last_pos = -1;
  static int _last_frame = -1;
  int x1 = 0;
  int y1 = 0;
  int x2 = 0;
  int y2 = 0;

  /*
  ** figure out these function calls as we will need to call them multiple
  *times.
  */
  const int tac_cell = Coord_Cell(TacticalCoord);
  const int tac_cell_x = static_cast<CELL>(Cell_X(static_cast<CELL>(tac_cell)));
  const int tac_cell_y = static_cast<CELL>(Cell_Y(static_cast<CELL>(tac_cell)));
  const int barlen = 6;

  /*
  ** If the current tactical cell is invalid or we haven't moved and we are not
  *forced to redraw then
  ** just skip the redraw process.
  */
  if (tac_cell != -1 && _last_pos == tac_cell &&
      _last_frame == SpecialRadarFrame && !forced) {
    return;
  }

  if (_last_pos != -1) {
    /*
    ** The first thing we need to do is take care of erasing the last radar cell
    *position.  We do this
    ** by converting to pixel coordinates, then adjusting for the pixel coords
    *for the current frame and
    ** finally taking care of calling the erase procedure which will convert the
    *pixel coordinates back
    ** to the cells that need to be redraw.
    **/
    const int last_cell_x =
        static_cast<CELL>(Cell_X(static_cast<CELL>(_last_pos)));
    const int last_cell_y =
        static_cast<CELL>(Cell_Y(static_cast<CELL>(_last_pos)));

    Cell_XY_To_Radar_Pixel(last_cell_x, last_cell_y, x1, y1);
    Cell_XY_To_Radar_Pixel(last_cell_x + Lepton_To_Cell(TacLeptonWidth),
                           last_cell_y + Lepton_To_Cell(TacLeptonHeight), x2,
                           y2);
    x2--;
    y2--;

    /*
    ** Adjust the current coordinates based on the last animation frame.
    */
    x1 -= _last_frame;
    y1 -= _last_frame;
    x2 += _last_frame;
    y2 += _last_frame;

    /*
    ** Finally mark the map (actually remove the marks that indicate the radar
    *cursor was there
    */
    Mark_Radar(x1, y1, x2, y2, false, barlen);
  }

  /*
  ** Find the upper left and lower right corners of the radar cursor.
  ** Remember to adjust x2 and y2 back by one pixel as they will not be
  ** pointing to the right value otherwise.  They point one cell ahead
  ** of where they should.
  */
  Cell_XY_To_Radar_Pixel(tac_cell_x, tac_cell_y, x1, y1);
  Cell_XY_To_Radar_Pixel(tac_cell_x + Lepton_To_Cell(TacLeptonWidth),
                         tac_cell_y + Lepton_To_Cell(TacLeptonHeight), x2, y2);
  x2--;
  y2--;

  /*
  ** Adjust the coordinates based on the current frame of radar animation.
  */
  x1 -= SpecialRadarFrame;
  y1 -= SpecialRadarFrame;
  x2 += SpecialRadarFrame;
  y2 += SpecialRadarFrame;

  Mark_Radar(x1, y1, x2, y2, true, barlen);

  /*
  ** setup a graphic view port class so we can write all the pixels relative
  ** to 0,0 rather than relative to full screen coordinates.
  */
  GraphicViewPortClass* oldpage = Set_Logic_Page(HidPage);
  GraphicViewPortClass draw_window(
      LogicPage->Get_Graphic_Buffer(),
      RadX + RadOffX + BaseX + LogicPage->Get_XPos(),
      RadY + RadOffY + BaseY + LogicPage->Get_YPos(), RadarWidth, RadarHeight);

  draw_window.Draw_Line(x1, y1, x1 + barlen, y1, kLtGreen);
  draw_window.Draw_Line(x1, y1, x1, y1 + barlen, kLtGreen);

  // Draw upper right hand corner
  draw_window.Draw_Line(x2 - barlen, y1, x2, y1, kLtGreen);
  draw_window.Draw_Line(x2, y1, x2, y1 + barlen, kLtGreen);

  // Draw lower left hand corner
  draw_window.Draw_Line(x1, y2 - barlen, x1, y2, kLtGreen);
  draw_window.Draw_Line(x1, y2, x1 + barlen, y2, kLtGreen);

  // Draw lower right hand corner
  draw_window.Draw_Line(x2, y2 - barlen, x2, y2, kLtGreen);
  draw_window.Draw_Line(x2 - barlen, y2, x2, y2, kLtGreen);

  Set_Logic_Page(oldpage);
  _last_pos = tac_cell;
  _last_frame = SpecialRadarFrame;
  RadarCursorRedraw = false;
}

/***************************************************************************
 * RadarClass::Radar_Anim -- Renders current frame of radar animation      *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/19/1995 PWG : Created.                                             *
 *=========================================================================*/
void RadarClass::Radar_Anim() {
  /*
  ** Do nothing if we're in player-name mode
  */
  if (IsPlayerNames) {
    return;
  }

  if (!Map.IsSidebarActive) {
    return;
  }

  GraphicViewPortClass* oldpage = Set_Logic_Page(HidPage);
  GraphicViewPortClass draw_window(
      LogicPage->Get_Graphic_Buffer(), RadX + RadOffX + LogicPage->Get_XPos(),
      RadY + RadOffY + LogicPage->Get_YPos(), RadIWidth, RadIHeight);
// Mono_Set_Cursor(0,0);
  Draw_Box(RadX + RadOffX - 1, RadY + RadOffY - 1, RadIWidth + 2,
           RadIHeight + 2, BOXSTYLE_RAISED, true);
  draw_window.Clear();
  CC_Draw_Shape(RadarAnim, RadarAnimFrame, RadX, RadY + 2,
                WINDOW_MAIN, SHAPE_NORMAL);
  Flag_To_Redraw(false);
  Set_Logic_Page(oldpage);
}

/***********************************************************************************************
 * RadarClass::AI -- Processes radar input (non-tactical). *
 *                                                                                             *
 *    This routine intercepts any player input that concerns the radar map, but
 *not those      * areas that represent the tactical map. These are handled by
 *the tactical map AI          * processor. Primarily, this routine handles the
 *little buttons that border the radar      * map. *
 *                                                                                             *
 * INPUT:   input -- The player input code. *
 *                                                                                             *
 *          x,y   -- Mouse coordinate parameters to use. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/23/1994 JLB : Created. * 12/26/1994 JLB : Moves tactical map
 *with click or drag.                                   * 12/31/1994 JLB : Uses
 *mouse coordinate parameters.                                        *
 *=============================================================================================*/
void RadarClass::AI(KeyNumType& input, int x, int y) {
  /*
  ** Check to see if we need to animate the radar cursor
  */
  if (IsRadarActive && Map.IsSidebarActive && SpecialRadarFrame) {
    SpecialRadarFrame--;
    RadarCursorRedraw = true;
    IsRadarToRedraw = true;
    Flag_To_Redraw(false);
  }

  /*
  ** Check goes here to see if there is enough power to run the radar
  */
  if (IsRadarActivating) {
    if (!DoesRadarExist) {
      RadarAnimFrame++;
      if (RadarAnimFrame < kRadarActivatedFrame) {
        IsRadarToRedraw = true;
        Flag_To_Redraw(false);
      } else {
        DoesRadarExist = true;
        Radar_Activate(3);
      }
    } else {
      RadarAnimFrame--;
      if (RadarAnimFrame > kRadarActivatedFrame) {
        IsRadarToRedraw = true;
        Flag_To_Redraw(false);
      } else {
        Radar_Activate(3);
      }
    }
  }

  /*
  ** Check goes here to see if there is enough power to run the radar
  */
  if (IsRadarDeactivating) {
    RadarAnimFrame++;
    if (RadarAnimFrame == kMaxRadarFrames) {
      IsRadarDeactivating = false;
    } else {
      IsRadarToRedraw = true;
      Flag_To_Redraw(false);
    }
  }

  /*
  ** Check here to see if radar is being jammed, so we can update the
  ** animation with snow.
  */
  if (!IsRadarActivating && !IsRadarDeactivating && IsRadarJammed) {
    RadarAnimFrame++;
    RadarAnimFrame = std::max<int>(RadarAnimFrame, kRadarActivatedFrame);
    if (RadarAnimFrame > 3 + kRadarActivatedFrame) {
      RadarAnimFrame = kRadarActivatedFrame;
    }
    IsRadarToRedraw = true;
    Flag_To_Redraw(false);
  }

  /*
  ** Check here to see if the sonar pulse is active, and if it is, flag the
  ** radar to redraw so the pulse ping will display.
  */
  if (IsPulseActive) {
    Flag_To_Redraw(true);
    IsRadarToRedraw = true;
    if (RadarPulseFrame >= 8) {
      RadarPulseFrame = 0;
      IsPulseActive = false;
    }
  }

  DisplayClass::AI(input, x, y);
}

/***********************************************************************************************
 * RadarClass::RTacticalClass::Action -- I/O function for the radar map. *
 *                                                                                             *
 *    This is the main action function for handling player I/O on the radar map.
 *It processes  * mouse clicks as well as mouse moves. *
 *                                                                                             *
 * INPUT:   flags -- The event flags that trigger this function call. *
 *                                                                                             *
 *          key   -- Reference the keyboard event that applies to the trigger
 *event.           *
 *                                                                                             *
 * OUTPUT:  Should further processing of the input list be aborted? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::RTacticalClass::Action(unsigned flags, KeyNumType& key) {
  int x = 0;
  int y = 0;                      // Sub cell pixel coordinates.
  ObjectClass* object = nullptr;  // what object is in the cell
  ActionType action =
      ACTION_NONE;  // Action possible with currently selected object.

  /*
  **	Force any help label to disappear when the mouse is held over the
  **	radar map.
  */
  if (Map.IsSidebarActive) {
    Map.Help_Text(TXT_NONE);
  }

  if (!Map.IsRadarActive) {
    if (Map.IsSidebarActive) {
      Map.Override_Mouse_Shape(MOUSE_NORMAL);
      //			Map.Override_Mouse_Shape(MOUSE_NORMAL, true);
    }
    return false;
  }

  /*
  ** Disable processing if the player names are up
  */
  if (Map.Is_Player_Names()) {
    GadgetClass::Action(0, key);
    return true;
  }

  /*
  **	Set some working variables that depend on the mouse position. For the
  *press *	or release event, special mouse queuing storage variables are
  *used. Other *	events must use the current mouse position globals.
  */
  if (flags & (kLeftPress | kLeftRelease | kRightPress | kRightRelease)) {
    x = Keyboard->MouseQX;
    y = Keyboard->MouseQY;
  } else {
    x = Get_Mouse_X();
    y = Get_Mouse_Y();
  }

  /*
  **	See if the mouse is over the radar general area, but not yet
  **	over the active region of the radar map. In such a case, the
  **	mouse is overridden to be the normal cursor and no other
  **	action is performed.
  */
  if (x < Map.RadX + Map.RadOffX || x >= Map.RadX + Map.RadIWidth ||
      y < Map.RadY + Map.RadOffY || y >= Map.RadY + Map.RadIHeight) {
    Map.Override_Mouse_Shape(MOUSE_NORMAL);
    return false;
  }

  const int result = Map.Click_In_Radar(x, y, false);

  if (result == 1) {
    CELL cell =
        Map.RadarClass::Click_Cell_Calc(x, y);  // cell num click happened over
    if (cell != -1 && Map.In_Radar(cell)) {
      const bool shadow = !Map[cell].IsMapped &&
                          !Debug_Unshroud;  // is the cell in shadow or not
      //			shadow	= (!Map[cell].IsVisible &&
      //! Debug_Unshroud);
      const int cellx = 12;
      const int celly = 12;  // Sub cell pixel coordinates.

      /*
      **	Determine the object that the mouse is currently over.
      */
      if (!shadow) {
        object = Map.Cell_Object(cell, cellx, celly);
      }

      /*
      **	If there is a currently selected object, then the action to
      *perform if *	the left mouse button were clicked must be determined.
      */
      if (CurrentObject.Count()) {
        if (object) {
          action = CurrentObject[0]->What_Action(object);
        } else {
          action = CurrentObject[0]->What_Action(cell);
        }

        /*
        ** If this is not a valid radar map action then we are not going to do
        ** anything.
        */
        switch (action) {
          case ACTION_MOVE:
          case ACTION_NOMOVE:
          case ACTION_ATTACK:
          case ACTION_ENTER:
          case ACTION_CAPTURE:
          case ACTION_SABOTAGE:
            break;

          default:
            action = ACTION_NONE;
            object = nullptr;
            break;
        }

        /*
        ** On the radar map the only reason we would want the normal cursor to
        ** appear is if we were over one of our own selected units.  Otherwise
        ** we can't move there.
        **/
        if (action == ACTION_NONE) {
          if (object && object->IsSelected) {
            object = nullptr;
          } else {
            action = ACTION_NOMOVE;
          }
        }

        /*
        **	A right mouse button press toggles the zoom mode.
        */
        if (flags & kRightPress) {
          Map.Mouse_Right_Press();
        }

        /*
        **	When the mouse buttons aren't pressed, only the mouse cursor
        *shape is processed. *	The shape changes depending on what object the
        *mouse is currently over and what *	object is currently selected.
        */
        if (flags & kLeftUp) {
          Map.Mouse_Left_Up(-1, shadow, object, action, true);
        }

        /*
        **	Normal actions occur when the mouse button is released. The
        *press event is *	intercepted and possible rubber-band mode is
        *flagged.
        */
        if (flags & kLeftPress) {
          Map.Mouse_Left_Release(cell, cellx, celly, object, action, true);
        }

      } else {
        Map.Set_Default_Mouse(MOUSE_RADAR_CURSOR, !Map.IsZoomed);

        if (flags & kLeftPress) {
          cell = Map.RadarClass::Click_Cell_Calc(x, y);
          if (cell != -1) {
            int cell_x = Cell_X(cell);
            int cell_y = Cell_Y(cell);
            cell_x -= Lepton_To_Cell(Map.TacLeptonWidth) / 2;
            cell_x = std::max(cell_x, Map.MapCellX);
            cell_y -= Lepton_To_Cell(Map.TacLeptonHeight) / 2;
            cell_y = std::max(cell_y, Map.MapCellY);
            cell = XY_Cell(cell_x, cell_y);
            Map.Set_Tactical_Position(Cell_Coord(cell));
            cell = Coord_Cell(Map.DesiredTacticalCoord);
            Map.IsDisplayToRedraw = true;
            Map.Flag_To_Redraw(true);
            Map.SpecialRadarFrame = 4;
          }
        }

        /*
        **	A right mouse button press toggles the zoom mode.
        */
        if (flags & kRightPress) {
          Map.Zoom_Mode(cell);
        }
      }
    }
  }
  if (result == -1) {
    Map.Override_Mouse_Shape(MOUSE_NORMAL, true);
  }
  GadgetClass::Action(0, key);
  return true;
}

/***********************************************************************************************
 * RadarClass::Refresh_Cells -- Intercepts refresh request and updates radar if
 *needed         *
 *                                                                                             *
 *    This routine intercepts the refresh cells request and if it detects that
 *the sidebar     * should be rerendered, it flags the radar map to redraw
 *during the next draw operation.   *
 *                                                                                             *
 * INPUT:   cell  -- The origin cell that the refresh cell offset list is based
 *upon.          *
 *                                                                                             *
 *          list  -- Pointer to the list of offsets from the origin cell that
 *specifies the    * cells to be flagged for redraw. If the list starts with the
 *special       * code to refresh the sidebar, then this routine recognizes it
 *and flags    * the radar map to be redrawn accordingly. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/01/1995 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Refresh_Cells(CELL cell, std::span<const int16_t> list) {
  if (list.front() == kRefreshSidebar) {
    IsRadarToRedraw = true;
    Flag_To_Redraw(false);
  }
  DisplayClass::Refresh_Cells(cell, list);
}

/***********************************************************************************************
 * RadarClass::Set_Radar_Position -- Sets the radar position to center around
 *specified cell.  *
 *                                                                                             *
 *    This routine will try to center the radar map around the cell position
 *specified.        *
 *                                                                                             *
 * INPUT:   cell  -- The cell to try and position the radar map around. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Set_Radar_Position(CELL cell) {
  int oldx = 0;
  int oldy = 0;

  if (ZoomFactor != 1) {
    oldx = Cell_X(cell) - MapCellX;
    oldy = Cell_Y(cell) - MapCellY;
  } else {
    oldx = 0;
    oldy = 0;
  }

  Confine_Rect(&oldx, &oldy, RadarCellWidth, RadarCellHeight, MapCellWidth,
               MapCellHeight);

  const int newx = oldx + MapCellX;
  const int newy = oldy + MapCellY;
  const int newcell = XY_Cell(newx, newy);

  if (RadarCell != newcell) {
    bool forced = false;
    const int xmod = newx;
    const int ymod = newy;

    const int radx =
        static_cast<CELL>(Cell_X(static_cast<CELL>(RadarCell)) - xmod);
    const int rady =
        static_cast<CELL>(Cell_Y(static_cast<CELL>(RadarCell)) - ymod);

    RadarX = newx;
    RadarY = newy;
    RadarCell = newcell;

    if (Map.IsSidebarActive && Map.IsRadarActive) {
      const int radw = RadarCellWidth - std::abs(radx);   // Replicable width.
      const int radh = RadarCellHeight - std::abs(rady);  // Replicable height.

      if (radw < 1) {
        forced = true;
      }
      if (radh < 1) {
        forced = true;
      }

      if (!forced && (radw != RadarWidth || radh != RadarHeight)) {
        /*
        ** Blit the section that is actually overlapping.
        **
        ** If the video card isnt able to blit overlapped regions then we have
        ** to do the blit in two stages via an intermediate buffer. The test to
        *allow
        ** overlapped blits is done in the library at the time of setting the
        *video mode.
        */
        if (OverlappedVideoBlits || !HidPage.Get_IsDirectDraw()) {
          /*
          ** Overlapped blits are OK or we dont have a video memory hid page so
          *blits are
          ** always done in software by the library anyway.
          */
          HidPage.Blit(
              HidPage,
              ((radx < 0 ? -radx : 0) * ZoomFactor) + RadX + RadOffX + BaseX,
              ((rady < 0 ? -rady : 0) * ZoomFactor) + RadY + RadOffY + BaseY,
              ((radx < 0 ? 0 : radx) * ZoomFactor) + RadX + RadOffX + BaseX,
              ((rady < 0 ? 0 : rady) * ZoomFactor) + RadY + RadOffY + BaseY,
              radw * ZoomFactor, radh * ZoomFactor);

        } else {
          /*
          ** Create a temporary intermediate surface
          */
          GraphicBufferClass temp_surface;
          temp_surface.Init(((RadarWidth + 16) / 16) * 16,
                            ((RadarHeight + 16) / 16) * 16, {}, 0,
                            GBC_VIDEOMEM);

          /*
          ** Do the blit in 2 stages.
          */
          HidPage.Blit(
              temp_surface,
              ((radx < 0 ? -radx : 0) * ZoomFactor) + RadX + RadOffX + BaseX,
              ((rady < 0 ? -rady : 0) * ZoomFactor) + RadY + RadOffY + BaseY, 0,
              0, RadarWidth, RadarHeight);

          temp_surface.Blit(
              HidPage, 0, 0,
              ((radx < 0 ? 0 : radx) * ZoomFactor) + RadX + RadOffX + BaseX,
              ((rady < 0 ? 0 : rady) * ZoomFactor) + RadY + RadOffY + BaseY,
              radw * ZoomFactor, radh * ZoomFactor);
        }

        /*
        ** Now we need to flag the section of the map that is going to redraw.
        */
        if (radx != 0) {
          int min = 0;
          int max = 0;
          if (radx < 0) {  // this mean regen the right edge
            min = radw;
            max = radw + std::abs(radx);
          } else {  //	this mean regen the left edge
            min = 0;
            max = radx;
          }
          for (int x = min; x < max; x++) {
            for (int y = 0; y < RadarCellHeight; y++) {
              Radar_Pixel(XY_Cell(newx + x, newy + y));
            }
          }
        }
        if (newy != 0) {
          int min = 0;
          int max = 0;
          if (rady < 0) {  // this mean regen the bottom edge
            min = radh;
            max = radh + std::abs(rady);
          } else {  // this mean regen the top edge
            min = 0;
            max = rady;
          }
          for (int y = min; y < max; y++) {
            for (int x = 0; x < RadarCellWidth; x++) {
              Radar_Pixel(XY_Cell(newx + x, newy + y));
            }
          }
        }
      }
    }
    RadarCursorRedraw = IsRadarActive;
    IsRadarToRedraw = IsRadarActive;
    Flag_To_Redraw(false);
    if (ZoomFactor > 4) {
      FullRedraw = forced;
    }
  } else {
    RadarCursorRedraw = IsRadarActive;
    IsRadarToRedraw = IsRadarActive;
    Flag_To_Redraw(false);
  }
}

/***********************************************************************************************
 * RadarClass::Radar_Position -- Returns with the current position of the radar
 *map.           *
 *                                                                                             *
 *    This returns the cell number of the upper left corner of the radar map. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the radar map upper left corner cell position. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
CELL RadarClass::Radar_Position() const { return static_cast<CELL>(RadarCell); }

/***********************************************************************************************
 * RadarClass::Set_Map_Dimensions -- Sets the tactical map dimensions. *
 *                                                                                             *
 *    This routine is called when the tactical map changes its dimensions. This
 *occurs when    * the tactical map moves and when the sidebar pops on or off. *
 *                                                                                             *
 * INPUT:   x,y   -- The cell coordinate of the upper left corner of the
 *tactical map.         *
 *                                                                                             *
 *          w,y   -- The cell width and height of the tactical map. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Set_Map_Dimensions(int x, int y, int w, int h) {
  Set_Radar_Position(XY_Cell(x, y));
  DisplayClass::Set_Map_Dimensions(x, y, w, h);
}

/***********************************************************************************************
 * RadarClass::Set_Tactical_Position -- Sets the map's tactical position and
 *adjusts radar to  *
 *                                                                                             *
 *    This routine is called when the tactical map is to change position. The
 *radar map might  * be adjusted as well by this routine. *
 *                                                                                             *
 * INPUT:   coord -- The new coordinate to use for the upper left corner of the
 *tactical       * map. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/17/1995 JLB : Created. *
 *=============================================================================================*/
void RadarClass::Set_Tactical_Position(COORDINATE coord) {
  DisplayClass::Set_Tactical_Position(coord);
  Set_Radar_Position(Coord_Cell(TacticalCoord));
}

/***********************************************************************************************
 * RadarClass::Cell_On_Radar -- Determines if a cell is currently visible on
 *radar.            *
 *                                                                                             *
 *    This routine will examine the specified cell number and return whether it
 *is visible     * on the radar map. This depends on the radar map position. *
 *                                                                                             *
 * INPUT:   cell  -- The cell number to check. *
 *                                                                                             *
 * OUTPUT:  Is the specified cell visible on the radar map currently? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/03/1995 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Cell_On_Radar(CELL cell) const {
  if (static_cast<unsigned>(cell) > MAP_CELL_TOTAL) {
    return false;
  }

  if (!IsZoomed) {
    return true;
  }
  return Cell_X(cell) - RadarX <= RadarCellWidth &&
         Cell_Y(cell) - RadarY <= RadarCellHeight;
}

/***********************************************************************************************
 * RadarClass::Player_Names -- toggles the Player-Names mode of the radar map *
 *                                                                                             *
 * INPUT: * on         true = turn on; false = turn off *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/07/1995 BRR : Created. *
 *=============================================================================================*/
void RadarClass::Player_Names(bool on) {
  IsPlayerNames = on;
  IsRadarToRedraw = true;
  Flag_To_Redraw(true);  // force drawing of the plate
}

/***********************************************************************************************
 * RadarClass::Spy_Next_House -- advances to the next house we're spying on, or
 *returns NULL	  *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: * 0 = no house to spy on, 1 = found house to spy on *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/20/1996 BWG : Created. *
 *=============================================================================================*/
bool RadarClass::Spy_Next_House() {
  bool tospy = false;
  const auto spiedby = base::Bit<uint32_t>(PlayerPtr->Class->House);

  IsPlayerNames = false;
  IsRadarToRedraw = true;

  int maxhouse = 0;  // One past the last house to consider.
  HousesType firsthouse = HOUSE_NONE;
  HousesType house = HOUSE_NONE;

  if (Session.Type == GAME_NORMAL) {
    firsthouse = HOUSE_SPAIN;
    maxhouse = static_cast<int>(HOUSE_GOOD);
  } else {
    firsthouse = HOUSE_MULTI1;
    maxhouse = static_cast<int>(magic_enum::enum_count<HousesType>());
  }

  if (IsHouseSpy) {
    house = static_cast<HousesType>(static_cast<int>(SpyingOn) + 1);
  } else {
    house = firsthouse;
  }

  house = std::max(house, firsthouse);

  while (static_cast<int>(house) < maxhouse && !tospy) {
    const HouseClass* hptr = HouseClass::As_Pointer(house);
    if ((hptr && hptr->IsActive && hptr != PlayerPtr) &&
        (hptr->RadarSpied & spiedby)) {
      tospy = true;
      SpyingOn = house;
      break;
    }

    house++;
  }

  IsHouseSpy = tospy;

  Flag_To_Redraw(true);  // force drawing of the plate
  return tospy;
}

/***********************************************************************************************
 * Draw_House_Info -- Print house statistics on the radar map
 **
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 03/20/1996 BWG : Created. *
 *=============================================================================================*/
bool RadarClass::Draw_House_Info() {
  char txt[40];
  /*
  ** Do nothing if the sidebar isn't there
  */
  if (!Map.IsSidebarActive) {
    return false;
  }
  CC_Draw_Shape(RadarFrame, 1, RadX, RadY + 2, WINDOW_MAIN,
                SHAPE_NORMAL);
  int y = RadY + RadOffY + 4;

  MouseClass::Repair.Draw_Me(true);
  MouseClass::Upgrade.Draw_Me(true);
  MouseClass::Zoom.Draw_Me(true);

  Fancy_Text_Print(TXT_SPY_INFO, RadX + RadOffX + 12, y,
                   &ColorRemaps[PCOLOR_GREY], kTBlack,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  y += 14;

  HouseClass* ptr = HouseClass::As_Pointer(SpyingOn);
  if (ptr && ptr->RadarSpied & base::Bit<uint32_t>(PlayerPtr->Class->House)) {
    const PlayerColorType c_idx = ptr->RemapColor;
    RemapControlType* color = &ColorRemaps[c_idx];
    const TextPrintType style = TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW;

    /*
    ** Print house's name below 'spy report'
    */
    txt[0] = 0;
    absl::SNPrintF(txt, sizeof(txt), "%s",
                   ptr->IniName);  // Text_String(ptr->Class->FullName));
    //		absl::SNPrintF(txt, sizeof(txt), "%s",
    // ptr->Name());//Text_String(ptr->Class->FullName));
    if (!std::string_view(txt).empty()) {
      if (std::string_view(txt).size() > 9) {
        txt[9] = '.';
        txt[10] = '\0';
      }
      Fancy_Text_Print(txt, RadX + RadOffX + 12, y, color, kBlack, style);
    } else {
      port::SafeCopy(txt, "________");
    }
    y += 12 + 1;

    Fancy_Text_Print(TXT_BUILDNGS, RadX + RadOffX + 12, y,
                     &ColorRemaps[PCOLOR_GREY], kTBlack,
                     TPF_6PT_GRAD | TPF_NOSHADOW);
    y += 12 + 1;

    // count & print buildings
    absl::SNPrintF(txt, sizeof(txt), "%i", ptr->CurBuildings);
    Fancy_Text_Print(txt, RadX + RadOffX + 12, y, color, kBlack, style);
    y += 12 + 1;

    Fancy_Text_Print(TXT_UNITS, RadX + RadOffX + 12, y,
                     &ColorRemaps[PCOLOR_GREY], kTBlack,
                     TPF_6PT_GRAD | TPF_NOSHADOW);
    y += 12 + 1;
    // count & print units
    absl::SNPrintF(txt, sizeof(txt), "%i", ptr->CurUnits);
    Fancy_Text_Print(txt, RadX + RadOffX + 12, y, color, kBlack, style);
    y += 12 + 1;

    Fancy_Text_Print(TXT_INFANTRY, RadX + RadOffX + 12, y,
                     &ColorRemaps[PCOLOR_GREY], kTBlack,
                     TPF_6PT_GRAD | TPF_NOSHADOW);
    y += 12 + 1;
    // count & print infantry
    absl::SNPrintF(txt, sizeof(txt), "%i", ptr->CurInfantry);
    Fancy_Text_Print(txt, RadX + RadOffX + 12, y, color, kBlack, style);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * Draw_Names -- draws players' names on the radar map *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/07/1995 BRR : Created. *
 *=============================================================================================*/
void RadarClass::Draw_Names() const {
  char txt[40];
  RemapControlType* color = nullptr;
  /*
  ** Do nothing if the sidebar isn't there
  */
  if (!Map.IsSidebarActive) {
    return;
  }

  //	CC_Draw_Shape(RadarAnim, kRadarActivatedFrame, RADAR_X, RADAR_Y+1,
  //		WINDOW_MAIN, SHAPE_NORMAL);
  CC_Draw_Shape(RadarFrame, 1, RadX, RadY + 2, WINDOW_MAIN,
                SHAPE_NORMAL);

  int y = RadY + RadOffY + 4;

  Fancy_Text_Print(TXT_NAME_COLON, RadX + RadOffX, y, &ColorRemaps[PCOLOR_GREY],
                   kTBlack, TPF_6PT_GRAD | TPF_NOSHADOW);
  Fancy_Text_Print(TXT_KILLS_COLON, RadX + RadOffX + RadIWidth - 2, y,
                   &ColorRemaps[PCOLOR_GREY], kTBlack,
                   TPF_RIGHT | TPF_6PT_GRAD | TPF_NOSHADOW);
  y += 12 + 1;

  LogicPage->Draw_Line(RadX + RadOffX, y, RadX + RadOffX + RadIWidth - 1, y,
                       kLtGrey);
  y += 4;

  for (HousesType house = HOUSE_MULTI1;
       static_cast<int>(house) <
       static_cast<int>(HOUSE_MULTI1) + Session.MaxPlayers;
       house++) {
    HouseClass* ptr = HouseClass::As_Pointer(house);

    if (!ptr) {
      continue;
    }

    /*
    **	Decode this house's color
    */
    const PlayerColorType c_idx = ptr->RemapColor;

    if (ptr->IsDefeated) {
      color = &GreyScheme;
    } else {
      color = &ColorRemaps[c_idx];
    }
    const TextPrintType style =
        ptr->IsDefeated ? TPF_6PT_GRAD | TPF_NOSHADOW
                        : TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW;

    /*
    **	Initialize our message
    */
    txt[0] = 0;
    //		absl::SNPrintF(txt, sizeof(txt), "%s", ptr->Name());
    absl::SNPrintF(txt, sizeof(txt), "%s",
                   ptr->IsHuman ? ptr->IniName : Text_String(TXT_COMPUTER));

    if (std::string_view(txt).empty()) {
      port::SafeCopy(txt, "________");
    }

    /*
    **	Print the player name, and the # of kills
    */
    if (std::string_view(txt).size() > 9) {
      txt[9] = '.';
      txt[10] = '\0';
    }
    Fancy_Text_Print(txt, RadX + RadOffX, y, color, kTBlack, style);

    int kills = 0;
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      kills += ptr->UnitsKilled[h];
      kills += ptr->BuildingsKilled[h];
    }
    absl::SNPrintF(txt, sizeof(txt), "%2d", kills);
    Fancy_Text_Print(txt, RadX + RadOffX + RadIWidth - 2, y, color, kTBlack,
                     style | TPF_RIGHT);

    y += 12 + 1;
  }

  MouseClass::Repair.Draw_Me(true);
  MouseClass::Upgrade.Draw_Me(true);
  MouseClass::Zoom.Draw_Me(true);
}

void RadarClass::Activate_Pulse() {
  if (IsRadarActive || PlayerPtr->IsGPSActive) {
    IsPulseActive = true;
    RadarPulseFrame = 0;
  }
}

/***********************************************************************************************
 * RadarClass::Is_Radar_Active -- Determines if the radar map is currently being
 *displayed.    *
 *                                                                                             *
 *    Determines if the radar map is currently being displayed. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the radar map currently being displayed as active? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/12/1996 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Is_Radar_Active() const {
  return IsRadarActive || PlayerPtr->IsGPSActive;
  //	return IsRadarActive || PlayerPtr->IsGPSActive;
}

/***********************************************************************************************
 * RadarClass::Is_Radar_Existing -- Queries to see if radar map is available. *
 *                                                                                             *
 *    This will determine if the radar map is available. If available, the radar
 *will show     * representations of terrain, units, and buildings. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the radar map available to be displayed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/12/1996 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Is_Radar_Existing() const {
  return DoesRadarExist || PlayerPtr->IsGPSActive;
}

/***********************************************************************************************
 * RadarClass::Get_Jammed -- Fetch the current radar jammed state for the
 *player.              *
 *                                                                                             *
 *    This will fetch the current state of the radar jamming for the player. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Is the radar currently jammed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/12/1996 JLB : Created. *
 *=============================================================================================*/
bool RadarClass::Get_Jammed() const {
  if (PlayerPtr->IsGPSActive) {
    return false;
  }
  return IsRadarJammed;
}

void RadarClass::Flag_Cell(CELL cell) {
  //	Radar_Pixel(cell);
  DisplayClass::Flag_Cell(cell);
}
