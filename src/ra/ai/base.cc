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

#include "ra/ai/base.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "base/types.h"
#include "engine/stream/archive.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/data/ccini.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/type.h"

namespace ra {

bool BaseNodeClass::operator==(const BaseNodeClass& node) const {
  return Type == node.Type && Cell == node.Cell;
}

bool BaseNodeClass::operator!=(const BaseNodeClass& node) const {
  return !(*this == node);
}

bool BaseNodeClass::operator>(const BaseNodeClass& /*unused*/) { return true; }

BaseClass::~BaseClass() { Nodes.clear(); }

bool BaseClass::Is_Built(int index) const {
  return Get_Building(index) != nullptr;
}

BuildingClass* BaseClass::Get_Building(int index) const {
  const auto& node = base::At(Nodes, index);
  const Cell target_cell = node.Cell;

  const auto& map_cell = TheMap().at(target_cell);

  // Returns `candidate` as a building if it is the one this node asks for,
  // nullptr otherwise.
  const auto check_candidate = [&](ObjectClass* candidate) -> BuildingClass* {
    if (!candidate) {
      return nullptr;
    }

    if (candidate->What_Am_I() != RTTI_BUILDING) {
      return nullptr;
    }

    auto* building = dynamic_cast<BuildingClass*>(candidate);

    if (building->Class->Type != node.Type) {
      return nullptr;
    }

    // A building of this type that stands elsewhere and merely covers this
    // cell does not fill the node.
    if (Cell::Containing(building->Coord) != target_cell) {
      return nullptr;
    }

    return building;
  };

  // A building occupies the cells of its foundation and only overlaps the
  // others its image covers. Its top-left cell can be either, so look in both
  // lists.
  if (auto* match = check_candidate(map_cell.Cell_Building())) {
    return match;
  }

  for (auto* overlapper : map_cell.Overlappers) {
    if (auto* match = check_candidate(overlapper)) {
      return match;
    }
  }

  return nullptr;
}

bool BaseClass::Is_Node(const BuildingClass* obj) {
  return Get_Node(obj) != nullptr;
}

BaseNodeClass* BaseClass::Get_Node(const BuildingClass* obj) {
  for (int i = 0; i < std::ssize(Nodes); i++) {
    if (obj->Class->Type == base::At(Nodes, i).Type &&
        Cell::Containing(obj->Coord) == base::At(Nodes, i).Cell) {
      return &base::At(Nodes, i);
    }
  }
  return nullptr;
}

BaseNodeClass* BaseClass::Get_Node(Cell cell) {
  for (int index = 0; index < std::ssize(Nodes); index++) {
    if (cell == base::At(Nodes, index).Cell) {
      return &base::At(Nodes, index);
    }
  }
  return nullptr;
}

BaseNodeClass* BaseClass::Next_Buildable(StructType type) {
  for (int i = 0; i < std::ssize(Nodes); i++) {
    // STRUCT_NONE asks for the first hole of any type.
    if (type == STRUCT_NONE) {
      if (!Is_Built(i)) {
        return &base::At(Nodes, i);
      }

    } else {
      if (base::At(Nodes, i).Type == type && !Is_Built(i)) {
        return &base::At(Nodes, i);
      }
    }
  }

  // No hole: the caller picks a location itself (see
  // BuildingClass::Exit_Object).
  return nullptr;
}

void BaseClass::Read_INI(CCINIClass& ini) {
  char buf[128];
  char uname[10];
  BaseNodeClass node;  // node to add to list

  // The scenario names the owner under "Player"; the human player's house is
  // only the fallback for a section that omits it.
  House = ini.Get_HousesType(INI_Name(), "Player", ThePlayer()->Class->House);

  const int count = ini.Get_Int(INI_Name(), "Count", 0);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < count; i++) {
    absl::SNPrintF(uname, sizeof(uname), "%03d", i);
    ini.Get_String(INI_Name(), uname, nullptr, buf, sizeof(buf));

    base::Tokenizer tokens(buf, ",");
    node.Type = BuildingTypeClass::From_Name(tokens.Next());

    node.Cell =
        Cell::FromIndex(base::ParseIntegerOr<int16_t>(tokens.Next(), -1));

    // A missing or malformed entry would leave a node no building can fill:
    // Get_Building() indexes the map with its cell, and a save holding its
    // STRUCT_NONE type is rejected when loaded.
    if (node.Type == STRUCT_NONE || node.Cell.index() < 0 ||
        node.Cell.index() >= MAP_CELL_TOTAL) {
      continue;
    }

    Nodes.push_back(node);
  }
}

void BaseClass::Write_INI(CCINIClass& ini) {
  ini.Clear(INI_Name());

  if (House != HOUSE_NONE) {
    ini.Put_HousesType(INI_Name(), "Player", House);

    // Read_INI() finds the entries by counting from "000" up to Count.
    ini.Put_Int(INI_Name(), "Count", static_cast<int>(std::ssize(Nodes)));

    for (base::ssize i = 0; i < std::ssize(Nodes); i++) {
      char buf[128];
      char uname[10];

      absl::SNPrintF(uname, sizeof(uname), "%03td", i);

      absl::SNPrintF(
          buf, sizeof(buf), "%s,%d",
          BuildingTypeClass::As_Reference(base::At(Nodes, i).Type).IniName,
          base::At(Nodes, i).Cell.index());

      ini.Put_String(INI_Name(), uname, buf);
    }
  }
}

template <class Archive>
void BaseClass::Serialize(Archive& ar) {
  ar(House);
  auto count = static_cast<int32_t>(std::ssize(Nodes));
  ar(count);
  if constexpr (Archive::kIsReading) {
    // One node per cell is a sanity bound against a corrupt save, not a limit
    // the game enforces.
    if (!ar.ok() || count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid base node count");
      return;
    }
    Nodes.clear();
    Nodes.reserve(base::ToSize(count));
    for (int i = 0; i < count; ++i) {
      BaseNodeClass node;
      ar(node.Type, node.Cell);
      if (!ar.ok() || !magic_enum::enum_contains(node.Type) ||
          node.Cell.index() < 0 || node.Cell.index() >= MAP_CELL_TOTAL) {
        ar.Fail("invalid base node");
        return;
      }
      Nodes.push_back(node);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      ar(base::At(Nodes, i).Type, base::At(Nodes, i).Cell);
    }
  }
}
template void BaseClass::Serialize(ArchiveWriter&);
template void BaseClass::Serialize(ArchiveReader&);

}  // namespace ra
