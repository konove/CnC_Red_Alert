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

#include <algorithm>
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

namespace {

// The scenario INI section the base is stored in.
constexpr char kIniSection[] = "Base";

// Returns whether `building` is the one `node` asks for. A building of the
// node's type that stands elsewhere and merely covers the node's cell is not.
bool FillsNode(const BuildingClass& building, const BaseNode& node) {
  return building.Class->Type == node.type &&
         Cell::Containing(building.Coord) == node.cell;
}

}  // namespace

bool PrebuiltBase::IsBuilt(int node_index) const {
  return FindBuilding(node_index) != nullptr;
}

BuildingClass* PrebuiltBase::FindBuilding(int node_index) const {
  const BaseNode& node = base::At(nodes_, node_index);
  const CellClass& map_cell = TheMap().at(node.cell);

  // A building occupies the cells of its foundation and only overlaps the
  // others its image covers. Its top-left cell can be either, so look in both
  // lists.
  if (BuildingClass* occupier = map_cell.Cell_Building();
      occupier != nullptr && FillsNode(*occupier, node)) {
    return occupier;
  }
  for (ObjectClass* overlapper : map_cell.Overlappers) {
    if (auto* building = dynamic_cast<BuildingClass*>(overlapper);
        building != nullptr && FillsNode(*building, node)) {
      return building;
    }
  }
  return nullptr;
}

bool PrebuiltBase::HasNodeFor(const BuildingClass* building) const {
  return std::ranges::any_of(nodes_, [building](const BaseNode& node) {
    return FillsNode(*building, node);
  });
}

BaseNode* PrebuiltBase::FindNode(const BuildingClass* building) {
  for (BaseNode& node : nodes_) {
    if (FillsNode(*building, node)) {
      return &node;
    }
  }
  return nullptr;
}

const BaseNode* PrebuiltBase::FirstUnbuiltNode(StructType type) const {
  for (int i = 0; i < std::ssize(nodes_); i++) {
    const BaseNode& node = base::At(nodes_, i);
    // STRUCT_NONE asks for the first hole of any type.
    if ((type == STRUCT_NONE || node.type == type) && !IsBuilt(i)) {
      return &node;
    }
  }

  // No hole: the caller picks a location itself (see
  // BuildingClass::Exit_Object).
  return nullptr;
}

void PrebuiltBase::ReadIni(const CCINIClass& ini) {
  // The scenario names the owner under "Player"; the human player's house is
  // only the fallback for a section that omits it.
  house_ = ini.Get_HousesType(kIniSection, "Player", ThePlayer()->Class->House);

  const int node_count = ini.Get_Int(kIniSection, "Count", 0);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < node_count; i++) {
    char value[128];
    const std::string entry = absl::StrFormat("%03d", i);
    ini.Get_String(kIniSection, entry.c_str(), nullptr, value, sizeof(value));

    base::Tokenizer tokens(value, ",");
    const StructType type = BuildingTypeClass::From_Name(tokens.Next());
    const Cell cell =
        Cell::FromIndex(base::ParseIntegerOr<int16_t>(tokens.Next(), -1));

    // A missing or malformed entry would leave a node no building can fill:
    // FindBuilding() indexes the map with its cell, and a save holding its
    // STRUCT_NONE type is rejected when loaded.
    if (type == STRUCT_NONE || cell.index() < 0 ||
        cell.index() >= MAP_CELL_TOTAL) {
      continue;
    }

    nodes_.emplace_back(type, cell);
  }
}

void PrebuiltBase::WriteIni(CCINIClass& ini) const {
  ini.Clear(kIniSection);

  if (house_ == HOUSE_NONE) {
    return;
  }
  ini.Put_HousesType(kIniSection, "Player", house_);

  // ReadIni() finds the entries by counting from "000" up to Count.
  ini.Put_Int(kIniSection, "Count", static_cast<int>(std::ssize(nodes_)));

  for (base::ssize i = 0; i < std::ssize(nodes_); i++) {
    const BaseNode& node = base::At(nodes_, i);
    const std::string entry = absl::StrFormat("%03td", i);
    const std::string value = absl::StrFormat(
        "%s,%d", BuildingTypeClass::As_Reference(node.type).IniName,
        node.cell.index());

    ini.Put_String(kIniSection, entry.c_str(), value.c_str());
  }
}

template <class Archive>
void PrebuiltBase::Serialize(Archive& ar) {
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
      BaseNode node;
      ar(node.type, node.cell);
      if (!ar.ok() || !magic_enum::enum_contains(node.type) ||
          node.cell.index() < 0 || node.cell.index() >= MAP_CELL_TOTAL) {
        ar.Fail("invalid base node");
        return;
      }
      nodes_.push_back(node);
    }
  } else {
    for (BaseNode& node : nodes_) {
      ar(node.type, node.cell);
    }
  }
}
template void PrebuiltBase::Serialize(ArchiveWriter&);
template void PrebuiltBase::Serialize(ArchiveReader&);

}  // namespace ra
