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

/* $Header:   F:\projects\c&c\vcs\code\terrain.h_v   2.16   16 Oct 1995 16:47:48
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TERRAIN.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 29, 1994 *
 *                                                                                             *
 *                  Last Update : April 29, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_TERRAIN_H_
#define CNC_RED_ALERT_TD_TERRAIN_H_

#include <cstddef>

#include "td/defines.h"
#include "td/inline.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/stage.h"
#include "td/techno.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

/****************************************************************************
**	Each type of terrain has certain pieces of static information associated
**	with it. This class elaborates this data.
*/
class TerrainClass : public ObjectClass, public StageClass {
 public:
  const TerrainTypeClass* const Class;
  operator TerrainType() const { return Class->Type; }

  /*
  **	Constructor for terrain object class.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  TerrainClass();
  TerrainClass(TerrainType id, CELL cell);
  TerrainClass(const NoInitClass& x)
      : ObjectClass(x), Class(Class), StageClass(x) {}
  ~TerrainClass() override;
  RTTIType What_Am_I() const override { return RTTI_TERRAIN; }

  static void Init();

  /*
  **	Terrain specific support functions.
  */
  void Start_To_Crumble();

  /*
  **	Query functions.
  */
  const ObjectTypeClass& Class_Of() const override { return *Class; }

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Center_Coord() const override;
  COORDINATE Render_Coord() const override { return Coord; }
  COORDINATE Sort_Y() const override {
    return Coord_Add(Coord, Class->CenterBase);
  }
  COORDINATE Target_Coord() const override { return Sort_Y(); }

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE coord, DirType dir = DIR_N) override;
  bool Limbo() override;
  MoveType Can_Enter_Cell(CELL cell,
                          FacingType facing = FACING_NONE) const override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void Draw_It(int x, int y, WindowNumberType window) override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
  unsigned char* Radar_Icon(CELL cell);

  /*
  **	User I/O.
  */
  void Clicked_As_Target(int) override {}

  /*
  **	Combat related.
  */
  void Fire_Out() override;
  bool Catch_Fire() override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source) override;
  TARGET As_Target() const override;

  /*
  **	AI.
  */
  void AI() override;

  /*
   **	Scenario and debug support.
   */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static const char* INI_Name() { return "TERRAIN"; }
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

 private:
  /*
  **	If this terrain object is on fire, then this flag will be true.
  */
  unsigned IsOnFire : 1;

  /*
  **	Is this a terrain object that undergoes crumbling animation and it is
  **	in fact crumbling at this time?
  */
  unsigned IsCrumbling : 1;

  /*
  ** If this is a tree that becomes a blossom tree, is it currently doing so?
  */
  unsigned IsBlossoming : 1;

  /*
  ** If this is a blossom tree, is it barnacled?
  */
  unsigned IsBarnacled : 1;

  /*
  ** If this is a blossom tree that is barnacled, is it pulsing and spewing
  ** out spores?
  */
  unsigned IsSporing : 1;

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void* VTable;
};

#endif  // CNC_RED_ALERT_TD_TERRAIN_H_
