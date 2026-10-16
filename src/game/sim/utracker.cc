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

// File: TypeTally, the per-type counts behind the internet game
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

TypeTally::TypeTally(int type_count)
    : counts_(base::ToSize(type_count)), UnitCount(type_count) {
  Clear();
}

TypeTally::~TypeTally() = default;

void TypeTally::Increment(int type) { counts_.at(base::ToSize(type))++; }

void TypeTally::Decrement(int type) { counts_.at(base::ToSize(type))--; }

std::span<const int32_t> TypeTally::Get_All_Totals() const { return counts_; }

int TypeTally::Count(int type) const { return counts_.at(base::ToSize(type)); }

// Zero is zero in either byte order, so the counts are back in host order.
void TypeTally::Clear() {
  std::ranges::fill(counts_, 0);
  InNetworkFormat = false;
}

void TypeTally::To_Network_Format() {
  if (!InNetworkFormat) {
    for (int i = 0; i < UnitCount; i++) {
      counts_.at(base::ToSize(i)) = static_cast<int32_t>(
          htonl(static_cast<uint32_t>(counts_.at(base::ToSize(i)))));
    }
  }
  InNetworkFormat = true;
}

void TypeTally::To_PC_Format() {
  if (InNetworkFormat) {
    for (int i = 0; i < UnitCount; i++) {
      counts_.at(base::ToSize(i)) = static_cast<int32_t>(
          ntohl(static_cast<uint32_t>(counts_.at(base::ToSize(i)))));
    }
  }
  InNetworkFormat = false;
}

}  // namespace game
