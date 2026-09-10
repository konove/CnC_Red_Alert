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

// Field-wise map saves preserve the constructed UI and cell array. Heap
// pointers resolve to slots without dereferencing objects that load later.

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/conquer.h"
#include "ra/crate.h"
#include "ra/credits.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/externs.h"
#include "ra/gscreen.h"
#include "ra/help.h"
#include "ra/jshell.h"
#include "ra/map.h"
#include "ra/mouse.h"
#include "ra/object.h"
#include "ra/power.h"
#include "ra/radar.h"
#include "ra/scenario.h"
#include "ra/scroll.h"
#include "ra/serialize.h"
#include "ra/sidebar.h"
#include "ra/tab.h"
#include "ra/type.h"
#include "sdllib/wwstd.h"
#include "tech/archive.h"
#include "tech/ftimer.h"
#include "tech/pipe.h"
#include "tech/straw.h"

// ID comes from the sparse map index. Movement zones are rebuilt by
// Post_Load_Game(), so they do not make an otherwise empty cell worth saving.
bool CellClass::Should_Save() const {
  for (const ObjectClass* overlapper : Overlappers) {
    if (overlapper != nullptr) {
      return true;
    }
  }
  return IsPlot || IsCursorHere || IsMapped || IsVisible || IsWaypoint ||
         IsRadarCursor || IsFlagged || IsToShroud || Jammed != 0 ||
         Trigger.Is_Valid() || TType != TEMPLATE_NONE || TIcon != 0 ||
         Overlay != OVERLAY_NONE || OverlayData != 0 ||
         Smudge != SMUDGE_NONE || SmudgeData != 0 || Owner != HOUSE_NONE ||
         InfType != HOUSE_NONE || OccupierPtr != nullptr ||
         Flag.Composite != 0 || Land != LAND_CLEAR;
}

template <class Archive>
void CellClass::Serialize(Archive& ar) {
  uint8_t flags = static_cast<uint8_t>(
      IsPlot | (IsCursorHere << 1) | (IsMapped << 2) | (IsVisible << 3) |
      (IsWaypoint << 4) | (IsRadarCursor << 5) | (IsFlagged << 6) |
      (IsToShroud << 7));
  ar(flags, Jammed, Trigger, TType, TIcon, Overlay, OverlayData, Smudge,
     SmudgeData, Owner, InfType, ObjectPtr(OccupierPtr));
  if constexpr (Archive::kIsReading) {
    IsPlot = (flags & 1) != 0;
    IsCursorHere = (flags & 2) != 0;
    IsMapped = (flags & 4) != 0;
    IsVisible = (flags & 8) != 0;
    IsWaypoint = (flags & 16) != 0;
    IsRadarCursor = (flags & 32) != 0;
    IsFlagged = (flags & 64) != 0;
    IsToShroud = (flags & 128) != 0;
    std::fill(std::begin(Overlappers), std::end(Overlappers), nullptr);
  }
  int32_t count = static_cast<int32_t>(kOverlapperCount);
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > kOverlapperCount) {
      ar.Fail("invalid cell overlapper count");
      return;
    }
  }
  for (int i = 0; i < count; ++i) {
    ar(ObjectPtr(Overlappers[i]));
  }
  ar(Flag.Composite, Land);
}
template void CellClass::Serialize(ArchiveWriter&);
template void CellClass::Serialize(ArchiveReader&);

template <class Archive>
void MapClass::Serialize(Archive& ar) {
  ar(MapCellX, MapCellY, MapCellWidth, MapCellHeight, TotalValue,
     TiberiumGrowth, TiberiumGrowthCount, TiberiumGrowthExcess,
     TiberiumSpread, TiberiumSpreadCount, TiberiumSpreadExcess, TiberiumScan);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || MapCellX < 0 || MapCellY < 0 || MapCellWidth <= 0 ||
        MapCellHeight <= 0 || MapCellWidth > MAP_CELL_W ||
        MapCellHeight > MAP_CELL_H || MapCellX > MAP_CELL_W - MapCellWidth ||
        MapCellY > MAP_CELL_H - MapCellHeight || TiberiumGrowthCount < 0 ||
        TiberiumGrowthCount > std::ssize(TiberiumGrowth) ||
        TiberiumSpreadCount < 0 ||
        TiberiumSpreadCount > std::ssize(TiberiumSpread) || TiberiumScan < 0 ||
        TiberiumScan >= MAP_CELL_TOTAL) {
      ar.Fail("invalid map dimensions or ore scan state");
      return;
    }
    for (auto& crate : Crates) {
      crate.Init();
    }
  }

  int32_t count = 0;
  if constexpr (!Archive::kIsReading) {
    for (const auto& crate : Crates) {
      if (crate.Is_Valid()) {
        ++count;
      }
    }
  }
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > std::ssize(Crates)) {
      ar.Fail("invalid crate count");
      return;
    }
    int32_t previous = -1;
    for (int i = 0; i < count; ++i) {
      int32_t index = 0;
      ar(index);
      if (!ar.ok() || index <= previous || index >= std::ssize(Crates)) {
        ar.Fail("invalid or duplicate crate index");
        return;
      }
      ar(Crates[index]);
      if (!ar.ok() || !Crates[index].Is_Valid()) {
        ar.Fail("invalid saved crate");
        return;
      }
      previous = index;
    }
  } else {
    for (int32_t index = 0; index < std::ssize(Crates); ++index) {
      if (Crates[index].Is_Valid()) {
        ar(index, Crates[index]);
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
}
template void DisplayClass::Serialize(ArchiveWriter&);
template void DisplayClass::Serialize(ArchiveReader&);

template <class Archive>
void RadarClass::Serialize(Archive& ar) {
  DisplayClass::Serialize(ar);
  bool exists = DoesRadarExist;
  bool active = IsRadarActive;
  bool jammed = IsRadarJammed;
  bool zoomed = IsZoomed;
  bool names = IsPlayerNames;
  bool spy = IsHouseSpy;
  ar(exists, active, jammed, zoomed, names, spy, SpyingOn, ZoomFactor,
     RadarX, RadarY, RadarCell, RadarCellWidth, RadarCellHeight, BaseX,
     BaseY, RadarWidth, RadarHeight);
  if constexpr (Archive::kIsReading) {
    DoesRadarExist = exists;
    IsRadarActive = active;
    IsRadarJammed = jammed;
    IsZoomed = zoomed;
    IsPlayerNames = names;
    IsHouseSpy = spy;
  }
}
template void RadarClass::Serialize(ArchiveWriter&);
template void RadarClass::Serialize(ArchiveReader&);

template <class Archive>
void SidebarClass::StripClass::Serialize(Archive& ar) {
  bool building = IsBuilding;
  ar(building, Flasher, TopIndex, BuildableCount);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || BuildableCount < 0 || BuildableCount > kMaxBuildables ||
        TopIndex < 0 || TopIndex >= kMaxBuildables) {
      ar.Fail("invalid sidebar buildable count or scroll position");
      return;
    }
    IsBuilding = building;
    for (auto& item : Buildables) {
      item = {0, RTTI_NONE, -1};
    }
  }
  for (int i = 0; i < BuildableCount; ++i) {
    auto& item = Buildables[i];
    ar(item.BuildableID, item.BuildableType, item.Factory);
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
  IsToRedraw = true;
  IsToUpdate = true;
}

void DisplayClass::ResetTransientUiState() {
  GScreenClass::ResetTransientUiState();
  PendingObject = nullptr;
  CursorSize = nullptr;
  ProximityCheck = false;
  IsToRedraw = true;
  IsRepairMode = false;
  IsSellMode = false;
  IsTargettingMode = SPC_NONE;
  IsRubberBand = false;
  IsTentative = false;
  IsShadowPresent = false;
  BandX = BandY = NewX = NewY = 0;
  std::fill(CellRedraw.begin(), CellRedraw.end(), true);
}

void RadarClass::ResetTransientUiState() {
  DisplayClass::ResetTransientUiState();
  IsToRedraw = true;
  RadarCursorRedraw = true;
  IsRadarActivating = false;
  IsRadarDeactivating = false;
  IsPulseActive = false;
  RadarPulseFrame = 0;
  SpecialRadarFrame = 0;
  RadarAnimFrame = IsRadarActive ? RADAR_ACTIVATED_FRAME : 0;
  PixelPtr = 0;
  std::fill(std::begin(PixelStack), std::end(PixelStack), 0);
}

void PowerClass::ResetTransientUiState() {
  RadarClass::ResetTransientUiState();
  IsToRedraw = true;
  FlashTimer.Set(0);
  RecordedDrain = RecordedPower = -1;
  DesiredDrainHeight = DesiredPowerHeight = 0;
  DrainHeight = PowerHeight = DrainBounce = PowerBounce = 0;
  PowerDir = DrainDir = 0;
}

void SidebarClass::ResetTransientUiState() {
  PowerClass::ResetTransientUiState();
  IsToRedraw = true;
  IsRepairActive = IsUpgradeActive = IsDemolishActive = false;
  for (auto& column : Column) {
    column.IsToRedraw = true;
    column.IsScrolling = column.IsScrollingDown = false;
    column.Scroller = column.Slid = column.LastSlid = 0;
    column.Set_Stage(0);
    column.Set_Rate(0);
  }
}

void TabClass::ResetTransientUiState() {
  SidebarClass::ResetTransientUiState();
  Credits = CreditClass();
  IsToRedraw = true;
  FlasherTimer.Set(0);
  MoneyFlashTimer.Set(0);
}

void HelpClass::ResetTransientUiState() {
  TabClass::ResetTransientUiState();
  HelpText = nullptr;
  IsRight = false;
  Cost = X = Y = DrawX = DrawY = Width = 0;
  Text = TXT_NONE;
  Color = LTGREY;
  CountDownTimer.Set(0);
  OverlapList[0] = kRefreshEol;
}

void ScrollClass::ResetTransientUiState() {
  HelpClass::ResetTransientUiState();
  // Autoscroll is a local preference, not saved-game state.
  Inertia = 0;
  Counter.Set(0);
}

void MouseClass::ResetTransientUiState() {
  ScrollClass::ResetTransientUiState();
  Override_Mouse_Shape(MOUSE_NORMAL, false);
  NormalMouseShape = MOUSE_NORMAL;
  Frame = 0;
  AnimTimer.Set(0);
}

template <class Archive>
void MouseClass::Serialize(Archive& ar) {
  if constexpr (Archive::kIsReading) {
    LastTheater = THEATER_NONE;
    Reset_Theater_Shapes();
    Init_Theater(Scen.Theater);
    TerrainTypeClass::Init(Scen.Theater);
    TemplateTypeClass::Init(Scen.Theater);
    OverlayTypeClass::Init(Scen.Theater);
    UnitTypeClass::Init(Scen.Theater);
    InfantryTypeClass::Init(Scen.Theater);
    BuildingTypeClass::Init(Scen.Theater);
    BulletTypeClass::Init(Scen.Theater);
    AnimTypeClass::Init(Scen.Theater);
    AircraftTypeClass::Init(Scen.Theater);
    VesselTypeClass::Init(Scen.Theater);
    SmudgeTypeClass::Init(Scen.Theater);
    // Init_Cells also clears TotalValue, so do this before reading map state.
    Init_Cells();
  }
  SidebarClass::Serialize(ar);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok()) {
      return;
    }
    ResetTransientUiState();
  }

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
    LastTheater = Scen.Theater;
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

bool MouseClass::Load(Straw& file) {
  ArchiveReader reader(file);
  Serialize(reader);
  return reader.ok();
}

bool MouseClass::Save(Pipe& file) {
  ArchiveWriter writer(file);
  Serialize(writer);
  return true;
}
