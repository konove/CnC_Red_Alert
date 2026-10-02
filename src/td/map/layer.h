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

/* $Header:   F:\projects\c&c\vcs\code\layer.h_v   2.19   16 Oct 1995 16:46:22
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LAYER.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 31, 1994 *
 *                                                                                             *
 *                  Last Update : May 31, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_MAP_LAYER_H_
#define CNC_RED_ALERT_TD_MAP_LAYER_H_

#include <vector>

#include "base/array.h"
#include "td/objects/object.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

// One of the display's object layers (ground, air, top), or the logic
// list: the objects in the order they are drawn or processed. A layer may be
// kept roughly sorted for drawing; Sort() makes one bubble pass per call and
// Sorted_Add() inserts before the first object that sorts after the new one.
// Both orders are game state: they decide draw order and, for the logic list,
// the order objects act in, which multiplayer games must agree on.
class LayerClass {
 public:
  LayerClass() = default;
  virtual ~LayerClass();
  LayerClass(const LayerClass&) = delete;
  LayerClass& operator=(const LayerClass&) = delete;
  LayerClass(LayerClass&&) = delete;
  LayerClass& operator=(LayerClass&&) = delete;

  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  // Returns the number of objects in the layer.
  [[nodiscard]] int Count() const { return static_cast<int>(objects_.size()); }

  // Returns the object at `index`, which must be in [0, Count()).
  [[nodiscard]] ObjectClass* at(int index) const {
    return base::At(objects_, index);
  }

  // Returns the position of `object` in the layer, or -1.
  [[nodiscard]] int ID(const ObjectClass* object) const;

  // Removes `object`, keeping the others in order. Returns false if the layer
  // does not hold it.
  bool Delete(const ObjectClass* object);

  // Empties the layer.
  void Clear() { objects_.clear(); }

  // Makes one bubble pass towards draw order.
  void Sort();

  // Appends `object`, or with `sort` inserts it in draw order. Returns true.
  bool Submit(ObjectClass* object, bool sort = false);

  // Inserts `object` before the first object that sorts after it. Returns
  // true.
  bool Sorted_Add(ObjectClass* object);

  virtual void Init() { Clear(); }
  virtual void One_Time() {}

 private:
  std::vector<ObjectClass*> objects_;
};

extern template void LayerClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void LayerClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_MAP_LAYER_H_
