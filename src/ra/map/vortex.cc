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

/*************************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S **
 *************************************************************************************
 *                                                                                   *
 *                 Project Name : Command & Conquer - Red Alert *
 *                                                                                   *
 *                    File Name : VORTEX.CPP *
 *                                                                                   *
 *                   Programmer : Steve Tall *
 *                                                                                   *
 *                   Start Date : August 12th, 1996 *
 *                                                                                   *
 *                  Last Update : September 6th, 1996 [ST] *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Overview: *
 *                                                                                   *
 *  Circley vortexy swirly type thing. (Really just a pixel & color remap). *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Functions: *
 *                                                                                   *
 * CVC::ChronalVortexClass -- vortex class constructor *
 * CVC::~ChronalVortexClass -- vortex class destructor * CVC::Appear -- Makes a
 *chronal vortex appear at the given coordinate.             * CVC::Disappear --
 *Makes the chronal vortex go away.                               * CVC::Hide --
 *Makes the vortex hide. It might come back later.                     *
 * CVC::Show -- Makes a hidden vortex visible again. * CVC::Stop -- Stops the
 *vortex without going through the hide animation            * CVC::Load --
 *Loads the chronal vortex from a savegame file.                       *
 * CVC::Save -- Saves the vortex class data to a savegame file * CVC::AI -- AI
 *for the vortex. Includes movement and firing.                       *
 * CVC::Movement -- Movement AI for the vortex. * CVC::Set_Target -- Make the
 *vortex zap a particular object.                       * CVC::Attack -- look
 *for objects to attack                                         *
 * CVC::Zap_Target -- If the vortex has a target object then zap it with
 *lightning.  * CVC::Coordinate_Remap -- Draws the vortex * CVC::Render --
 *Renders the vortex at its current position.                        *
 * CVC::Set_Redraw -- Flags the cells under to vortex to redraw. *
 * CVC::Setup_Remap_Tables -- Initialises the color remap tables based on
 *theater.   * CVC::BuildShadeTable -- Builds a fading color lookup table. *
 *                                                                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - */

#include "ra/map/vortex.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/rect.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/archive.h"
#include "game/core/cell_size.h"
#include "game/core/game_clock.h"
#include "game/ui/standard_colors.h"
#include "ra/ai/scenario.h"
#include "ra/assets/audio.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/shape_draw.h"
#include "ra/assets/window_rect.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/random_pick.h"
#include "ra/core/target.h"
#include "ra/editor/mapedit.h"
#include "ra/map/display.h"
#include "ra/map/layer.h"
#include "ra/map/map.h"
#include "ra/map/mouse.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

namespace ra {

/*
** Instance of chronal vortex class. This must be the only instance.
*/

/***********************************************************************************************
 * CVC::ChronalVortexClass -- vortex class constructor *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:25PM ST : Created *
 *=============================================================================================*/
ChronalVortexClass::ChronalVortexClass()
    : RenderBuffer(game::kCellWidthPixels * 4, game::kCellHeightPixels * 4) {}

/***********************************************************************************************
 * CVC::~ChronalVortexClass -- vortex class destructor *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:25PM ST : Created *
 *=============================================================================================*/
// Declared rather than defaulted in the header only so the deleted copy and
// move operations have a destructor beside them.
ChronalVortexClass::~ChronalVortexClass() = default;

/***********************************************************************************************
 * CVC::Appear -- Makes a chronal vortex appear at the given coordinate. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Coordinate that vortex should appear at. *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: This member does nothing if the vortex is already active *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:27PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Appear(Coordinate coordinate) {
  if (Active) {
    return;
  }

  /*
  ** Adjust the given coordinate so the vortex appears in a central position
  */
  int x = Lepton_To_Pixel(coordinate.x());
  int y = Lepton_To_Pixel(coordinate.y());

  x -= 32;
  y -= 32;

  const int lx = Pixel_To_Lepton(x);
  const int ly = Pixel_To_Lepton(y);

  Position = Coordinate::FromLeptons(lx, ly);

  /*
  ** Initialise the vortex variables.
  */
  AnimateDir = 1;
  AnimateFrame = 0;
  State = STATE_GROW;
  Active = true;
  Animate = 0;
  StartShutdown = false;
  LastAttackFrame = static_cast<int>(game::CurrentFrame());
  TargetObject = Target{};
  ZapFrame = 0;
  Hidden = false;
  StartHiding = false;
  XDir = 0;
  YDir = 0;

  /*
  ** Vortex starts off in a random direction.
  */
  DesiredXDir = RandomPick(-Speed, Speed);
  DesiredYDir = RandomPick(-Speed, Speed);
}

/***********************************************************************************************
 * CVC::Disappear -- Makes the chronal vortex go away. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:30PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Disappear() {
  if (Hidden) {
    Active = false;
  } else {
    StartShutdown = true;
  }
}

/***********************************************************************************************
 * CVC::Hide -- Makes the vortex hide. It might come back later. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: This doesnt deactivate the vortex. Use Disappear to get rid of it
 *permanently.    *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:30PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Hide() {
  if (!StartShutdown) {
    StartHiding = true;
  }
}

/***********************************************************************************************
 * CVC::Show -- Makes a hidden vortex visible again. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:31PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Show() {
  /*
  ** Dont do anything if vortx is dying.
  */
  if (!StartShutdown) {
    /*
    ** If the vortex is hidden then show it again.
    */
    if (Hidden) {
      Hidden = false;
      StartHiding = false;
      AnimateFrame = 0;
      State = STATE_GROW;
      XDir = 0;
      YDir = 0;
    } else {
      /*
      ** If the vortex is in the process of hiding then reverse it.
      */
      StartHiding = false;
      if (State == STATE_SHRINK) {
        State = STATE_GROW;
        AnimateFrame = VORTEX_FRAMES - AnimateFrame;
      }
    }
  }
}

/***********************************************************************************************
 * CVC::Stop -- Stops the vortex without going through the hide animation *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:32PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Stop() {
  if (Active) {
    Active = false;
  }
}

template <class Archive>
void ChronalVortexClass::Serialize(Archive& ar) {
  bool active = Active != 0;
  bool shutdown = StartShutdown != 0;
  bool hiding = StartHiding != 0;
  bool hidden = Hidden != 0;
  ar(Position, AnimateDir, AnimateFrame, Animate, State, active, shutdown,
     hiding, hidden, LastAttackFrame, ZapFrame, TargetObject, TargetDistance,
     HiddenFrame, XDir, YDir, DesiredXDir, DesiredYDir, Range, Speed, Damage);
  if constexpr (Archive::kIsReading) {
    // These signed one-bit fields represent true as -1.
    Active = active ? -1 : 0;
    StartShutdown = shutdown ? -1 : 0;
    StartHiding = hiding ? -1 : 0;
    Hidden = hidden ? -1 : 0;
    // The page is scratch redrawn on the next render, but the saved game
    // says nothing about it, so start it blank rather than on the pixels
    // the outgoing game left there.
    RenderBuffer.view().Clear();
    if (ar.ok()) {
      Theater = THEATER_NONE;
      Setup_Remap_Tables(TheScenario().Theater);
    }
  }
}
template void ChronalVortexClass::Serialize(ArchiveWriter&);
template void ChronalVortexClass::Serialize(ArchiveReader&);

/***********************************************************************************************
 * CVC::AI -- AI for the vortex. Includes movement and firing. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:34PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::AI() {

  /*
  ** No AI if vortex isnt active
  */
  if (Active) {
    /*
    ** Do the movement AI
    */
    Movement();

    /*
    ** Do the attack AI
    */
    Zap_Target();

    if (Hidden && game::CurrentFrame() - HiddenFrame > 50) {
      /*
      ** Vortex is hidden. Chance of it showing itself increases the longer its
      *stays hidden.
      */
      const int chance = RandomPick(0, 2000);
      if (chance <= game::CurrentFrame() - HiddenFrame) {
        Show();
      }
    } else {
      if (Animate == 0) {
        /*
        ** Its time to animate the vortex.
        */
        AnimateFrame += AnimateDir;

        if (AnimateFrame > VORTEX_FRAMES) {
          /*
          ** State changes can only occur on final animation frames.
          */
          AnimateFrame = 1;

          if (StartShutdown) {
            /*
            ** Vortex is in the process of dying.
            */
            if (State == STATE_SHRINK) {
              Set_Redraw();
              Active = false;
              AnimateFrame = 0;
            } else {
              Attack();
              State = STATE_SHRINK;
            }
          } else {
            if (StartHiding) {
              /*
              ** Vortex wants to hide.
              */
              if (State == STATE_SHRINK) {
                /*
                ** Hide the vortex now.
                */
                Set_Redraw();
                StartHiding = false;
                Hidden = true;
                HiddenFrame = static_cast<int>(game::CurrentFrame());
                if (RandomPick(0, 4) == 4) {
                  Disappear();
                }
              } else {
                /*
                ** Start hiding the vortex.
                */
                Attack();
                State = STATE_SHRINK;
              }
            } else {
              Attack();
              if (State == STATE_GROW) {
                State = STATE_ROTATE;
              } else {
                // Attack();
              }
            }
          }
        } else {
          if (AnimateFrame == VORTEX_FRAMES / 2) {
            Attack();
          }
        }
      }
      Animate = (Animate + 1) % 2;
    }
  }
}

/***********************************************************************************************
 * CVC::Movement -- Movement AI for the vortex. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:39PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Movement() {
  bool newpick = true;

  /*
  ** Update the vortex position by applying the x and y direction variables
  */
  // Each component is a 16-bit lepton word: drifting past the world's left or
  // top edge wraps to a huge position, as it always has.
  const int x = static_cast<uint16_t>(Position.x() + XDir);
  const int y = static_cast<uint16_t>(Position.y() + YDir);

  Position = Coordinate::FromLeptons(x, y);

  /*
  ** Reverse the direction of the vortex if its drifting off the map.
  */
  if (x > game::kCellWidthLeptons *
              (TheMap().MapCellX + TheMap().MapCellWidth - 4)) {
    newpick = false;
    if (DesiredXDir > 0) {
      DesiredXDir = -DesiredXDir;
    }
  }

  if (y > game::kCellHeightLeptons *
              (TheMap().MapCellY + TheMap().MapCellHeight - 4)) {
    newpick = false;
    if (DesiredYDir > 0) {
      DesiredYDir = -DesiredYDir;
    }
  }

  if (x < (game::kCellWidthLeptons * TheMap().MapCellX) +
              (2 * game::kCellWidthLeptons)) {
    newpick = false;
    if (DesiredXDir < 0) {
      DesiredXDir = -DesiredXDir;
    }
  }

  if (y < (game::kCellHeightLeptons * TheMap().MapCellY) +
              (2 * game::kCellWidthLeptons)) {
    newpick = false;
    if (DesiredYDir < 0) {
      DesiredYDir = -DesiredYDir;
    }
  }

  /*
  ** Vortex direction tends towards the desired direction unless the vortex is
  *shutting down or *	appearing in which case the direction tends towards 0.
  */
  if (State == STATE_ROTATE || Hidden) {
    if (XDir < DesiredXDir) {
      XDir++;
    }
    if (XDir > DesiredXDir) {
      XDir--;
    }
    if (YDir < DesiredYDir) {
      YDir++;
    }
    if (YDir > DesiredYDir) {
      YDir--;
    }
  } else {
    if (XDir > 0) {
      XDir -= Speed / 8;
    }
    if (XDir < 0) {
      XDir += Speed / 8;
    }
    if (YDir > 0) {
      YDir -= Speed / 8;
    }
    if (YDir < 0) {
      YDir += Speed / 8;
    }
  }

  /*
  ** Occasionally change the direction of the vortex.
  */
  if (newpick && RandomPick(0, 100) == 100) {
    DesiredXDir = RandomPick(-Speed, Speed);
    DesiredYDir = RandomPick(-Speed, Speed);
  }
}

/***********************************************************************************************
 * CVC::Set_Target -- Make the vortex zap a particular object. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to object to zap *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:42PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Set_Target(ObjectClass* target) {
  if (Active) {
    ZapFrame = 0;
    TargetObject = Target{};
    if (target != nullptr) {
      TargetObject = target->As_Target();
    }
    LastAttackFrame = static_cast<int>(game::CurrentFrame());
    TargetDistance =
        target != nullptr ? Distance(target->Center_Coord(), Position) : 0;
  }
}

/***********************************************************************************************
 * CVC::Attack -- look for objects to attack *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:42PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Attack() {
  int distance = 0;
  // if(TargetObject) return;
  // if(!TargetObject) return;
  /*
  ** Calculate the position of the center of the vortex.
  */
  int x = Lepton_To_Pixel(Position.x());
  int y = Lepton_To_Pixel(Position.y());

  x += 32;
  y += 12;

  const int lx = Pixel_To_Lepton(x);
  const int ly = Pixel_To_Lepton(y);

  const Coordinate here = Coordinate::FromLeptons(lx, ly);

  /*
  ** Scan through the ground layer objects and see who we should attack
  */

  /*
  ** First scan - find any object directly above the vortex.
  */
  for (int i = 0; i < MouseClass::Layer.at(LAYER_GROUND).Count(); i++) {
    ObjectClass* obj = MouseClass::Layer.at(LAYER_GROUND).at(i);

    if (obj->Is_Techno() && obj->Strength > 0) {
      distance = Distance(obj->Center_Coord(), here);

      if (distance <= game::kCellWidthLeptons * 2) {
        Set_Target(obj);
        break;
      }
    }
  }

  /*
  ** If we found something to attack then just return
  */
  if (!TargetObject) {
    return;
  }

  /*
  ** Scan through all ground level objects.
  **
  ** Objects within range have a chance of being selected based on their
  *distance from the vortex.
  */

  int chance = RandomPick(0, 1000);
  if (chance > game::CurrentFrame() - LastAttackFrame) {
    return;
  }

  for (int i = 0; i < MouseClass::Layer.at(LAYER_GROUND).Count(); i++) {
    ObjectClass* obj = MouseClass::Layer.at(LAYER_GROUND).at(i);

    if (obj && obj->Is_Techno()) {
      distance = Distance(obj->Center_Coord(), Position);

      if (distance < game::kCellWidthLeptons * Range) {
        chance = RandomPick(0, distance);
        if (chance < game::kCellWidthLeptons) {
          Set_Target(obj);
          break;
        }
      }
    }
  }
}

/***********************************************************************************************
 * CVC::Zap_Target -- If the vortex has a target object then zap it with
 *lightning.            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:45PM ST : Created *
 *=============================================================================================*/
#define ZAP_COUNT 1
void ChronalVortexClass::Zap_Target() {
  if (!Hidden && TargetObject && ZapFrame < ZAP_COUNT) {
    /*
    ** Get the center of the vortex.
    */
    int x = Lepton_To_Pixel(Position.x());
    int y = Lepton_To_Pixel(Position.y());

    x += 32;
    y += 12;

    const int lx = Pixel_To_Lepton(x);
    const int ly = Pixel_To_Lepton(y);

    const Coordinate here = Coordinate::FromLeptons(lx, ly);

    /*
    ** Create a temporary techno object se we can access the lightning ability
    *of the tesla.
    */
    TechnoClass* temptech =
        base::NewPooled<BuildingClass>(STRUCT_TESLA, HOUSE_GOOD);
    if (temptech != nullptr) {
      temptech->Coord = here;
      ObjectClass* obj = TargetObject.AsObject();
      const Target target = Target::FromCoordinate(obj->Center_Coord());
      PlaySoundEffectAt(VOC_TESLA_ZAP, obj->Center_Coord());
      temptech->Electric_Zap(target, 0, here, LightningRemap);
      base::DeletePooled(temptech);

      /*
      ** Flag the whole map to redraw to cover the lightning.
      */
      TheMap().Flag_To_Redraw(true);

      /*
      ** Zap the target 3 times but only do damage on the last
      * frame.
      */
      ZapFrame++;

      if (ZapFrame == ZAP_COUNT) {
        ZapFrame = 0;
        int damage = Damage;
        obj->Take_Damage(damage, TargetDistance, WARHEAD_TESLA, nullptr, true);
        TargetObject = Target{};
      }
    }

    /*
    ** Vortex might pretend to go away after zapping the target.
    */
    if (RandomPick(0, 2) == 2) {
      Hide();
    }
  }
}

/***********************************************************************************************
 * CVC::Coordinate_Remap -- Draws the vortex *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to view port to draw the vortex into * x offset * y offset *
 *           width of vortex * height of vortex * ptr to shading remap tables *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:48PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Coordinate_Remap(
    PixelView* inbuffer, int x, int y, int width, int height,
    std::span<const uint8_t> remap_table) {
  if (width <= 0 || height <= 0 || x < 0 || y < 0) {
    return;
  }
  const auto count = base::ToSize(width) * base::ToSize(height);
  if (count > remap_table.size() / 3 || !inbuffer->Lock()) {
    return;
  }
  std::vector<uint8_t> destbuf(count);
  const std::span<uint8_t> output(destbuf);
  const auto input = inbuffer->pixels();
  const auto stride =
      base::ToSize(inbuffer->pitch() + inbuffer->x_add() + inbuffer->width());
  const auto origin = base::ToSize(x) + (base::ToSize(y) * stride);
  for (size_t i = 0; i < count; ++i) {
    const auto getx = base::At(remap_table, i * 3);
    const auto gety = base::At(remap_table, (i * 3) + 1);
    const auto remap_color = base::At(remap_table, (i * 3) + 2);
    const auto source = origin + getx + (gety * stride);
    if (source >= input.size() || remap_color >= std::size(VortexRemapTables)) {
      inbuffer->Unlock();
      return;
    }
    base::At(output, i) = base::At(base::At(VortexRemapTables, remap_color),
                                   base::At(input, source));
  }
  inbuffer->CopyFromBufferLocked(base::Rect{x, y, width, height}, output);
  inbuffer->Unlock();
}

/***********************************************************************************************
 * CVC::Render -- Renders the vortex at its current position. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:49PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Render(PixelView& view) {
  if (Active && !Hidden) {
    char fname[80];

    int frame = 0;

    /*
    ** Calculate which coordinate lookup table we should be using for this
    *frame.
    */
    switch (State) {
      case STATE_GROW:
        frame = 0;
        break;

      case STATE_ROTATE:
        frame = VORTEX_FRAMES;
        break;

      case STATE_SHRINK:
        frame = VORTEX_FRAMES * 2;
        break;
      default:
        break;
    }

    frame += AnimateFrame;

    absl::SNPrintF(fname, sizeof(fname), "HOLE%04d.lut", frame);

    const auto lut_ptr = MixArchive::RetrieveData(fname);
    if (!lut_ptr.empty()) {
      /*
      ** Build a representation of the area of the screen where the vortex will
      *be
      ** in an off-screen buffer.
      ** This is necessary for clipping as we cant remap pixels from off screen
      *if we build
      ** the image from the hidpage.
      */
      const auto xc = static_cast<int16_t>(Position.cell_x());
      const auto yc = static_cast<int16_t>(Position.cell_y());
      const TemplateTypeClass* ttype = nullptr;
      int icon = 0;  // The icon number to use from the template set.

      /*
      ** Temporarily modify the tactical window so it works with our offscreen
      *buffer
      */
      const int wx =
          base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowX);
      const int wy =
          base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowY);
      const int ww =
          base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowWidth);
      const int wh = base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)],
                              kWindowHeight);

      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowX) = 0;
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowY) = 0;
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowWidth) =
          RenderBuffer.width();
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowHeight) =
          RenderBuffer.height();

      /*
      ** Loop through all the cells that the vortex overlaps and render the
      *template, smudge
      ** and overlay for each cell.
      */
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          const Cell cell = Cell::FromXY(xc + x, yc + y);
          if (cell != Cell::None()) {
            // cellptr = &Map[ cell.Center().Whole() ];
            CellClass* cellptr = &TheMap().at(cell);

            /*
            **	Fetch a pointer to the template type associated with this cell.
            */
            if (cellptr->TType != TEMPLATE_NONE &&
                cellptr->TType != TEMPLATE_CLEAR1 &&
                // The original 8-bit template field used 255 as a sentinel.
                cellptr->TType != static_cast<TemplateType>(255)) {
              ttype = &TemplateTypeClass::As_Reference(cellptr->TType);
              icon = cellptr->TIcon;
            } else {
              ttype = &TemplateTypeClass::As_Reference(TEMPLATE_CLEAR1);
              icon = cellptr->Clear_Icon();
            }

            /*
            ** Draw the template
            */
            if (!ttype->Get_Image_Data().empty()) {
              RenderBuffer.view().DrawStamp(
                  ttype->Get_Image_Data(), icon, x * game::kCellWidthPixels,
                  y * game::kCellHeightPixels, {}, WindowRect(WINDOW_MAIN));
            }

            /*
            **	Draw any smudge.
            */
            if (cellptr->Smudge != SMUDGE_NONE) {
              SmudgeTypeClass::As_Reference(cellptr->Smudge)
                  .Draw_It(RenderBuffer.view(), x * game::kCellWidthPixels,
                           y * game::kCellHeightPixels, cellptr->SmudgeData);
            }

            /*
            **	Draw the overlay object.
            */
            if (cellptr->Overlay != OVERLAY_NONE) {
              const OverlayTypeClass& otype =
                  OverlayTypeClass::As_Reference(cellptr->Overlay);
              CC_Draw_Shape(
                  RenderBuffer.view(), otype.Get_Image_Data(),
                  cellptr->OverlayData,
                  (x * game::kCellWidthPixels) + (game::kCellWidthPixels / 2),
                  (y * game::kCellHeightPixels) + (game::kCellHeightPixels / 2),
                  WINDOW_TACTICAL,
                  engine::gfx::ShapeFlags::kCentered |
                      engine::gfx::ShapeFlags::kGhost,
                  {}, DisplayClass::UnitShadow);
            }
          }
        }
      }

      /*
      ** Restore the tactical window to its correct value
      */
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowX) = wx;
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowY) = wy;
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowWidth) =
          ww;
      base::At(WindowList[static_cast<int>(WINDOW_TACTICAL)], kWindowHeight) =
          wh;

      /*
      ** Render the vortex over the cells we just rendered to our buffer
      */
      Coordinate_Remap(&RenderBuffer.view(),
                       Lepton_To_Pixel(Position.Fraction().x()),
                       Lepton_To_Pixel(Position.Fraction().y()), 64, 64,
                       base::UnsignedBytes(lut_ptr));

      /*
      ** Calculate the pixel position of our fresh block of cells on the
      *tactical map so *	we can blit it to the hid page.
      */
      // Position.Whole();
      const Coordinate render_pos = Coordinate::FromLeptons(
          xc * game::kCellWidthLeptons, yc * game::kCellHeightLeptons);

      const int xtac =
          Pixel_To_Lepton(Lepton_To_Pixel(TheMap().TacticalCoord.x()));
      int xoff = Pixel_To_Lepton(Lepton_To_Pixel(render_pos.x()));
      xoff -= xtac;

      const int ytac =
          Pixel_To_Lepton(Lepton_To_Pixel(TheMap().TacticalCoord.y()));
      int yoff = Pixel_To_Lepton(Lepton_To_Pixel(render_pos.y()));
      yoff -= ytac;

      /*
      ** Create a view port to blit to
      */
      PixelView target(view.buffer(), 0, 16 + view.y_pos(),
                       Lepton_To_Pixel(TheMap().TacLeptonWidth),
                       Lepton_To_Pixel(TheMap().TacLeptonHeight));

      // Blit the freshly drawn cells and vortex into place on the hid page.
      // The whole of RenderBuffer goes across; Blit clips it to the tactical
      // view and advances the source by whatever came off the top and left.
      RenderBuffer.view().BlitTo(
          target, base::Rect{0, 0, RenderBuffer.width(), RenderBuffer.height()},
          Lepton_To_Pixel(xoff), Lepton_To_Pixel(yoff));
    }
  }
}

/***********************************************************************************************
 * CVC::Set_Redraw -- Flags the cells under to vortex to redraw. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:50PM ST : Created *
 *=============================================================================================*/
// Not const: flags the cells under the vortex for redraw.
// NOLINTNEXTLINE(readability-make-member-function-const)
void ChronalVortexClass::Set_Redraw() {
  if (Active) {
    const auto xc = static_cast<int16_t>(Position.cell_x());
    const auto yc = static_cast<int16_t>(Position.cell_y());

    for (int y = std::max(0, yc - 1); y < yc + 4; y++) {
      for (int x = std::max(0, xc - 1); x < xc + 4; x++) {
        const Cell cell = Cell::FromXY(x, y);
        if (cell != Cell::None()) {
          TheMap().at(cell).Redraw_Objects();
        }
      }
    }
  }
}

/***********************************************************************************************
 * CVC::Setup_Remap_Tables -- Initialises the color remap tables based on
 *theater.             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Theater *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/29/96 4:51PM ST : Created *
 *=============================================================================================*/
void ChronalVortexClass::Setup_Remap_Tables(TheaterType theater) {
  /*
  ** The names of the remap files for each theater
  */
  static char _remaps[3][13] = {"TEMP_VTX.PAL", "SNOW_VTX.PAL", "INTR_VTX.PAL"};

  /*
  ** If the theater has changed then load the remap tables from disk if they
  *exist or create
  ** them if they dont.
  */
  if (theater != Theater) {
    Theater = theater;

    const std::string_view remap_name =
        base::At(_remaps, static_cast<int>(Theater));

    if (const auto file = OpenGameFile(remap_name)) {
      file->ReadObject(VortexRemapTables);
    } else {
      for (int i = 0; i < MAX_REMAP_SHADES; i++) {
        base::At(VortexRemapTables, i) = BuildShadeTable(
            ThePalettes().game_palette(), game::kTransparentBlack,
            240 - (i * 256 / MAX_REMAP_SHADES));
      }

      if (const auto out = OpenGameFile(remap_name, FileAccess::kWrite)) {
        out->WriteObject(VortexRemapTables);
      }
    }
  }

  /*
  ** Set up the remap table for the lightning
  */
  for (int i = 0; i < 256; i++) {
    base::At(LightningRemap, i) = static_cast<unsigned char>(i);
  }
  LightningRemap[192] = 208;
  LightningRemap[193] = 209;
  LightningRemap[194] = 210;
  LightningRemap[195] = 211;
  LightningRemap[196] = 212;
  LightningRemap[197] = 213;
  LightningRemap[198] = 214;
  LightningRemap[199] = 215;
}

FadingTable ChronalVortexClass::BuildShadeTable(
    const PaletteClass& palette, const engine::gfx::PaletteIndex color,
    int frac) {
  const auto keeps_own_color = [](int index) {
    return index == 0 ||
           (index >= kCycleColorStart &&
            index < kCycleColorStart + kCycleColorCount) ||
           index == kPulseColor || index == kEmberColor;
  };
  FadingTable table{};
  for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
    if (keeps_own_color(index)) {
      base::At(table, index) = static_cast<uint8_t>(index);
      continue;
    }
    const engine::gfx::PaletteColor faded =
        palette.at(index).FadedToward(frac, palette.at(color));
    // Ties go to the lower color.
    int best = -1;
    int best_distance = 0;
    for (int candidate = 0; candidate < PaletteClass::COLOR_COUNT;
         candidate++) {
      if (keeps_own_color(candidate)) {
        continue;
      }
      const int distance = palette.at(candidate).SquaredDistance(faded);
      if (best == -1 || distance < best_distance) {
        best = candidate;
        best_distance = distance;
      }
    }
    base::At(table, index) = static_cast<uint8_t>(best);
  }
  return table;
}

void ChronalVortexClass::Detach(Target target) {
  if (target && target == TargetObject) {
    Set_Target(nullptr);
  }
}

}  // namespace ra
