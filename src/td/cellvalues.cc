// Cell defaults and sparse-save selection, independent of the game session.
#include "base/array.h"
#include "td/cell.h"
#include "td/defines.h"

CellClass::CellClass() { Flag.Composite = 0; }

void CellClass::Reset() {
  IsPlot = false;
  IsCursorHere = false;
  IsMapped = false;
  IsVisible = false;
  IsTrigger = false;
  IsWaypoint = false;
  IsRadarCursor = false;
  IsFlagged = false;
  TType = TEMPLATE_NONE;
  TIcon = 0;
  Overlay = OVERLAY_NONE;
  OverlayData = 0;
  Smudge = SMUDGE_NONE;
  SmudgeData = 0;
  Owner = HOUSE_NONE;
  InfType = HOUSE_NONE;
  OccupierPtr = nullptr;
  base::At(Overlappers, 0) = nullptr;
  base::At(Overlappers, 1) = nullptr;
  base::At(Overlappers, 2) = nullptr;
  Flag.Composite = 0;
  Land = LAND_CLEAR;
}

bool CellClass::Should_Save() const {
  return IsPlot || IsCursorHere || IsMapped || IsVisible || IsTrigger ||
         IsWaypoint || IsRadarCursor || IsFlagged || TType != TEMPLATE_NONE ||
         TIcon != 0 || Overlay != OVERLAY_NONE || OverlayData != 0 ||
         Smudge != SMUDGE_NONE || SmudgeData != 0 || Owner != HOUSE_NONE ||
         InfType != HOUSE_NONE || OccupierPtr != nullptr ||
         base::At(Overlappers, 0) != nullptr ||
         base::At(Overlappers, 1) != nullptr ||
         base::At(Overlappers, 2) != nullptr || Flag.Composite != 0 ||
         Land != LAND_CLEAR;
}
