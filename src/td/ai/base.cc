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

#include "td/ai/base.h"

#include <iterator>
#include <span>
#include <string>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "td/core/defines.h"
#include "td/data/profile.h"
#include "td/map/world.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/object.h"
#include "td/objects/type.h"

namespace td {

bool BaseNode::operator==(const BaseNode& other) const {
  return type == other.type && coord == other.coord;
}

bool BaseNode::operator!=(const BaseNode& node) const {
  return type != node.type || coord != node.coord;
}

bool BaseNode::operator>(const BaseNode& /*unused*/) { return true; }

PrebuiltBase::~PrebuiltBase() { nodes_.clear(); }

void PrebuiltBase::ReadIni(const char* ini_text) {
  BaseNode node;  // node to add to list

  // The base belongs to whoever opposes the human player.
  char player[20];
  WWGetPrivateProfileString("BASIC", "Player", "GoodGuy", player, ini_text);
  if (HouseTypeClass::From_Name(player) == HOUSE_GOOD) {
    house_ = HOUSE_BAD;
  } else {
    house_ = HOUSE_GOOD;
  }

  const int node_count =
      WWGetPrivateProfileInt(INI_Name(), "Count", 0, ini_text);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < node_count; i++) {
    char value[128];
    const std::string entry = absl::StrFormat("%03d", i);
    WWGetPrivateProfileString(INI_Name(), entry.c_str(), nullptr, value,
                              ini_text);

    base::Tokenizer tokens(value, ",");
    node.type = BuildingTypeClass::From_Name(tokens.Next());
    // A node without a type is one no building can fill, and a game saved
    // with it is rejected when loaded.
    if (node.type == STRUCT_NONE) {
      continue;
    }

    const char* coordinate_text = tokens.Next();
    if (coordinate_text == nullptr) {
      continue;
    }
    const auto coordinate = base::ParseDecimalBits(coordinate_text);
    if (!coordinate) {
      continue;
    }
    node.coord = Coordinate::FromPacked(*coordinate);
    // Cell::Containing() does not mask an off-map coordinate, so its cell
    // would index past the map in FindBuilding().
    if (node.coord.cell_x() >= MAP_CELL_W ||
        node.coord.cell_y() >= MAP_CELL_H) {
      continue;
    }

    nodes_.push_back(node);
  }
}

void PrebuiltBase::WriteIni(std::string& ini_text) {
  // Passing no entry deletes the whole section.
  WWWritePrivateProfileString(INI_Name(), nullptr, nullptr, ini_text);

  // ReadIni() finds the entries by counting from "000" up to Count.
  WWWritePrivateProfileInt(INI_Name(), "Count",
                           static_cast<int>(std::ssize(nodes_)), ini_text);

  for (int i = 0; i < std::ssize(nodes_); i++) {
    const std::string entry = absl::StrFormat("%03d", i);
    const std::string value = absl::StrFormat(
        "%s,%d",
        BuildingTypeClass::As_Reference(base::At(nodes_, i).type).IniName,
        static_cast<int>(base::At(nodes_, i).coord.packed()));

    WWWritePrivateProfileString(INI_Name(), entry.c_str(), value.c_str(),
                                ini_text);
  }
}

bool PrebuiltBase::IsBuilt(int node_index) {
  return FindBuilding(node_index) != nullptr;
}

BuildingClass* PrebuiltBase::FindBuilding(int node_index) {
  ObjectClass* obj[4];

  const Cell cell = Cell::Containing(base::At(nodes_, node_index).coord);

  // A building occupies the cells of its foundation and only overlaps the
  // others its image covers. The node's cell can be either, so look in both
  // lists.
  base::At(obj, 0) = TheMap().at(cell).Cell_Building();
  base::At(obj, 1) = base::At(TheMap().at(cell).Overlappers, 0);
  base::At(obj, 2) = base::At(TheMap().at(cell).Overlappers, 1);
  base::At(obj, 3) = base::At(TheMap().at(cell).Overlappers, 2);

  BuildingClass* bldg = nullptr;
  for (auto& i : obj) {
    if (i && i->Coord == base::At(nodes_, node_index).coord &&
        i->What_Am_I() == RTTI_BUILDING &&
        dynamic_cast<BuildingClass*>(i)->Class->Type ==
            base::At(nodes_, node_index).type) {
      bldg = dynamic_cast<BuildingClass*>(i);
      break;
    }
  }

  return bldg;
}

bool PrebuiltBase::HasNodeFor(BuildingClass* building) {
  return FindNode(building) != nullptr;
}

BaseNode* PrebuiltBase::FindNode(BuildingClass* building) {
  for (int i = 0; i < std::ssize(nodes_); i++) {
    if (building->Class->Type == base::At(nodes_, i).type &&
        building->Coord == base::At(nodes_, i).coord) {
      return &base::At(nodes_, i);
    }
  }
  return nullptr;
}

BaseNode* PrebuiltBase::FirstUnbuiltNode(StructType type) {
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

  // No hole: BuildingClass::Exit_Object() then does not place the building.
  return nullptr;
}

}  // namespace td
