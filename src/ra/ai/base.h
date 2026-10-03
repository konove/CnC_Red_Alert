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

// File: The prebuilt base, the list of buildings a scenario tells the computer
// to put up and to rebuild when they are destroyed.

#ifndef CNC_RED_ALERT_RA_AI_BASE_H_
#define CNC_RED_ALERT_RA_AI_BASE_H_

#include <vector>

#include "absl/base/attributes.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class BuildingClass;
class CCINIClass;

// One entry of the prebuilt base: a type of building and the cell its top-left
// corner goes on. Nodes are equal when both match.
struct BaseNode {
  bool operator==(const BaseNode&) const = default;

  StructType type = STRUCT_NONE;  // STRUCT_NONE until a node is filled in.
  Cell cell{};                    // The building's top-left cell.
};

// The prebuilt base of a single-player scenario: the buildings one computer
// house puts up, in list order, each on a fixed cell. A node is "built" while a
// building of its type stands on its cell, so a building placed by the
// scenario's [STRUCTURES] section fills its node from the start, and one that
// is destroyed leaves a hole the house builds again. A scenario has at most
// one; the World owns it.
//
// Example:
//   const PrebuiltBase& base = TheWorld().base();
//   if (const BaseNode* node = base.FirstUnbuiltNode()) {
//     house.BuildStructure = node->type;
//   }
class PrebuiltBase {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  // Empties the base and leaves it without an owner, as it is before a
  // scenario is read.
  void Clear() {
    house_ = HOUSE_NONE;
    nodes_.clear();
  }

  // Appends the nodes of the scenario's [Base] section, in the order the
  // section numbers them, and takes the owner from its Player entry; without
  // one the owner is the human player's house. An entry is "NNN=TYPE,cell",
  // NNN counting from 000 up to the section's Count; an entry that is missing,
  // names no building type or has a cell off the map is skipped. Call Clear()
  // first: the list is not cleared.
  void ReadIni(const CCINIClass& ini);

  // Replaces the [Base] section of `ini` with this base. A base without an
  // owner (HOUSE_NONE) leaves the section empty.
  void WriteIni(CCINIClass& ini) const;

  // Returns whether the building of node `node_index` stands on the map.
  [[nodiscard]] bool IsBuilt(int node_index) const;

  // Returns the building that fills node `node_index`: one of the node's type
  // whose top-left cell is the node's cell, whichever house owns it. Returns
  // nullptr if the node is a hole. `node_index` must be a valid index into
  // nodes().
  [[nodiscard]] BuildingClass* FindBuilding(int node_index) const;

  // Returns whether the base has a node for `building`, which must not be
  // nullptr.
  [[nodiscard]] bool HasNodeFor(const BuildingClass* building) const;

  // Returns the first node with the type and top-left cell of `building`,
  // which must not be nullptr, or nullptr if the base has none.
  BaseNode* FindNode(const BuildingClass* building)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Returns the first node that is not built: of type `type`, or of any type
  // when `type` is STRUCT_NONE. Returns nullptr when every such node is built.
  [[nodiscard]] const BaseNode* FirstUnbuiltNode(
      StructType type = STRUCT_NONE) const ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // The buildings of the base, in the order the house builds them. A node whose
  // building is destroyed is not removed; a destroyed building flagged for
  // rebuilding that has no node gets one appended (BuildingClass::Take_Damage).
  std::vector<BaseNode>& nodes() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return nodes_;
  }
  [[nodiscard]] const std::vector<BaseNode>& nodes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return nodes_;
  }

  // The house that builds the base; HOUSE_NONE if the scenario has none.
  [[nodiscard]] HousesType house() const { return house_; }
  void set_house(HousesType house) { house_ = house; }

 private:
  std::vector<BaseNode> nodes_;
  HousesType house_ = HOUSE_NONE;
};

extern template void PrebuiltBase::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void PrebuiltBase::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_BASE_H_
