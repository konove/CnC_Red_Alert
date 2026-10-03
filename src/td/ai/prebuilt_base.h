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

#ifndef CNC_RED_ALERT_TD_AI_PREBUILT_BASE_H_
#define CNC_RED_ALERT_TD_AI_PREBUILT_BASE_H_

#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "td/core/coordinate.h"
#include "td/core/defines.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

class BuildingClass;

// One entry of the prebuilt base: a type of building and the coordinate it is
// placed at, which is the coordinate the placed building reports. Nodes are
// equal when both match.
struct BaseNode {
  bool operator==(const BaseNode&) const = default;

  StructType type = STRUCT_NONE;  // STRUCT_NONE until a node is filled in.
  Coordinate coord{};             // Where the building is placed.
};

// The prebuilt base of a single-player scenario: the buildings the computer
// house puts up, in list order, each at a fixed coordinate. A node is "built"
// while a building of its type stands at its coordinate, so a building placed
// by the scenario's [STRUCTURES] section fills its node from the start, and one
// that is destroyed leaves a hole the house builds again. A scenario has at
// most one; the World owns it.
//
// Example:
//   const PrebuiltBase& base = TheWorld().base();
//   if (const BaseNode* node = base.FirstUnbuiltNode()) {
//     techno = &BuildingTypeClass::As_Reference(node->type);
//   }
class PrebuiltBase {
 public:
  // Field-wise saved-game state; read and write share this field list. Defined
  // in td/saveload/saveglobals.cc.
  template <class Archive>
  void Serialize(Archive& ar);

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
  void WriteIni(std::string& ini_text) const;

  // Returns the building that fills `node`: one of the node's type at the
  // node's coordinate, whichever house owns it. Returns nullptr if the node is
  // a hole. The node's coordinate must be on the map.
  [[nodiscard]] static BuildingClass* FindBuilding(const BaseNode& node);

  // Returns whether the base has a node for `building`: one with its type and
  // coordinate.
  [[nodiscard]] bool HasNodeFor(const BuildingClass& building) const;

  // Returns the first node for `building`, or nullptr if the base has none.
  BaseNode* FindNode(const BuildingClass& building)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Appends a node for a building of `type` placed at `coord`, which must be
  // on the map.
  void AddNode(StructType type, Coordinate coord);

  // Removes the first node for `building`, if the base has one.
  void RemoveNodeFor(const BuildingClass& building);

  // Removes the first node of `type`, if the base has one.
  void RemoveFirstNodeOfType(StructType type);

  // Returns the first node that is not built: of type `type`, or of any type
  // when `type` is STRUCT_NONE. Returns nullptr when every such node is built.
  [[nodiscard]] const BaseNode* FirstUnbuiltNode(
      StructType type = STRUCT_NONE) const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // The buildings of the base, in the order the house builds them. A node whose
  // building is destroyed stays; one whose building type was sabotaged in the
  // previous mission is removed when the scenario starts.
  [[nodiscard]] const std::vector<BaseNode>& nodes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return nodes_;
  }

  // The house that builds the base; HOUSE_NONE before a scenario is read.
  [[nodiscard]] HousesType house() const { return house_; }
  void set_house(const HousesType house) { house_ = house; }

 private:
  std::vector<BaseNode> nodes_;
  HousesType house_ = HOUSE_NONE;
};

extern template void PrebuiltBase::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void PrebuiltBase::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_AI_PREBUILT_BASE_H_
