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

#include "ra/ai/prebuilt_base.h"

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
#include "engine/file/ini_file.h"
#include "engine/stream/archive.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/data/ini_types.h"
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

bool IsOnMap(const Cell cell) {
  return cell.index() >= 0 && cell.index() < MAP_CELL_TOTAL;
}

}  // namespace

BuildingClass* PrebuiltBase::FindBuilding(const BaseNode& node) {
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

bool PrebuiltBase::HasNodeFor(const BuildingClass& building) const {
  return std::ranges::any_of(nodes_, [&building](const BaseNode& node) {
    return FillsNode(building, node);
  });
}

BaseNode* PrebuiltBase::FindNode(const BuildingClass& building) {
  for (BaseNode& node : nodes_) {
    if (FillsNode(building, node)) {
      return &node;
    }
  }
  return nullptr;
}

void PrebuiltBase::AddNode(StructType type, Cell cell) {
  nodes_.emplace_back(type, cell);
}

void PrebuiltBase::RemoveNodeFor(const BuildingClass& building) {
  const auto node =
      std::ranges::find_if(nodes_, [&building](const BaseNode& candidate) {
        return FillsNode(building, candidate);
      });
  if (node != nodes_.end()) {
    nodes_.erase(node);
  }
}

const BaseNode* PrebuiltBase::FirstUnbuiltNode(const StructType type) const {
  for (const BaseNode& node : nodes_) {
    // STRUCT_NONE asks for the first hole of any type.
    if ((type == STRUCT_NONE || node.type == type) &&
        FindBuilding(node) == nullptr) {
      return &node;
    }
  }

  // No hole: the caller picks a location itself (see
  // BuildingClass::Exit_Object).
  return nullptr;
}

void PrebuiltBase::ReadIni(const engine::file::IniFile& ini) {
  // The scenario names the owner under "Player"; the human player's house is
  // only the fallback for a section that omits it.
  house_ = GetHousesType(ini, kIniSection, "Player", ThePlayer()->Class->House);

  const int node_count = ini.GetInt(kIniSection, "Count", 0);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < node_count; i++) {
    const std::string entry = absl::StrFormat("%03d", i);
    const std::string value = ini.GetString(kIniSection, entry);
    if (value.empty()) {
      continue;
    }

    base::Tokenizer tokens(value, ",");
    const StructType type = BuildingTypeClass::From_Name(tokens.Next());
    const Cell cell =
        Cell::FromIndex(base::ParseIntegerOr<int16_t>(tokens.Next(), -1));

    // A missing or malformed entry would leave a node no building can fill:
    // FindBuilding() indexes the map with its cell, and a save holding its
    // STRUCT_NONE type is rejected when loaded.
    if (type == STRUCT_NONE || !IsOnMap(cell)) {
      continue;
    }

    AddNode(type, cell);
  }
}

void PrebuiltBase::WriteIni(engine::file::IniFile& ini) const {
  ini.Clear(kIniSection);

  if (house_ == HOUSE_NONE) {
    return;
  }
  PutHousesType(ini, kIniSection, "Player", house_);

  // ReadIni() finds the entries by counting from "000" up to Count.
  ini.PutInt(kIniSection, "Count", static_cast<int>(std::ssize(nodes_)));

  for (base::ssize i = 0; i < std::ssize(nodes_); i++) {
    const auto& [type, cell] = base::At(nodes_, i);
    const std::string entry = absl::StrFormat("%03td", i);
    const std::string value = absl::StrFormat(
        "%s,%d", BuildingTypeClass::As_Reference(type).IniName, cell.index());

    ini.PutString(kIniSection, entry, value);
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
          !IsOnMap(node.cell)) {
        ar.Fail("invalid base node");
        return;
      }
      nodes_.push_back(node);
    }
  } else {
    for (auto& [type, cell] : nodes_) {
      ar(type, cell);
    }
  }
}
template void PrebuiltBase::Serialize(ArchiveWriter&);
template void PrebuiltBase::Serialize(ArchiveReader&);

}  // namespace ra
