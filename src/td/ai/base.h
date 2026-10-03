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

// File: The prebuilt base, the list of buildings a scenario tells the computer
// to put up and to rebuild when they are destroyed.

#ifndef CNC_RED_ALERT_TD_AI_BASE_H_
#define CNC_RED_ALERT_TD_AI_BASE_H_

#include <span>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "td/core/defines.h"
#include "td/objects/building.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

// One entry of the prebuilt base: a type of building and the coordinate it is
// placed at, which is the coordinate the placed building reports.
class BaseNodeClass {
 public:
  BaseNodeClass() = default;
  // Nodes are equal when they name the same building type at the same
  // coordinate.
  bool operator==(const BaseNodeClass& other) const;
  bool operator!=(const BaseNodeClass& node) const;
  // Always true: the nodes have no order.
  bool operator>(const BaseNodeClass& node);

  StructType type = STRUCT_NONE;  // STRUCT_NONE until a node is filled in.
  Coordinate coord{};             // Where the building is placed.
};

// The prebuilt base of a single-player scenario: the buildings the computer
// house puts up, in list order, each at a fixed coordinate. A node is "built"
// while a building of its type stands at its coordinate, so a building placed
// by the scenario's [STRUCTURES] section fills its node from the start, and one
// that is destroyed leaves a hole the house builds again. A scenario has at
// most one; the World owns it. (Despite its name, this is not a base class of
// anything.)
//
// Example:
//   BaseClass& base = TheWorld().base();
//   if (const BaseNodeClass* node = base.FirstUnbuiltNode()) {
//     techno = &BuildingTypeClass::As_Reference(node->type);
//   }
class BaseClass {
 public:
  BaseClass() = default;
  virtual ~BaseClass();
  BaseClass(const BaseClass&) = delete;
  BaseClass& operator=(const BaseClass&) = delete;
  BaseClass(BaseClass&&) = delete;
  BaseClass& operator=(BaseClass&&) = delete;

  // Empties the node list. The owner stays: ReadIni() always sets it.
  void ClearNodes() { nodes_.clear(); }

  // Appends the nodes of the [Base] section of the scenario INI text in
  // `ini_text`, in the order the section numbers them. An entry is
  // "NNN=TYPE,coordinate", NNN counting from 000 up to the section's Count; an
  // entry that is missing, names no building type or lies off the map is
  // skipped. The owner becomes the house opposing the [BASIC] Player: HOUSE_BAD
  // against GoodGuy, HOUSE_GOOD against anyone else. Call ClearNodes() first:
  // the list is not cleared.
  void ReadIni(const char* ini_text);

  // Replaces the [Base] section of the INI text in `ini_text` with this base's
  // nodes. The owner is not written.
  void WriteIni(std::string& ini_text);

  // The name of the scenario INI section the base is stored in.
  static const char* INI_Name() { return "Base"; }

  // Field-wise saved-game state; read and write share this field list. Defined
  // in td/saveload/saveglobals.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Returns whether the building of node `node_index` stands on the map.
  bool IsBuilt(int node_index);

  // Returns the building that fills node `node_index`: one of the node's type
  // at the node's coordinate, whichever house owns it. Returns nullptr if the
  // node is a hole. `node_index` must be a valid index into nodes().
  BuildingClass* FindBuilding(int node_index);

  // Returns whether the base has a node for `building`, which must not be
  // nullptr.
  bool HasNodeFor(BuildingClass* building);

  // Returns the first node with the type and coordinate of `building`, which
  // must not be nullptr, or nullptr if the base has none.
  BaseNodeClass* FindNode(BuildingClass* building)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // Returns node `index`, which must be a valid index into nodes().
  BaseNodeClass* Get_Node(int index) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return &base::At(nodes_, index);
  }

  // Returns the first node that is not built: of type `type`, or of any type
  // when `type` is STRUCT_NONE. Returns nullptr when every such node is built.
  BaseNodeClass* FirstUnbuiltNode(StructType type = STRUCT_NONE)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // The buildings of the base, in the order the house builds them. A node whose
  // building is destroyed stays; one whose building type was sabotaged in the
  // previous mission is removed when the scenario starts.
  std::vector<BaseNodeClass>& nodes() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return nodes_;
  }
  [[nodiscard]] const std::vector<BaseNodeClass>& nodes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return nodes_;
  }

  // The house that builds the base; HOUSE_NONE before a scenario is read.
  [[nodiscard]] HousesType house() const { return house_; }
  void set_house(HousesType house) { house_ = house; }

 private:
  std::vector<BaseNodeClass> nodes_;
  HousesType house_ = HOUSE_NONE;
};

extern template void BaseClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void BaseClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_AI_BASE_H_
