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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer - Red Alert *
 *                                                                                             *
 *                    File Name : VORTEX.H *
 *                                                                                             *
 *                   Programmer : Steve Tall *
 *                                                                                             *
 *                   Start Date : 8/12/96 *
 *                                                                                             *
 *                  Last Update : August 29th, 1996 [ST] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *  Overview: * Definition of ChronalVortexClass. The Chronal vortex sometimes
 *appears when the          * chronosphere is used. *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_VORTEX_H_
#define CNC_RED_ALERT_RA_VORTEX_H_

#define MAX_REMAP_SHADES \
  16  // Number of lookup tables required for vortex shading.
#define VORTEX_FRAMES \
  16  // Number of frames in one complete rotation of the vortex.
#include "ra/defines.h"
#include "ra/object.h"
#include "ra/palette.h"
#include "sdllib/gbuffer.h"

class ChronalVortexClass {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  ** Constructor and destructor.
  */
  ChronalVortexClass();
  ~ChronalVortexClass();
  ChronalVortexClass(const ChronalVortexClass&) = delete;
  ChronalVortexClass& operator=(const ChronalVortexClass&) = delete;
  ChronalVortexClass(ChronalVortexClass&&) = delete;
  ChronalVortexClass& operator=(ChronalVortexClass&&) = delete;

  void Detach(TARGET target);

  /*
  ** Makes the vortex appear at the specified coordinate.
  */
  void Appear(COORDINATE coordinate);

  /*
  ** Makes the vortex go away.
  */
  void Disappear();

  /*
  ** Call this every frame.
  */
  void AI();

  /*
  ** Render the vortex
  */
  void Render();

  /*
  ** Flags cells under the vortex to be redrawn
  */
  void Set_Redraw();

  /*
  ** Call whenever the theater changes to recalculate the shading lookup tables
  */
  void Setup_Remap_Tables(TheaterType theater);

  /*
  ** Functions to load and save the vortex.
  */

  /*
  ** Returns true of vortex is currently active.
  */
  [[nodiscard]] bool Is_Active() const { return Active; }

  /*
  ** Makes the vortex attack the specified target. Target must be in range of
  *the vortex.
  */
  void Set_Target(ObjectClass* target);

  /*
  ** Disables the vortex.
  */
  void Stop();

  /*
  ** Members to allow read access to private data
  */
  [[nodiscard]] int Get_Range() const { return Range; }
  [[nodiscard]] int Get_Speed() const { return Speed; }
  [[nodiscard]] int Get_Damage() const { return Damage; }

  /*
  ** Members to allow write access to private data.
  */
  void Set_Range(int range) { Range = range; }
  void Set_Speed(int speed) { Speed = speed; }
  void Set_Damage(int damage) { Damage = damage; }

  /*
  ** Possible states the vortex can be in.
  */
  typedef enum AnimStateType {
    STATE_GROW,    // Vortex has just appeared and is growing larger
    STATE_ROTATE,  // Vortex is rotating
    STATE_SHRINK   // Vortex is shrinking and about to disappear
  } AnimStateType;

 private:
  /*
  ** Members for setting up the lookup tables.
  */
  static void Build_Fading_Table(const PaletteClass& palette, void* dest,
                                 int color, int frac);
  void Coordinate_Remap(GraphicViewPortClass* inbuffer, int x, int y, int width,
                        int height, const unsigned char* remap_table);

  /*
  ** Misc internal functions
  */
  void Attack();
  void Zap_Target();
  void Movement();
  void Hide();
  void Show();

  /*
  ** Position of the top left of the vortex
  */
  COORDINATE Position = 0;

  /*
  ** Direction of rotation
  */
  int AnimateDir = 1;

  /*
  ** Current frame of animation
  */
  int AnimateFrame = 0;

  /*
  ** Animation flag. When 0 vortex will animate 1 frame.
  */
  int Animate = 0;

  /*
  ** State of vortex. See ENUM for info.
  */
  AnimStateType State = STATE_GROW;

  /*
  ** Color lookup tables for shading on vortex.
  */
  unsigned char VortexRemapTables[MAX_REMAP_SHADES][256]{};

  /*
  ** Color lookup table to make the blue lightning orange.
  */
  unsigned char LightningRemap[256]{};

  /*
  ** Is vortex currently active?
  */
  int Active : 1 {0};

  /*
  ** Is the vortex winding down?
  */
  int StartShutdown : 1 = 0;

  /*
  ** Is the vortex about to hide from view?
  */
  int StartHiding : 1 = 0;

  /*
  ** Is the vortex active but hidden?
  */
  int Hidden : 1 = 0;

  /*
  ** Theater that lookup table is good for.
  */
  TheaterType Theater{THEATER_NONE};

  /*
  ** Last frame that vortex attacked on
  */
  int LastAttackFrame = 0;

  /*
  ** How many times lightning has zapped on this attack
  */
  int ZapFrame = 0;

  /*
  ** Ptr to object that the vortex is zapping
  */
  TARGET TargetObject = kTargetNone;
  //		ObjectClass		*TargetObject;

  /*
  ** Distance to the target object
  */
  int TargetDistance = 0;

  /*
  ** Game frame that vortex hid on.
  */
  int HiddenFrame = 0;

  /*
  ** Direction vortex is going in.
  */
  int XDir = 0;
  int YDir = 0;

  /*
  ** Direction vortex should be going in
  */
  int DesiredXDir = 0;
  int DesiredYDir = 0;

  /*
  ** Range in cells of the vortex lightning
  */
  int Range{10};

  /*
  ** Max speed in leptons per frame of the vortex.
  */
  int Speed{10};

  /*
  ** Damge of vortex lightning zap.
  */
  int Damage{200};

  /*
  ** Offscreen buffer to render vortex into. This is needed so we can handle
  *clipping.
  */
  GraphicBufferClass* RenderBuffer{
      nullptr};  // We havn't allocated it yet. It will be allocated as needed.
};

class ArchiveReader;
class ArchiveWriter;
extern template void ChronalVortexClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void ChronalVortexClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_VORTEX_H_
