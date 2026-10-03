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
#include <iterator>
#include <string>

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

bool BaseNodeClass::operator==(const BaseNodeClass& other) const {
  return type == other.type && cell == other.cell;
}

bool BaseNodeClass::operator!=(const BaseNodeClass& node) const {
  return !(*this == node);
}

bool BaseNodeClass::operator>(const BaseNodeClass& /*unused*/) { return true; }

BaseClass::~BaseClass() { nodes_.clear(); }

bool BaseClass::IsBuilt(int node_index) const {
  return FindBuilding(node_index) != nullptr;
}

BuildingClass* BaseClass::FindBuilding(int node_index) const {
  const auto& node = base::At(nodes_, node_index);
  const Cell target_cell = node.cell;

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

    if (building->Class->Type != node.type) {
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

bool BaseClass::HasNodeFor(const BuildingClass* building) {
  return FindNode(building) != nullptr;
}

BaseNodeClass* BaseClass::FindNode(const BuildingClass* building) {
  for (int i = 0; i < std::ssize(nodes_); i++) {
    if (building->Class->Type == base::At(nodes_, i).type &&
        Cell::Containing(building->Coord) == base::At(nodes_, i).cell) {
      return &base::At(nodes_, i);
    }
  }
  return nullptr;
}

BaseNodeClass* BaseClass::Get_Node(Cell cell) {
  for (int index = 0; index < std::ssize(nodes_); index++) {
    if (cell == base::At(nodes_, index).cell) {
      return &base::At(nodes_, index);
    }
  }
  return nullptr;
}

BaseNodeClass* BaseClass::FirstUnbuiltNode(StructType type) {
  for (int i = 0; i < std::ssize(nodes_); i++) {
    // STRUCT_NONE asks for the first hole of any type.
    if (type == STRUCT_NONE) {
      if (!IsBuilt(i)) {
        return &base::At(nodes_, i);
      }

    } else {
      if (base::At(nodes_, i).type == type && !IsBuilt(i)) {
        return &base::At(nodes_, i);
      }
    }
  }

  // No hole: the caller picks a location itself (see
  // BuildingClass::Exit_Object).
  return nullptr;
}

void BaseClass::ReadIni(CCINIClass& ini) {
  char value[128];
  BaseNodeClass node;  // node to add to list

  // The scenario names the owner under "Player"; the human player's house is
  // only the fallback for a section that omits it.
  house_ = ini.Get_HousesType(INI_Name(), "Player", ThePlayer()->Class->House);

  const int node_count = ini.Get_Int(INI_Name(), "Count", 0);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < node_count; i++) {
    const std::string entry = absl::StrFormat("%03d", i);
    ini.Get_String(INI_Name(), entry.c_str(), nullptr, value, sizeof(value));

    base::Tokenizer tokens(value, ",");
    node.type = BuildingTypeClass::From_Name(tokens.Next());

    node.cell =
        Cell::FromIndex(base::ParseIntegerOr<int16_t>(tokens.Next(), -1));

    // A missing or malformed entry would leave a node no building can fill:
    // FindBuilding() indexes the map with its cell, and a save holding its
    // STRUCT_NONE type is rejected when loaded.
    if (node.type == STRUCT_NONE || node.cell.index() < 0 ||
        node.cell.index() >= MAP_CELL_TOTAL) {
      continue;
    }

    nodes_.push_back(node);
  }
}

void BaseClass::WriteIni(CCINIClass& ini) {
  ini.Clear(INI_Name());

  if (house_ != HOUSE_NONE) {
    ini.Put_HousesType(INI_Name(), "Player", house_);

    // ReadIni() finds the entries by counting from "000" up to Count.
    ini.Put_Int(INI_Name(), "Count", static_cast<int>(std::ssize(nodes_)));

    for (base::ssize i = 0; i < std::ssize(nodes_); i++) {
      const std::string entry = absl::StrFormat("%03td", i);
      const std::string value = absl::StrFormat(
          "%s,%d",
          BuildingTypeClass::As_Reference(base::At(nodes_, i).type).IniName,
          base::At(nodes_, i).cell.index());

      ini.Put_String(INI_Name(), entry.c_str(), value.c_str());
    }
  }
}

template <class Archive>
void BaseClass::Serialize(Archive& ar) {
  ar(house_);
  auto count = static_cast<int32_t>(std::ssize(nodes_));
  ar(count);
  if constexpr (Archive::kIsReading) {
    // One node per cell is a sanity bound against a corrupt save, not a limit
    // the game enforces.
    if (!ar.ok() || count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid base node count");
      return;
    }
    nodes_.clear();
    nodes_.reserve(base::ToSize(count));
    for (int i = 0; i < count; ++i) {
      BaseNodeClass node;
      ar(node.type, node.cell);
      if (!ar.ok() || !magic_enum::enum_contains(node.type) ||
          node.cell.index() < 0 || node.cell.index() >= MAP_CELL_TOTAL) {
        ar.Fail("invalid base node");
        return;
      }
      nodes_.push_back(node);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      ar(base::At(nodes_, i).type, base::At(nodes_, i).cell);
    }
  }
}
template void BaseClass::Serialize(ArchiveWriter&);
template void BaseClass::Serialize(ArchiveReader&);

}  // namespace ra
