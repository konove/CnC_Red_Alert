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

class ArchiveReader;
class ArchiveWriter;

#include <cstddef>

#include "absl/base/attributes.h"
#include "td/defines.h"
#include "td/inline.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/stage.h"
#include "td/techno.h"
#include "td/type.h"

/****************************************************************************
**	Each type of terrain has certain pieces of static information associated
**	with it. This class elaborates this data.
*/
class TerrainClass final : public ObjectClass, public StageClass {
 public:
  const TerrainTypeClass* Class = nullptr;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator TerrainType() const { return Class->Type; }

  /*
  **	Constructor for terrain object class.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  TerrainClass() {
    IsActive = true;
    Strength = 0;
  }
  TerrainClass(TerrainType type, CELL cell);
  ~TerrainClass() override;
  TerrainClass(const TerrainClass&) = delete;
  TerrainClass& operator=(const TerrainClass&) = delete;
  TerrainClass(TerrainClass&&) = delete;
  TerrainClass& operator=(TerrainClass&&) = delete;
  [[nodiscard]] RTTIType What_Am_I() const override { return RTTI_TERRAIN; }

  static void Init();

  /*
  **	Terrain specific support functions.
  */
  void Start_To_Crumble();

  /*
  **	Query functions.
  */
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  [[nodiscard]] COORDINATE Center_Coord() const override;
  [[nodiscard]] COORDINATE Render_Coord() const override { return Coord; }
  [[nodiscard]] COORDINATE Sort_Y() const override {
    return Coord_Add(Coord, Class->CenterBase);
  }
  [[nodiscard]] COORDINATE Target_Coord() const override { return Sort_Y(); }

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE coord, DirType dir = DIR_N) override;
  bool Limbo() override;
  [[nodiscard]] MoveType Can_Enter_Cell(
      CELL cell, FacingType facing = FACING_NONE) const override;

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
  void Clicked_As_Target(int /*unused*/) override {}

  /*
  **	Combat related.
  */
  void Fire_Out() override;
  bool Catch_Fire() override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source) override;
  [[nodiscard]] TARGET As_Target() const override;

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
  // Field-wise saved-game support, defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Dee-buggin' support.
  */
  // debug self-check; callers run it for its assertions and ignore the count.
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  int Validate() const;

 private:
  /*
  **	If this terrain object is on fire, then this flag will be true.
  */
  bool IsOnFire : 1 = false;

  /*
  **	Is this a terrain object that undergoes crumbling animation and it is
  **	in fact crumbling at this time?
  */
  bool IsCrumbling : 1 = false;

  /*
  ** If this is a tree that becomes a blossom tree, is it currently doing so?
  */
  bool IsBlossoming : 1 = false;

  /*
  ** If this is a blossom tree, is it barnacled?
  */
  bool IsBarnacled : 1 = false;

  /*
  ** If this is a blossom tree that is barnacled, is it pulsing and spewing
  ** out spores?
  */
  bool IsSporing : 1 = false;
};

extern template void TerrainClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TerrainClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_TERRAIN_H_
