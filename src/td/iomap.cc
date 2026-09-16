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

#include <algorithm>
#include <cstdint>

#include "base/array.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/cell.h"
#include "td/conquer.h"
#include "td/credits.h"
#include "td/defines.h"
#include "td/display.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/gscreen.h"
#include "td/help.h"
#include "td/map.h"
#include "td/mouse.h"
#include "td/power.h"
#include "td/radar.h"
#include "td/scroll.h"
#include "td/serialize.h"
#include "td/sidebar.h"
#include "td/tab.h"
#include "td/type.h"
#include "tech/archive.h"

template <class Archive>
void CellClass::Serialize(Archive& ar) {
  auto flags = static_cast<uint8_t>(
      (IsPlot ? 1U : 0U) | (IsCursorHere ? 1U << 1 : 0U) |
      (IsMapped ? 1U << 2 : 0U) | (IsVisible ? 1U << 3 : 0U) |
      (IsTrigger ? 1U << 4 : 0U) | (IsWaypoint ? 1U << 5 : 0U) |
      (IsRadarCursor ? 1U << 6 : 0U) | (IsFlagged ? 1U << 7 : 0U));
  ar(flags, TType, TIcon, Overlay, OverlayData, Smudge, SmudgeData, Owner,
     InfType, ObjectPtr(OccupierPtr));
  int32_t overlapper_count = 3;
  ar(overlapper_count);
  if constexpr (Archive::kIsReading) {
    if (overlapper_count < 0 || overlapper_count > 3) {
      ar.Fail("invalid cell overlapper count");
      return;
    }
    std::ranges::fill(Overlappers, nullptr);
  }
  for (int32_t i = 0; i < overlapper_count; ++i) {
    ar(ObjectPtr(base::At(Overlappers, i)));
  }
  ar(Flag.Composite, Land);
  if constexpr (Archive::kIsReading) {
    IsPlot = (flags & 1) != 0;
    IsCursorHere = (flags & 2) != 0;
    IsMapped = (flags & 4) != 0;
    IsVisible = (flags & 8) != 0;
    IsTrigger = (flags & 16) != 0;
    IsWaypoint = (flags & 32) != 0;
    IsRadarCursor = (flags & 64) != 0;
    IsFlagged = (flags & 128) != 0;
    if ((TType != TEMPLATE_NONE && TType >= TEMPLATE_COUNT) ||
        Overlay < OVERLAY_NONE || Overlay >= OVERLAY_COUNT ||
        Smudge < SMUDGE_NONE || Smudge >= SMUDGE_COUNT || Owner < HOUSE_NONE ||
        Owner >= HOUSE_COUNT || InfType < HOUSE_NONE ||
        InfType >= HOUSE_COUNT || Land < LAND_CLEAR || Land >= LAND_COUNT) {
      ar.Fail("invalid saved cell attributes");
    }
  }
  auto& trigger = CellTriggers[Cell_Number()];
  if (IsTrigger) {
    ar(TriggerPtr(trigger));
    if constexpr (Archive::kIsReading) {
      if (trigger == nullptr) {
        ar.Fail("missing saved cell trigger");
      }
    }
  } else if constexpr (Archive::kIsReading) {
    trigger = nullptr;
  }
}
template void CellClass::Serialize(ArchiveWriter&);
template void CellClass::Serialize(ArchiveReader&);

template <class Archive>
void MapClass::Serialize(Archive& ar) {
  bool forward = IsForwardScan;
  ar(MapCellX, MapCellY, MapCellWidth, MapCellHeight, TotalValue,
     TiberiumGrowthCount, TiberiumSpreadCount, TiberiumScan, forward);
  if constexpr (Archive::kIsReading) {
    if (MapCellX < 0 || MapCellY < 0 || MapCellWidth <= 0 ||
        MapCellHeight <= 0 || MapCellWidth > MAP_CELL_W ||
        MapCellHeight > MAP_CELL_H || MapCellX > MAP_CELL_W - MapCellWidth ||
        MapCellY > MAP_CELL_H - MapCellHeight || TiberiumGrowthCount < 0 ||
        TiberiumGrowthCount > 50 || TiberiumSpreadCount < 0 ||
        TiberiumSpreadCount > 50 || TiberiumScan < 0 ||
        TiberiumScan >= MAP_CELL_TOTAL) {
      ar.Fail("invalid map dimensions or tiberium scan state");
      return;
    }
    IsForwardScan = forward;
    std::ranges::fill(TiberiumGrowth, 0);
    std::ranges::fill(TiberiumSpread, 0);
  }
  for (int i = 0; i < TiberiumGrowthCount; ++i) {
    ar(base::At(TiberiumGrowth, i));
    if constexpr (Archive::kIsReading) {
      if (base::At(TiberiumGrowth, i) < 0 ||
          base::At(TiberiumGrowth, i) >= MAP_CELL_TOTAL) {
        ar.Fail("invalid tiberium growth cell");
      }
    }
  }
  for (int i = 0; i < TiberiumSpreadCount; ++i) {
    ar(base::At(TiberiumSpread, i));
    if constexpr (Archive::kIsReading) {
      if (base::At(TiberiumSpread, i) < 0 ||
          base::At(TiberiumSpread, i) >= MAP_CELL_TOTAL) {
        ar.Fail("invalid tiberium spread cell");
      }
    }
  }
}
template void MapClass::Serialize(ArchiveWriter&);
template void MapClass::Serialize(ArchiveReader&);

template <class Archive>
void DisplayClass::Serialize(Archive& ar) {
  MapClass::Serialize(ar);
  ar(TacticalCoord, DesiredTacticalCoord, ZoneCell, ZoneOffset,
     ObjectPtr(PendingObjectPtr), PendingHouse);
  if constexpr (Archive::kIsReading) {
    if (PendingHouse < HOUSE_NONE || PendingHouse >= HOUSE_COUNT) {
      ar.Fail("invalid pending placement owner");
    }
  }
}
template void DisplayClass::Serialize(ArchiveWriter&);
template void DisplayClass::Serialize(ArchiveReader&);

template <class Archive>
void RadarClass::Serialize(Archive& ar) {
  DisplayClass::Serialize(ar);
  bool exists = DoesRadarExist;
  bool active = IsRadarActive;
  bool zoomed = IsZoomed;
  bool names = IsPlayerNames;
  ar(exists, active, zoomed, names, ZoomFactor, RadarX, RadarY, RadarCell,
     RadarCellWidth, RadarCellHeight, BaseX, BaseY, RadarWidth, RadarHeight);
  if constexpr (Archive::kIsReading) {
    DoesRadarExist = exists;
    IsRadarActive = active;
    IsZoomed = zoomed;
    IsPlayerNames = names;
  }
}
template void RadarClass::Serialize(ArchiveWriter&);
template void RadarClass::Serialize(ArchiveReader&);

template <class Archive>
void SidebarClass::StripClass::Serialize(Archive& ar) {
  bool building = IsBuilding;
  ar(building, Flasher, TopIndex, BuildableCount);
  if constexpr (Archive::kIsReading) {
    if (BuildableCount < 0 || BuildableCount > kMaxBuildables || TopIndex < 0 ||
        TopIndex >= kMaxBuildables) {
      ar.Fail("invalid sidebar count or scroll position");
      return;
    }
    IsBuilding = building;
    for (auto& item : Buildables) {
      item = {0, RTTI_NONE, -1};
    }
  }
  for (int i = 0; i < BuildableCount; ++i) {
    auto& item = base::At(Buildables, i);
    ar(item.BuildableID, item.BuildableType, item.Factory);
    if constexpr (Archive::kIsReading) {
      if (item.Factory < -1 || item.Factory >= Factories.Length() ||
          item.BuildableID < 0 || item.BuildableType <= RTTI_NONE ||
          item.BuildableType > RTTI_SPECIAL) {
        ar.Fail("invalid sidebar buildable");
      }
    }
  }
}
template void SidebarClass::StripClass::Serialize(ArchiveWriter&);
template void SidebarClass::StripClass::Serialize(ArchiveReader&);

template <class Archive>
void SidebarClass::Serialize(Archive& ar) {
  RadarClass::Serialize(ar);
  bool active = IsSidebarActive;
  ar(active, Column);
  if constexpr (Archive::kIsReading) {
    IsSidebarActive = active;
  }
}
template void SidebarClass::Serialize(ArchiveWriter&);
template void SidebarClass::Serialize(ArchiveReader&);

void GScreenClass::ResetTransientUiState() {
  IsScreenToRedraw = true;
  IsToUpdate = true;
}

void DisplayClass::ResetTransientUiState() {
  GScreenClass::ResetTransientUiState();
  PendingObject = nullptr;
  CursorSize = {};
  ProximityCheck = false;
  IsDisplayToRedraw = true;
  IsRepairMode = false;
  IsSellMode = false;
  IsTargettingMode = 0;
  IsRubberBand = false;
  IsTentative = false;
  IsShadowPresent = false;
  BandX = BandY = NewX = NewY = 0;
  std::ranges::fill(CellRedraw, true);
}

void RadarClass::ResetTransientUiState() {
  DisplayClass::ResetTransientUiState();
  IsRadarToRedraw = true;
  RadarCursorRedraw = true;
  IsRadarActivating = false;
  IsRadarDeactivating = false;
  SpecialRadarFrame = 0;
  RadarAnimFrame = IsRadarActive ? kRadarActivatedFrame : 0;
  PixelPtr = 0;
  std::ranges::fill(PixelStack, 0);
}

void PowerClass::ResetTransientUiState() {
  RadarClass::ResetTransientUiState();
  IsPowerToRedraw = true;
  RecordedDrain = RecordedPower = -1;
  DesiredDrainHeight = DesiredPowerHeight = 0;
  DrainHeight = PowerHeight = DrainBounce = PowerBounce = 0;
  PowerDir = DrainDir = 0;
}

void SidebarClass::ResetTransientUiState() {
  PowerClass::ResetTransientUiState();
  IsSidebarToRedraw = true;
  IsRepairActive = IsUpgradeActive = IsDemolishActive = false;
  for (auto& column : Column) {
    column.IsToRedraw = true;
    column.IsScrolling = column.IsScrollingDown = false;
    column.Scroller = column.Slid = 0;
    column.Set_Stage(0);
    column.Set_Rate(0);
  }
}

void TabClass::ResetTransientUiState() {
  SidebarClass::ResetTransientUiState();
  Credits = CreditClass();
  IsTabToRedraw = true;
}

void HelpClass::ResetTransientUiState() {
  TabClass::ResetTransientUiState();
  HelpText = nullptr;
  IsRight = false;
  Cost = X = Y = DrawX = DrawY = Width = 0;
  Text = TXT_NONE;
  Color = kLtGrey;
  CountDownTimer.Set(0);
  base::At(OverlapList, 0) = REFRESH_EOL;
}

void ScrollClass::ResetTransientUiState() {
  HelpClass::ResetTransientUiState();
  Inertia = 0;
  // Autoscroll is a local preference, not saved-game state.
  Counter.Set(0);
}

void MouseClass::ResetTransientUiState() {
  ScrollClass::ResetTransientUiState();
  Override_Mouse_Shape(MOUSE_NORMAL, false);
  NormalMouseShape = MOUSE_NORMAL;
  Frame = 0;
  Timer.Set(0);
}


template <class Archive>
void MouseClass::Serialize(Archive& ar) {
  ar.Section(FourCC("MAPS"));
  ar(Theater);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || Theater < THEATER_DESERT || Theater >= THEATER_COUNT) {
      ar.Fail("invalid saved theater");
      return;
    }
    Init_Theater(Theater);
    TerrainTypeClass::Init(Theater);
    TemplateTypeClass::Init(Theater);
    OverlayTypeClass::Init(Theater);
    UnitTypeClass::Init(Theater);
    InfantryTypeClass::Init(Theater);
    BuildingTypeClass::Init(Theater);
    BulletTypeClass::Init(Theater);
    AnimTypeClass::Init(Theater);
    AircraftTypeClass::Init(Theater);
    SmudgeTypeClass::Init(Theater);
    // Init_Cells clears TotalValue, so it must precede the member reads.
    Init_Cells();
    for (CELL cell = 0; cell < MAP_CELL_TOTAL; ++cell) {
      CellTriggers[cell] = nullptr;
    }
  }
  SidebarClass::Serialize(ar);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok()) {
      return;
    }
    ResetTransientUiState();
  }
  ar.Section(FourCC("MCEL"));
  int32_t count = 0;
  if constexpr (!Archive::kIsReading) {
    for (CELL cell = 0; cell < MAP_CELL_TOTAL; ++cell) {
      if ((*this)[cell].Should_Save()) {
        ++count;
      }
    }
  }
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > MAP_CELL_TOTAL) {
      ar.Fail("invalid saved cell count");
      return;
    }
    CELL previous = -1;
    for (int i = 0; i < count; ++i) {
      CELL cell = 0;
      ar(cell);
      if (!ar.ok() || cell <= previous || cell >= MAP_CELL_TOTAL) {
        ar.Fail("invalid or duplicate saved cell index");
        return;
      }
      ar((*this)[cell]);
      if (!ar.ok()) {
        return;
      }
      previous = cell;
    }
    LastTheater = Theater;
  } else {
    for (CELL cell = 0; cell < MAP_CELL_TOTAL; ++cell) {
      if ((*this)[cell].Should_Save()) {
        ar(cell, (*this)[cell]);
      }
    }
  }
}
template void MouseClass::Serialize(ArchiveWriter&);
template void MouseClass::Serialize(ArchiveReader&);

bool MouseClass::Load(ArchiveReader& file) {
  Serialize(file);
  return file.ok();
}
bool MouseClass::Save(ArchiveWriter& file) {
  Serialize(file);
  return true;
}
