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

/* $Header: /CounterStrike/TERRAIN.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TERRAIN.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 29, 1994 *
 *                                                                                             *
 *                  Last Update : October 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * TerrainClass::AI -- Process the terrain object AI. *
 *   TerrainClass::Can_Enter_Cell -- Determines if the terrain object can exist
 *   in the cell. * TerrainClass::Catch_Fire -- Catches the terrain object on
 *   fire. * TerrainClass::Center_Coord -- Fetches the center point coordinate
 *   for terrain object. * TerrainClass::Draw_It -- Renders the terrain object
 * at the location specified. * TerrainClass::Fire_Out -- Handles when fire has
 *   gone out. * TerrainClass::Heath_Ratio -- Determines the health ratio for
 * the terrain object. * TerrainClass::Init -- Initialize the terrain object
 *   tracking system. * TerrainClass::Limbo -- Handles terrain specific limbo
 *   action. * TerrainClass::Mark -- Marks the terrain object on the map. *
 *   TerrainClass::Radar_Icon -- Fetches pointer to radar icon to use. *
 *   TerrainClass::Read_INI -- Reads terrain objects from INI file. *
 *   TerrainClass::Start_To_Crumble -- Initiates crumbling of terrain (tree)
 *   object. * TerrainClass::Take_Damage -- Damages the terrain object as
 *   specified. * TerrainClass::Target_Coord -- Returns with the target
 *   coordinate. * TerrainClass::TerrainClass -- This is the constructor for a
 *   terrain object. * TerrainClass::Unlimbo -- Unlimbo terrain object onto the
 *   map. * TerrainClass::Write_INI -- Write all terrain objects to the INI
 *   database specified. * TerrainClass::delete -- Deletes a terrain object. *
 *   TerrainClass::new -- Creates a new terrain object. *
 *   TerrainClass::~TerrainClass -- Default destructor for terrain class
 * objects.
 *   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/terrain.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/keyframe.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "game/core/game_clock.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "ra/ai/scenario.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/data/ini_types.h"
#include "ra/data/rules.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/mouse.h"
#include "ra/map/world.h"
#include "ra/objects/anim.h"
#include "ra/objects/cell.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/startup/game_state.h"

namespace ra {

using engine::gfx::ShapeFlags;

/***********************************************************************************************
 * TerrainClass::~TerrainClass -- Default destructor for terrain class objects.
 **
 *                                                                                             *
 *    This is the default destructor for terrain objects. It will remove the
 *object from the   * map and tracking systems, but only if the game is running.
 *Otherwise, it does nothing.   *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/
TerrainClass::~TerrainClass() {
  if (TheGameState().active() && Class) {
    TerrainClass::Limbo();
  }
}

/***********************************************************************************************
 * TerrainClass::Take_Damage -- Damages the terrain object as specified. *
 *                                                                                             *
 *    This routine is called when damage is to be inflicted upon the terrain
 *object. It is     * through this routine that terrain objects are attacked and
 *thereby destroyed. Not all    * terrain objects can be damaged by this routine
 *however.                                  *
 *                                                                                             *
 * INPUT:   damage      -- The damage points to inflict (raw). *
 *                                                                                             *
 *          warhead     -- The warhead type the indicates the kind of damage.
 *This is used to  * determine if the terrain object is damaged and if so, by
 *how much.  *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain object destroyed by this damage? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. * 11/22/1994 JLB : Shares base damage
 *handler for techno objects.                           * 12/11/1994 JLB :
 *Shortens attached burning animations.                                    *
 *=============================================================================================*/
game::ResultType TerrainClass::Take_Damage(int& damage, int distance,
                                           WarheadType warhead,
                                           TechnoClass* source, bool forced) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  game::ResultType res = game::RESULT_NONE;

  /*
  **	Small arms can never destroy a terrain element.
  */
  if ((!IsOnFire || warhead == WARHEAD_FIRE) && warhead != WARHEAD_SA &&
      !Class->IsImmune) {
    res = ObjectClass::Take_Damage(damage, distance, warhead, source, forced);

    if (damage && warhead == WARHEAD_FIRE) {
      Catch_Fire();
    }

    /*
    **	If the terrain object is destroyed by this damage, then only remove it
    *if it *	currently isn't on fire and isn't in the process of crumbling.
    */
    if (res == game::RESULT_DESTROYED) {
      /*
      **	Remove this terrain object from the targeting computers of all
      *other *	game objects. No use beating a dead horse.
      */
      Detach_All();

      if (IsOnFire) {
        /*
        **	Attached flame animation should be shortened as much as possible
        *so that *	crumbling can begin soon.
        */
        Shorten_Attached_Anims(this);
      } else {
        Start_To_Crumble();
      }
    }
  }
  return res;
}

/***********************************************************************************************
 * TerrainClass::new -- Creates a new terrain object. *
 *                                                                                             *
 *    This routine creates a new terrain object by grabbing a free slot * out of
 *the terrain object pool. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the terrain object allocated. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. *
 *=============================================================================================*/
void* TerrainClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().terrain().Allocate();
  if (ptr) {
    static_cast<TerrainClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * TerrainClass::delete -- Deletes a terrain object. *
 *                                                                                             *
 *    This routine deletes a terrain object by returning it to the * terrain
 *object pool. *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the terrain object to delete. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/14/1994 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TerrainClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().terrain().Free(static_cast<TerrainClass*>(ptr));
}

/***********************************************************************************************
 * TerrainClass::TerrainClass -- This is the constructor for a terrain object *
 *                                                                                             *
 *    This constructor for a terrain object will initialize the terrain * object
 *with it's proper type and insert it into the access * tracking system. *
 *                                                                                             *
 * INPUT:   type  -- The terrain object type. *
 *                                                                                             *
 *          cell  -- The location of the terrain object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. *
 *=============================================================================================*/
TerrainClass::TerrainClass(TerrainType type, Cell cell)
    : ObjectClass(RTTI_TERRAIN, TheObjectHeaps().terrain().SlotIndexOf(this)),
      Class(TheTypeHeaps().terrain().ActiveAt(static_cast<int>(type))) {
  Strength = Class->MaxStrength;
  if ((cell != Cell::None()) && (!Unlimbo(cell.Center()))) {
    delete this;
  }

  animation_.SetRate(0);  // turn off animation
}

/***********************************************************************************************
 * TerrainClass::Mark -- Marks the terrain object on the map. *
 *                                                                                             *
 *    This routine will mark or remove the terrain object from the map *
 *    tracking system. This is typically called when the terrain object * is
 *first created, when it is destroyed, and whenever it needs to be * redrawn. *
 *                                                                                             *
 * INPUT:   mark  -- The marking operation to perform. *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain object successfully marked? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. * 12/23/1994 JLB : Performs low level
 *legality check before proceeding.                     *
 *=============================================================================================*/
bool TerrainClass::Mark(MarkType mark) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (ObjectClass::Mark(mark)) {
    const Cell cell = Cell::Containing(Coord);

    switch (mark) {
      case MARK_UP:
        TheMap().Pick_Up(cell, this);
        break;

      case MARK_DOWN:
        TheMap().Place_Down(cell, this);
        break;

      case MarkType::MARK_CHANGE:
      case MarkType::MARK_CHANGE_REDRAW:
      case MarkType::MARK_OVERLAP_DOWN:
      case MarkType::MARK_OVERLAP_UP:
      default:
        TheMap().Refresh_Cells(cell, Overlap_List(true));
        TheMap().Refresh_Cells(cell, Occupy_List());
        break;
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * TerrainClass::Draw_It -- Renders the terrain object at the location
 *specified.              *
 *                                                                                             *
 *    This routine is used to render the terrain object at the location
 *specified and          * clipped to the window specified. This is the
 *gruntwork drawing routine for the           * terrain objects as they are
 *displayed on the map.                                        *
 *                                                                                             *
 * INPUT:   x,y      -- The coordinate to draw the terrain object at (centered).
 **
 *                                                                                             *
 *          window   -- The clipping window to draw to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/27/1994 JLB : Created. * 11/09/1994 JLB : Changed selected
 *terrain highlight method.                               *
 *=============================================================================================*/
void TerrainClass::Draw_It(PixelView& view, int x, int y,
                           WindowNumberType window) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  const auto shapedata = Get_Image_Data();
  if (!shapedata.empty()) {
    int shapenum = 0;

    /*
    **	Determine the animation stage to render the terrain object. If it is
    *crumbling, then *	it will display the crumbling animation.
    */
    if (IsCrumbling) {
      shapenum = animation_.stage() + IsCrumbling;
    } else {
      if (Strength < 2) {
        shapenum++;
      }
    }

    ShapeFlags flags = ShapeFlags::kNone;
    if (IsSelected && TheDebugState().map_editor_active()) {
      flags = flags | ShapeFlags::kFading;
    }

    /*
    **Terrain is always theater specific so flag it as such for BuildFrame
    */
    CC_Draw_Shape(view, shapedata, shapenum, x, y, window,
                  flags | ShapeFlags::kGhost, MouseClass::FadingLight,
                  MouseClass::UnitShadow);
  }
}

/***********************************************************************************************
 * TerrainClass::Init -- Initialize the terrain object tracking system. *
 *                                                                                             *
 *    This routine will clear out the terrain object system so that no terrain
 *objects will    * exists. It is called prior to loading or starting a
 *scenario.                            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::Init() { TheObjectHeaps().terrain().FreeAll(); }

/***********************************************************************************************
 * TerrainClass::Can_Enter_Cell -- Determines if the terrain object can exist in
 *the cell.     *
 *                                                                                             *
 *    This routine will examine the cell specified and determine if the the
 *terrain object     * can legally exist there. *
 *                                                                                             *
 * INPUT:   cell  -- The cell to examine. *
 *                                                                                             *
 * OUTPUT:  If the terrain object can be placed in the cell specified, then a
 *value less than  * 256 will be returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. * 01/01/1995 JLB : Actually works now. *
 *=============================================================================================*/
game::MoveType TerrainClass::Can_Enter_Cell(Cell cell,
                                            game::Facing /*unused*/) const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (static_cast<unsigned>(cell.index()) >= MAP_CELL_TOTAL) {
    return game::MOVE_NO;
  }

  std::span<const int16_t> offset =
      Occupy_List();  // Pointer to cell offset list.
  while (offset.front() != kRefreshEol) {
    if (Class->IsWaterBased) {
      if (!TheMap()
               .at(cell.Offset(base::ConsumeFront(offset)))
               .Is_Clear_To_Build(SPEED_FLOAT)) {
        return game::MOVE_NO;
      }
    } else {
      if (!TheMap()
               .at(cell.Offset(base::ConsumeFront(offset)))
               .Is_Clear_To_Build()) {
        return game::MOVE_NO;
      }
    }
  }
  return game::MOVE_OK;
}

/***********************************************************************************************
 * TerrainClass::Catch_Fire -- Catches the terrain object on fire. *
 *                                                                                             *
 *    This routine is called if the terrain object is supposed to catch on fire.
 *The routine   * performs checking to make sure that only flammable terrain
 *objects that aren't already   * on fire get caught on fire. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain object caught on fire by this routine? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/27/1994 JLB : Created. * 12/11/1994 JLB : Don't catch fire if
 *already on fire or crumbling.                        *
 *=============================================================================================*/
bool TerrainClass::Catch_Fire() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (!IsCrumbling && !IsOnFire && Class->Armor == game::ARMOR_WOOD) {
    auto* anim = base::NewPooled<AnimClass>(
        ANIM_BURN_BIG, Sort_Y() + Coordinate::FromLeptons(0, -0x50));
    if (anim) {
      anim->Attach_To(this);
    }
    anim = base::NewPooled<AnimClass>(
        ANIM_BURN_MED, Sort_Y() + Coordinate::FromLeptons(0, -0xE0), 15);
    if (anim) {
      anim->Attach_To(this);
    }
    IsOnFire = true;
    return true;
  }
  return false;
}

/***********************************************************************************************
 * TerrainClass::Fire_Out -- Handles when fire has gone out. *
 *                                                                                             *
 *    When the fire has gone out on a burning terrain object, this routine is
 *called. The      * animation has already been terminated prior to calling this
 *routine. All this routine    * needs to perform is any necessary local flag
 *updating.                                   *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/27/1994 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::Fire_Out() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (IsOnFire) {
    IsOnFire = false;
    if (!IsCrumbling && !Strength) {
      Detach_All();
      Mark();
      Start_To_Crumble();
      base::NewPooled<AnimClass>(ANIM_SMOKE_M, Coord + Class->CenterBase);
    }
  }
}

/***********************************************************************************************
 * TerrainClass::AI -- Process the terrain object AI. *
 *                                                                                             *
 *    This is used to handle any AI processing necessary for terrain objects.
 *This might       * include animation effects. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/27/1994 JLB : Created. * 09/28/1994 JLB : Crumbling animation.
 ** 08/12/1996 JLB : Reset map zone when terrain object destroyed. * 10/04/1996
 *JLB : Growth speed regulated by rules. *
 *=============================================================================================*/
void TerrainClass::AI() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  ObjectClass::AI();

  if (*this == TERRAIN_MINE &&
      game::CurrentFrame() % (TheRules().GrowthRate * kTicksPerMinute) == 0) {
    TheMap().at(As_Target().AsCell()).Spread_Tiberium(true);
  }
  if (animation_.AI()) {
    Mark();

    /*
    **	If the terrain object is in the process of crumbling, then when at the
    **	last stage of the crumbling animation, delete the terrain object.
    */
    if (IsCrumbling &&
        animation_.stage() ==
            engine::gfx::KeyframeFrameCount(Class->Get_Image_Data()) - 1) {
      delete this;

      TheMap().Zone_Reset(kZoneFlagNormal | kZoneFlagCrusher |
                          kZoneFlagDestroyer);
    }
  }
}

/***********************************************************************************************
 * TerrainClass::Unlimbo -- Unlimbo terrain object onto the map. *
 *                                                                                             *
 *    This routine is used to unlimbo the terrain object onto a location on the
 *map. Normal    * unlimbo procedures are sufficient except that the coordinate
 *location of a terrain       * object is based on the upper left corner of a
 *cell rather than the center. Mask the      * coordinate value so that it snaps
 *to the upper left corner and then proceed with a       * normal unlimbo
 *process.                                                                  *
 *                                                                                             *
 * INPUT:   coord    -- The coordinate to mark as the terrain's location. *
 *                                                                                             *
 *          dir      -- unused *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain object successful in the unlimbo process?
 *Failure could be   * the result of illegal positioning. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/02/1994 JLB : Created. * 11/16/1994 JLB : Checks for theater
 *legality.                                             *
 *=============================================================================================*/
bool TerrainClass::Unlimbo(Coordinate coord, game::Direction dir) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (Class->Theater & base::Bit<uint32_t>(TheScenario().Theater)) {
    return ObjectClass::Unlimbo(coord, dir);
  }
  return false;
}

/***********************************************************************************************
 * TerrainClass::Start_To_Crumble -- Initiates crumbling of terrain (tree)
 *object.             *
 *                                                                                             *
 *    This routine is used to start the crumbling process for terrain object.
 *This only        * applies to trees. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::Start_To_Crumble() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (!IsCrumbling) {
    IsCrumbling = true;
    animation_.SetRate(2);
    animation_.set_stage(0);
  }
}

/***********************************************************************************************
 * TerrainClass::Limbo -- Handles terrain specific limbo action. *
 *                                                                                             *
 *    This routine (called as a part of the limbo process) will remove the
 *terrain occupation  * flag in the cell it occupies. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain object unlimboed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
bool TerrainClass::Limbo() {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  if (!IsInLimbo) {
    const Cell cell = Cell::Containing(Coord);
    TheMap().at(cell).Flag.Occupy.Monolith = false;
  }
  return ObjectClass::Limbo();
}

/***********************************************************************************************
 * TerrainClass::Center_Coord -- Fetches the center point coordinate for terrain
 *object.       *
 *                                                                                             *
 *    Use this routine to fetch the center point terrain *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/
Coordinate TerrainClass::Center_Coord() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  return Coord + Class->CenterBase;
}

/***********************************************************************************************
 * TerrainClass::Radar_Icon -- Fetches pointer to radar icon to use. *
 *                                                                                             *
 *    This routine will return with a pointer to the radar icon to use for the
 *cell number     * specified. *
 *                                                                                             *
 * INPUT:   cell  -- The cell number to use when determine what icon pointer to
 *return.        *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the 9 pixel values that make up the icon
 *of this         * terrain object located at the cell specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
std::span<const unsigned char> TerrainClass::Radar_Icon(Cell cell) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().terrain(), this);
  DCHECK(IsActive);

  const auto icons = Class->Get_Radar_Data();
  if (icons.size() < 2) {
    return {};
  }
  const int width = base::At(icons, 0);
  const int height = base::At(icons, 1);

  /*
  ** Icon number that we need can be found by converting the cell and base
  ** cell to and x and y offset from the upper left of the cell, and then
  ** multiplying it by the width of the terrain in icons, which we
  ** conveniently stored out as the first byte of every icon we made.
  */
  const Cell basecell = Cell::Containing(Coord);  // the base cell of terrain
  const int ydiff = static_cast<int16_t>(cell.y() - basecell.y());
  const int xdiff = static_cast<int16_t>(cell.x() - basecell.x());
  if (xdiff >= 0 && ydiff >= 0 && xdiff < width && ydiff < height) {
    const int iconnum = (ydiff * width) + xdiff;
    const auto start = (base::ToSize(iconnum) * 9) + 2;
    if (start <= icons.size() && icons.size() - start >= 9) {
      return icons.subspan(start, 9);
    }
  }
  return {};
}

/***********************************************************************************************
 * TerrainClass::Target_Coord -- Returns with the target coordinate. *
 *                                                                                             *
 *    This routine will return with the coordinate to use if this terrain object
 *were to be    * fired upon and the coordinate where the bullets should hit is
 *needed. Usually, this      * location is the base of the object (e.g., the
 *trunk of a tree).                          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate to use when firing at this object. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/07/1996 JLB : Created. *
 *=============================================================================================*/
Coordinate TerrainClass::Target_Coord() const {
  return Coordinate::FromLeptons(0, -Height) + Sort_Y();
}

/***********************************************************************************************
 * TerrainClass::Read_INI -- Reads terrain objects from INI file. *
 *                                                                                             *
 *    This routine reads a scenario control INI file and creates all * terrain
 *objects specified therein. Objects so created are placed * upon the map. *
 *                                                                                             *
 *      INI entry format: * cellnum = TypeName, Triggername *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the loaded scenario INI file data. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/24/1994 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::Read_INI(engine::file::IniFile& ini) {
  const int len = ini.EntryCount(INI_Name());

  for (int index = 0; index < len; index++) {
    const std::string_view entry = ini.EntryName(INI_Name(), index);
    const TerrainType terrain =
        GetTerrainType(ini, INI_Name(), entry, TERRAIN_NONE);
    const Cell cell = Cell::FromIndex(base::ParseIntegerOr<int16_t>(entry, 0));

    if (terrain != TERRAIN_NONE) {
      base::NewPooled<TerrainClass>(terrain, cell);
    }
  }
}

/***********************************************************************************************
 * TerrainClass::Write_INI -- Write all terrain objects to the INI database
 *specified.         *
 *                                                                                             *
 *    This routine will clear out any old terrain data from the INI database and
 *then          * fill it in with all the data from the terrain objects that
 *currently exists.             *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to store the terrain objects
 *in.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/30/1996 JLB : Created. *
 *=============================================================================================*/
void TerrainClass::Write_INI(engine::file::IniFile& ini) {
  /*
  **	First, clear out all existing terrain data from the ini file.
  */
  ini.Clear(INI_Name());

  /*
  **	Write the terrain data out.
  */
  for (int index = 0; index < TheObjectHeaps().terrain().Count(); index++) {
    TerrainClass* terrain = TheObjectHeaps().terrain().ActiveAt(index);
    if (terrain != nullptr && !terrain->IsInLimbo && terrain->IsActive) {
      char uname[10];
      absl::SNPrintF(uname, sizeof(uname), "%d",
                     Cell::Containing(terrain->Coord).index());
      PutTerrainType(ini, INI_Name(), uname, *terrain);
    }
  }
}

}  // namespace ra
