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

#include "td/ai/prebuilt_base.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "base/types.h"
#include "engine/file/ini_file.h"
#include "td/core/coordinate.h"
#include "td/core/defines.h"
#include "td/data/profile.h"
#include "td/map/world.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/object.h"
#include "td/objects/type.h"

namespace td {

namespace {

// The scenario INI section the base is stored in.
constexpr char kIniSection[] = "Base";

// Returns whether `building` is the one `node` asks for.
bool FillsNode(const BuildingClass& building, const BaseNode& node) {
  return building.Class->Type == node.type && building.Coord == node.coord;
}

// Cell::Containing() does not mask an off-map coordinate, so its cell would
// index past the map in FindBuilding().
bool IsOnMap(const Coordinate coord) {
  return coord.cell_x() < MAP_CELL_W && coord.cell_y() < MAP_CELL_H;
}

}  // namespace

void PrebuiltBase::ReadIni(const engine::file::IniFile& ini) {
  // The base belongs to whoever opposes the human player.
  char player[20];
  ini.GetString("BASIC", "Player", "GoodGuy", player);
  house_ =
      HouseTypeClass::From_Name(player) == HOUSE_GOOD ? HOUSE_BAD : HOUSE_GOOD;

  const int node_count = ini.GetInt(kIniSection, "Count", 0);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < node_count; i++) {
    char value[128];
    const std::string entry = absl::StrFormat("%03d", i);
    ini.GetString(kIniSection, entry, "", value);

    base::Tokenizer tokens(value, ",");
    const StructType type = BuildingTypeClass::From_Name(tokens.Next());
    // A node without a type is one no building can fill, and a game saved
    // with it is rejected when loaded.
    if (type == STRUCT_NONE) {
      continue;
    }

    const char* coordinate_text = tokens.Next();
    if (coordinate_text == nullptr) {
      continue;
    }
    const auto packed = base::ParseDecimalBits(coordinate_text);
    if (!packed) {
      continue;
    }
    const Coordinate coord = Coordinate::FromPacked(*packed);
    if (!IsOnMap(coord)) {
      continue;
    }

    AddNode(type, coord);
  }
}

void PrebuiltBase::WriteIni(std::string& ini_text) const {
  // Passing no entry deletes the whole section.
  WWWritePrivateProfileString(kIniSection, nullptr, nullptr, ini_text);

  // ReadIni() finds the entries by counting from "000" up to Count.
  WWWritePrivateProfileInt(kIniSection, "Count",
                           static_cast<int>(std::ssize(nodes_)), ini_text);

  for (base::ssize i = 0; i < std::ssize(nodes_); i++) {
    const auto& [type, coord] = base::At(nodes_, i);
    const std::string entry = absl::StrFormat("%03td", i);
    const std::string value =
        absl::StrFormat("%s,%d", BuildingTypeClass::As_Reference(type).IniName,
                        static_cast<int>(coord.packed()));

    WWWritePrivateProfileString(kIniSection, entry.c_str(), value.c_str(),
                                ini_text);
  }
}

BuildingClass* PrebuiltBase::FindBuilding(const BaseNode& node) {
  const CellClass& map_cell = TheMap().at(Cell::Containing(node.coord));

  // A building occupies the cells of its foundation and only overlaps the
  // others its image covers. The node's cell can be either, so look in both
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

void PrebuiltBase::AddNode(const StructType type, const Coordinate coord) {
  nodes_.emplace_back(type, coord);
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

void PrebuiltBase::RemoveFirstNodeOfType(const StructType type) {
  const auto node = std::ranges::find(nodes_, type, &BaseNode::type);
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

  // No hole: BuildingClass::Exit_Object() then does not place the building.
  return nullptr;
}

}  // namespace td
