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

// File: UnitTrackerClass, the per-type counts behind the internet game
// statistics.
//
// Originally UTRACKER.CPP by Steve Tall, June 3-7, 1996.

#include "game/sim/utracker.h"

#include <algorithm>
#include <cstdint>
#include <span>

#include "base/numeric.h"

// htonl() and ntohl(), for the byte order the statistics packet sends.
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

namespace game {

UnitTrackerClass::UnitTrackerClass(int unit_count)
    : UnitTotals(base::ToSize(unit_count)), UnitCount(unit_count) {
  Clear_Unit_Total();
}

UnitTrackerClass::~UnitTrackerClass() = default;

void UnitTrackerClass::Increment_Unit_Total(int unit_type) {
  UnitTotals.at(base::ToSize(unit_type))++;
}

void UnitTrackerClass::Decrement_Unit_Total(int unit_type) {
  UnitTotals.at(base::ToSize(unit_type))--;
}

std::span<const int32_t> UnitTrackerClass::Get_All_Totals() const {
  return UnitTotals;
}

int UnitTrackerClass::Get_Unit_Total(int unit_type) const {
  return UnitTotals.at(base::ToSize(unit_type));
}

// Zero is zero in either byte order, so the counts are back in host order.
void UnitTrackerClass::Clear_Unit_Total() {
  std::ranges::fill(UnitTotals, 0);
  InNetworkFormat = false;
}

void UnitTrackerClass::To_Network_Format() {
  if (!InNetworkFormat) {
    for (int i = 0; i < UnitCount; i++) {
      UnitTotals.at(base::ToSize(i)) = static_cast<int32_t>(
          htonl(static_cast<uint32_t>(UnitTotals.at(base::ToSize(i)))));
    }
  }
  InNetworkFormat = true;
}

void UnitTrackerClass::To_PC_Format() {
  if (InNetworkFormat) {
    for (int i = 0; i < UnitCount; i++) {
      UnitTotals.at(base::ToSize(i)) = static_cast<int32_t>(
          ntohl(static_cast<uint32_t>(UnitTotals.at(base::ToSize(i)))));
    }
  }
  InNetworkFormat = false;
}

}  // namespace game
