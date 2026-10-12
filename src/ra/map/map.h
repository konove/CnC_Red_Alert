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

/* $Header: /CounterStrike/MAP.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MAP.H *
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

#ifndef CNC_RED_ALERT_RA_MAP_MAP_H_
#define CNC_RED_ALERT_RA_MAP_MAP_H_

#include <cstdint>
#include <vector>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_source.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/map/gscreen.h"
#include "ra/objects/cell.h"
#include "ra/objects/crate.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class MapClass : public GScreenClass {
 public:
  // Saved-game state; defined in iomap.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  MapClass() = default;

  /*
  ** Initialization
  */
  void One_Time() override;    // Theater-specific inits
  void Init_Clear() override;  // Clears all to known state
  virtual void Alloc_Cells();  // Allocates buffers
  virtual void Free_Cells();   // Frees buffers
  virtual void Init_Cells();   // Frees buffers

  /*--------------------------------------------------------
  ** Main functions that deal with groupings of cells within the map or deals
  *with the cell
  ** as it relates to the map - not what the cell contains.
  */
  [[nodiscard]] static Cell Pick_Random_Location();
  [[nodiscard]] int Intact_Bridge_Count() const;
  bool Base_Region(Cell cell, HousesType& house, ZoneType& zone) const;
  [[nodiscard]] Cell Nearby_Location(Cell cell, SpeedType speed, int zone = -1,
                                     MZoneType check = MZONE_NORMAL) const;
  [[nodiscard]] ObjectClass* Close_Object(Coordinate coord) const;
  virtual void Detach(ObjectClass* /*unused*/) {}
  static int Cell_Region(Cell cell);
  static int64_t Cell_Threat(Cell cell, HousesType house);
  [[nodiscard]] bool In_Radar(Cell cell) const;
  void Sight_From(Cell cell, int sightrange, HouseClass* house,
                  bool incremental = false);
  static void Jam_From(Cell cell, int jamrange, HouseClass* house);
  static void UnJam_From(Cell cell, int jamrange, HouseClass* house);
  void Place_Down(Cell cell, ObjectClass* object);
  void Pick_Up(Cell cell, ObjectClass* object);
  void Overlap_Down(Cell cell, ObjectClass* object);
  void Overlap_Up(Cell cell, ObjectClass* object);
  bool Read_Binary(ByteSource& straw);
  bool Write_Binary(ByteSink& pipe);
  bool Place_Random_Crate();
  bool Remove_Crate(Cell cell);
  bool Zone_Reset(uint32_t method);
  bool Zone_Cell(Cell cell, int zone);
  int Zone_Span(Cell cell, int zone, MZoneType check);
  bool Destroy_Bridge_At(Cell cell);
  void Detach(Target target, bool all = true);
  void Shroud_The_Map();

  int32_t Overpass();

  virtual void Logic();
  virtual void Set_Map_Dimensions(int x, int y, int w, int h);

  /*
  ** Debug routine
  */
  bool Validate();

  /*
  **	This is the dimensions and position of the sub section of the global
  *map. *	It is this region that appears on the radar map and constrains
  *normal *	movement.
  */
  int MapCellX = 0;
  int MapCellY = 0;
  int MapCellWidth = 0;
  int MapCellHeight = 0;

  /*
  **	This is the total value of all harvestable Tiberium on the map.
  */
  int64_t TotalValue = 0;

  // Resolves a coordinate or cell ID through the checked cell array.
  CellClass& at(Coordinate coord) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(cells_, Cell::Containing(coord).index());
  }
  CellClass& operator[](Coordinate coord) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(coord);
  }
  CellClass& at(Cell cell) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(cells_, cell.index());
  }
  CellClass& operator[](Cell cell) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(cell);
  }
  [[nodiscard]] const CellClass& at(Coordinate coord) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(cells_, Cell::Containing(coord).index());
  }
  const CellClass& operator[](Coordinate coord) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(coord);
  }
  [[nodiscard]] const CellClass& at(Cell cell) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return base::At(cells_, cell.index());
  }
  const CellClass& operator[](Cell cell) const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return at(cell);
  }
  // Returns the number of the cell `cell` points to, which must be one of
  // this map's cells.
  [[nodiscard]] int ID(const CellClass* cell) const {
    return static_cast<int>(cell - cells_.data());
  }

 protected:
  // The map's cells, indexed by Cell::index().
  std::vector<CellClass> cells_;

  /*
  **	These are the size dimensions of the underlying array of cell objects.
  **	This is the dimensions of the "map" that the tactical view is
  **	restricted to.
  */
  int XSize = 0;
  int YSize = 0;
  int Size = 0;

  static const int RadiusCount[11];
  static const int RadiusOffset[];

  /*
  **	This specifies the information for the various crates in the game.
  */
  CrateClass Crates[256];

 private:
  friend class CellClass;

  /*
  **	Tiberium growth potential cells are recorded here.
  */
  Cell TiberiumGrowth[MAP_CELL_W / 2]{};
  int TiberiumGrowthCount = 0;
  int TiberiumGrowthExcess = 0;

  /*
  **	List of cells that are full enough strength that they could spread
  **	Tiberium to adjacent cells.
  */
  Cell TiberiumSpread[MAP_CELL_W / 2]{};
  int TiberiumSpreadCount = 0;
  int TiberiumSpreadExcess = 0;

  /*
  **	This is the current cell number in the incremental map scan process.
  */
  Cell TiberiumScan{};

  static constexpr int kScanAmount = MAP_CELL_TOTAL;
};

int Coord_Spillage_Number(Coordinate coord, int maxsize);

extern template void MapClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void MapClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_MAP_MAP_H_
