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

/* $Header: /CounterStrike/TERRAIN.H 1     3/03/97 10:25a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_TERRAIN_H_
#define CNC_RED_ALERT_RA_TERRAIN_H_

#include <cstddef>

#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/inline.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/stage.h"
#include "ra/techno.h"
#include "ra/type.h"

/****************************************************************************
**	Each type of terrain has certain pieces of static information associated
**	with it. This class elaborates this data.
*/
class TerrainClass final : public ObjectClass, public StageClass {
 public:
  /*
  **	This points to the constant terrain data (for this type) that gives this
  **	terrain object its character.
  */
  CCPtr<TerrainTypeClass> Class;

  /*
  **	Constructor for terrain object class.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  TerrainClass(TerrainType id, CELL cell);
  ~TerrainClass() override;
  TerrainClass(const TerrainClass&) = delete;
  TerrainClass& operator=(const TerrainClass&) = delete;
  TerrainClass(TerrainClass&&) = delete;
  TerrainClass& operator=(TerrainClass&&) = delete;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator TerrainType() const { return Class->Type; }

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
  COORDINATE Target_Coord() const override;

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
  void Draw_It(int x, int y, WindowNumberType window) const override;
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
                         TechnoClass* source, bool forced = false) override;

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
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "TERRAIN"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

 private:
  /*
  **	If this terrain object is on fire, then this flag will be true.
  */
  unsigned IsOnFire : 1 = false;

  /*
  **	Is this a terrain object that undergoes crumbling animation and it is
  **	in fact crumbling at this time?
  */
  unsigned IsCrumbling : 1 = false;

  // Shell for TFixedIHeapClass::Load; Serialize() supplies every value.
  TerrainClass() = default;
  friend class TFixedIHeapClass<TerrainClass>;
};

class ArchiveReader;
class ArchiveWriter;
extern template void TerrainClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TerrainClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_RA_TERRAIN_H_
