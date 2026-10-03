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

// One house's view of one map region: how much enemy firepower stands in it.
// The map is divided into regions of kRegionWidth x kRegionHeight cells, and
// each house keeps one Region per map region (HouseClass::Regions).

#ifndef CNC_RED_ALERT_RA_AI_REGION_H_
#define CNC_RED_ALERT_RA_AI_REGION_H_

#include <cstdint>

namespace ra {

// The threat a house sees in one map region: the summed risk ratings of the
// other houses' ground objects standing in the region, plus half of those in
// the regions beside it and a quarter of those diagonal to it
// (HouseClass::Adjust_Threat() does the spreading). A computer house counts
// every other house's objects, a human house only those it is not allied with
// (CellClass::Adjust_Threat()). Path finding and team routing read the value
// through MapClass::Cell_Threat() to steer around defended ground.
//
// Only the campaign tracks threat; in a multiplayer game every region stays 0.
//
// The value is kept up to date incrementally: an object adds its rating when
// it is placed on the map and subtracts the same amount when it is lifted, so
// the two calls must be given equal amounts or the region drifts.
class Region {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(threat_);
  }

  // Adds `delta` to the region's threat; negative when an object leaves.
  void AdjustThreat(const int delta) { threat_ += delta; }

  // The region's current threat; 0 when no enemy object is in or next to it.
  [[nodiscard]] int threat() const { return static_cast<int>(threat_); }

 private:
  // Summed threat ratings; 0 for an empty region. Saved as 64 bits, though the
  // sum of a map's worth of ratings fits an int.
  int64_t threat_ = 0;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_REGION_H_
