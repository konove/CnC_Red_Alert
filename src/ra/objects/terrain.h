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

#ifndef CNC_RED_ALERT_RA_OBJECTS_TERRAIN_H_
#define CNC_RED_ALERT_RA_OBJECTS_TERRAIN_H_

#include <cstddef>

#include "absl/base/attributes.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/sim/direction.h"
#include "ra/core/defines.h"
#include "ra/objects/object.h"
#include "ra/objects/stage.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

/****************************************************************************
**	Each type of terrain has certain pieces of static information associated
**	with it. This class elaborates this data.
*/
class TerrainClass final : public ObjectClass {
 public:
  /*
  **	This points to the constant terrain data (for this type) that gives this
  **	terrain object its character.
  */
  const TerrainTypeClass* Class = nullptr;

  /*
  **	Constructor for terrain object class.
  */
  void* operator new(std::size_t size) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  explicit TerrainClass(TerrainType type, Cell cell = Cell::None());
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
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  [[nodiscard]] Coordinate Center_Coord() const override;
  [[nodiscard]] Coordinate Render_Coord() const override { return Coord; }
  [[nodiscard]] Coordinate Sort_Y() const override {
    return Coord + Class->CenterBase;
  }
  [[nodiscard]] Coordinate Target_Coord() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(Coordinate coord,
               game::Direction dir = game::Direction::kNorth) override;
  bool Limbo() override;
  [[nodiscard]] game::MoveType Can_Enter_Cell(
      Cell cell, game::Facing facing = game::Facing::kNone) const override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void Draw_It(PixelView& view, int x, int y,
               WindowNumberType window) const override;
  bool Mark(MarkType mark = MARK_CHANGE) override;
  std::span<const unsigned char> Radar_Icon(Cell cell);

  /*
  **	User I/O.
  */
  void Clicked_As_Target(int /*unused*/) override {}

  /*
  **	Combat related.
  */
  void Fire_Out() override;
  bool Catch_Fire() override;
  game::ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                               TechnoClass* source,
                               bool forced = false) override;

  /*
  **	AI.
  */
  void AI() override;

  /*
  **	Scenario and debug support.
  */

  /*
  **	File I/O.
  */
  static void Read_INI(engine::file::IniFile& ini);
  static void Write_INI(engine::file::IniFile& ini);
  static const char* INI_Name() { return "TERRAIN"; }
  // The frame counter of the crumbling (and, for a blossom tree, growing)
  // animation.
  [[nodiscard]] StageClass& animation() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return animation_;
  }
  [[nodiscard]] const StageClass& animation() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return animation_;
  }

  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

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

  // Which frame is shown and when the next one is due.
  StageClass animation_;

  // Shell for ObjectHeap::Load; Serialize() supplies every value.
  TerrainClass() = default;
  friend class game::ObjectHeap<TerrainClass>;
};

extern template void TerrainClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TerrainClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_TERRAIN_H_
