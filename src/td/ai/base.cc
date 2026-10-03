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

bool BaseNodeClass::operator==(const BaseNodeClass& node) const {
  return Type == node.Type && Coord == node.Coord;
}

bool BaseNodeClass::operator!=(const BaseNodeClass& node) const {
  return Type != node.Type || Coord != node.Coord;
}

bool BaseNodeClass::operator>(const BaseNodeClass& /*unused*/) { return true; }

BaseClass::~BaseClass() { Nodes.clear(); }

void BaseClass::Read_INI(const char* buffer) {
  BaseNodeClass node;  // node to add to list

  // The base belongs to whoever opposes the human player.
  char player[20];
  WWGetPrivateProfileString("BASIC", "Player", "GoodGuy", player, buffer);
  if (HouseTypeClass::From_Name(player) == HOUSE_GOOD) {
    House = HOUSE_BAD;
  } else {
    House = HOUSE_GOOD;
  }

  const int count = WWGetPrivateProfileInt(INI_Name(), "Count", 0, buffer);

  // The entries are named by their number so that they are read in the order
  // they were written; the list order is the build order.
  for (int i = 0; i < count; i++) {
    char value[128];
    const std::string entry = absl::StrFormat("%03d", i);
    WWGetPrivateProfileString(INI_Name(), entry.c_str(), nullptr, value,
                              buffer);

    base::Tokenizer tokens(value, ",");
    node.Type = BuildingTypeClass::From_Name(tokens.Next());
    // A node without a type is one no building can fill, and a game saved
    // with it is rejected when loaded.
    if (node.Type == STRUCT_NONE) {
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
    node.Coord = Coordinate::FromPacked(*coordinate);
    // Cell::Containing() does not mask an off-map coordinate, so its cell
    // would index past the map in Get_Building().
    if (node.Coord.cell_x() >= MAP_CELL_W ||
        node.Coord.cell_y() >= MAP_CELL_H) {
      continue;
    }

    Nodes.push_back(node);
  }
}

void BaseClass::Write_INI(std::string& buffer) {
  // Passing no entry deletes the whole section.
  WWWritePrivateProfileString(INI_Name(), nullptr, nullptr, buffer);

  // Read_INI() finds the entries by counting from "000" up to Count.
  WWWritePrivateProfileInt(INI_Name(), "Count",
                           static_cast<int>(std::ssize(Nodes)), buffer);

  for (int i = 0; i < std::ssize(Nodes); i++) {
    const std::string entry = absl::StrFormat("%03d", i);
    const std::string value = absl::StrFormat(
        "%s,%d",
        BuildingTypeClass::As_Reference(base::At(Nodes, i).Type).IniName,
        static_cast<int>(base::At(Nodes, i).Coord.packed()));

    WWWritePrivateProfileString(INI_Name(), entry.c_str(), value.c_str(),
                                buffer);
  }
}

bool BaseClass::Is_Built(int index) { return Get_Building(index) != nullptr; }

BuildingClass* BaseClass::Get_Building(int index) {
  ObjectClass* obj[4];

  const Cell cell = Cell::Containing(base::At(Nodes, index).Coord);

  // A building occupies the cells of its foundation and only overlaps the
  // others its image covers. The node's cell can be either, so look in both
  // lists.
  base::At(obj, 0) = TheMap().at(cell).Cell_Building();
  base::At(obj, 1) = base::At(TheMap().at(cell).Overlappers, 0);
  base::At(obj, 2) = base::At(TheMap().at(cell).Overlappers, 1);
  base::At(obj, 3) = base::At(TheMap().at(cell).Overlappers, 2);

  BuildingClass* bldg = nullptr;
  for (auto& i : obj) {
    if (i && i->Coord == base::At(Nodes, index).Coord &&
        i->What_Am_I() == RTTI_BUILDING &&
        dynamic_cast<BuildingClass*>(i)->Class->Type ==
            base::At(Nodes, index).Type) {
      bldg = dynamic_cast<BuildingClass*>(i);
      break;
    }
  }

  return bldg;
}

bool BaseClass::Is_Node(BuildingClass* obj) { return Get_Node(obj) != nullptr; }

BaseNodeClass* BaseClass::Get_Node(BuildingClass* obj) {
  for (int i = 0; i < std::ssize(Nodes); i++) {
    if (obj->Class->Type == base::At(Nodes, i).Type &&
        obj->Coord == base::At(Nodes, i).Coord) {
      return &base::At(Nodes, i);
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

  // No hole: BuildingClass::Exit_Object() then does not place the building.
  return nullptr;
}

}  // namespace td
