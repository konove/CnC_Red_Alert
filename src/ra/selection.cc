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

// File: Selecting, grouping and forming up the player's units.

#include "ra/selection.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>

#include "engine/base/array.h"
#include "ra/aircraft.h"
#include "ra/ccptr.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/foot.h"
#include "ra/game_state.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/inline.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/record_playback.h"
#include "ra/session.h"
#include "ra/target.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "ra/vessel.h"
#include "ra/world.h"

// Unselect() removes the object from CurrentObject, so index 0 is always the
// next one to drop and the count shrinks on every pass.
void Unselect_All() {
  while (TheWorld().current_object().Count()) {
    TheWorld().current_object().at(0)->Unselect();
  }
}

void Toggle_Formation() {
  // kNoGroup means "no grouped unit found yet". It must be compared against
  // rather than -1: Group is an unsigned char, so an ungrouped unit reads back
  // as 255 and would otherwise be taken for a valid group number and used to
  // index the ten-entry TeamSpeed/TeamMaxSpeed arrays.
  int team = kNoGroup;
  // Seeded inverted -- min at the largest possible value, max at the smallest
  // -- so the first cell examined replaces both.
  int32_t minx = 0x7FFFFFFFL;
  int32_t miny = 0x7FFFFFFFL;
  int32_t maxx = 0;
  int32_t maxy = 0;
  bool set_form = false;

  // Recording support
  if (TheSession().Record) {
    RecordFormationEvent();
  }

  // Find the first selected object that is a member of a team, and
  // register his group as the team we're using.  Once we find the team
  // number, update the 'set_form' flag to know whether we should be setting
  // the formation's offsets, or clearing them.  If they currently have
  // illegal offsets (kNoFormationOffset), then we're setting.
  //
  // The three passes are ordered units, infantry, vessels because a mixed
  // group takes its speed from whichever type is found first.
  for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
    const UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
    if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
        obj->IsSelected) {
      team = obj->Group;
      if (std::cmp_not_equal(team, kNoGroup)) {
        set_form = obj->XFormOffset == kNoFormationOffset;
        base::At(std::span(TheWorld().team_speed()), team) = SPEED_WHEEL;
        base::At(std::span(TheWorld().team_max_speed()), team) =
            MPH_LIGHT_SPEED;
        break;
      }
    }
  }
  if (std::cmp_equal(team, kNoGroup)) {
    for (int index = 0; index < TheObjectHeaps().infantry().Count(); index++) {
      const InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
          obj->IsSelected) {
        team = obj->Group;
        if (std::cmp_not_equal(team, kNoGroup)) {
          set_form = obj->XFormOffset == kNoFormationOffset;
          base::At(std::span(TheWorld().team_speed()), team) = SPEED_WHEEL;
          base::At(std::span(TheWorld().team_max_speed()), team) =
              MPH_LIGHT_SPEED;
          break;
        }
      }
    }
  }

  if (std::cmp_equal(team, kNoGroup)) {
    for (int index = 0; index < TheObjectHeaps().vessel().Count(); index++) {
      const VesselClass* obj = TheObjectHeaps().vessel().Ptr(index);
      if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
          obj->IsSelected) {
        team = obj->Group;
        if (std::cmp_not_equal(team, kNoGroup)) {
          set_form = obj->XFormOffset == kNoFormationOffset;
          base::At(std::span(TheWorld().team_speed()), team) = SPEED_WHEEL;
          base::At(std::span(TheWorld().team_max_speed()), team) =
              MPH_LIGHT_SPEED;
          break;
        }
      }
    }
  }

  if (std::cmp_equal(team, kNoGroup)) {
    return;
  }
  // Now that we have a team, let's go set (or clear) the formation offsets.
  for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
    UnitClass* obj = TheObjectHeaps().unit().Ptr(i);
    if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
        std::cmp_equal(obj->Group, team)) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        if (obj->Class->MaxSpeed <
            base::At(std::span(TheWorld().team_max_speed()), team)) {
          base::At(std::span(TheWorld().team_max_speed()), team) =
              obj->Class->MaxSpeed;
          base::At(std::span(TheWorld().team_speed()), team) =
              obj->Class->Speed;
        }
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
    InfantryClass* obj = TheObjectHeaps().infantry().Ptr(i);
    if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
        std::cmp_equal(obj->Group, team)) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        base::At(std::span(TheWorld().team_max_speed()), team) =
            std::min(obj->Class->MaxSpeed,
                     base::At(std::span(TheWorld().team_max_speed()), team));
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  for (int i = 0; i < TheObjectHeaps().vessel().Count(); i++) {
    VesselClass* obj = TheObjectHeaps().vessel().Ptr(i);
    if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
        std::cmp_equal(obj->Group, team)) {
      obj->Mark(MARK_CHANGE);
      if (set_form) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
        minx = std::min(xc, minx);
        maxx = std::max(xc, maxx);
        miny = std::min(yc, miny);
        maxy = std::max(yc, maxy);
        base::At(std::span(TheWorld().team_max_speed()), team) =
            std::min(obj->Class->MaxSpeed,
                     base::At(std::span(TheWorld().team_max_speed()), team));
      } else {
        obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
      }
    }
  }

  // All the units have been counted to find the bounding rectangle and
  // center of the formation, or to clear their offsets.  Now, if we're to
  // set them into formation, proceed to do so.  Otherwise, bail.
  //
  // Offsets are taken from where each unit already stands, so the formation
  // locks in the group's current shape rather than imposing a canned one.
  if (set_form) {
    const int center_x = (((maxx - minx) / 2) + minx);
    const int center_y = (((maxy - miny) / 2) + miny);

    for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
      UnitClass* obj = TheObjectHeaps().unit().Ptr(i);
      if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
          std::cmp_equal(obj->Group, team)) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = xc - center_x;
        obj->YFormOffset = yc - center_y;
      }
    }

    for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
      InfantryClass* obj = TheObjectHeaps().infantry().Ptr(i);
      if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
          std::cmp_equal(obj->Group, team)) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = xc - center_x;
        obj->YFormOffset = yc - center_y;
      }
    }

    for (int i = 0; i < TheObjectHeaps().vessel().Count(); i++) {
      VesselClass* obj = TheObjectHeaps().vessel().Ptr(i);
      if (obj && !obj->IsInLimbo && obj->House == ThePlayer() &&
          std::cmp_equal(obj->Group, team)) {
        const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
        const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));

        obj->XFormOffset = xc - center_x;
        obj->YFormOffset = yc - center_y;
      }
    }
  }
}

// Handles the player's numbered unit groups. See the declaration in
// selection.h.
//
// AllowVoice is cleared once something has been selected so that picking a
// group of ten units produces one acknowledgement rather than ten.
void Handle_Team(const int team, const int action) {
  // Recording support
  if (TheSession().Record) {
    RecordTeamEvent(team, action);
  }

  TheGameState().allow_voice() = true;
  switch (action) {
    // Toggle the team selection. If the team is selected, then merely unselect
    // it. If the team is not selected, then unselect all others before
    // selecting this team.
    case 3:
    case 0:

      // If a non team member is currently selected, then deselect all
      // objects before selecting this team.
      if (TheWorld().current_object().Count() &&
          (TheWorld().current_object().at(0)->Is_Foot() &&
           std::cmp_not_equal(
               dynamic_cast<FootClass*>(TheWorld().current_object().at(0))
                   ->Group,
               team))) {
        Unselect_All();
      }

      for (int index = 0; index < TheObjectHeaps().vessel().Count(); index++) {
        VesselClass* obj = TheObjectHeaps().vessel().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }

      // Center the map around the team if the ALT key was pressed too.
      if (action == 3) {
        TheMap().Center_Map();
        TheMap().Flag_To_Redraw(true);
      }
      break;

    // Additive selection of team.
    case 1:
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().vessel().Count(); index++) {
        VesselClass* obj = TheObjectHeaps().vessel().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House->IsPlayerControl) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      break;

    // Create the team.
    case 2: {
      // Seeded inverted so the first member examined replaces both bounds.
      int32_t minx = 0x7FFFFFFFL;
      int32_t miny = 0x7FFFFFFFL;
      int32_t maxx = 0;
      int32_t maxy = 0;
      base::At(std::span(TheWorld().team_speed()), team) = SPEED_WHEEL;
      base::At(std::span(TheWorld().team_max_speed()), team) = MPH_LIGHT_SPEED;
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            if (obj->Class->MaxSpeed <
                base::At(std::span(TheWorld().team_max_speed()), team)) {
              base::At(std::span(TheWorld().team_max_speed()), team) =
                  obj->Class->MaxSpeed;
              base::At(std::span(TheWorld().team_speed()), team) =
                  obj->Class->Speed;
            }
          }
        }
      }

      for (int index = 0; index < TheObjectHeaps().vessel().Count(); index++) {
        VesselClass* obj = TheObjectHeaps().vessel().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            if (obj->Class->MaxSpeed <
                base::At(std::span(TheWorld().team_max_speed()), team)) {
              base::At(std::span(TheWorld().team_max_speed()), team) =
                  obj->Class->MaxSpeed;
              base::At(std::span(TheWorld().team_speed()), team) =
                  obj->Class->Speed;
            }
          }
        }
      }

      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
            const int32_t xc = Cell_X(Coord_Cell(obj->Center_Coord()));
            const int32_t yc = Cell_Y(Coord_Cell(obj->Center_Coord()));
            minx = std::min(xc, minx);
            maxx = std::max(xc, maxx);
            miny = std::min(yc, miny);
            maxy = std::max(yc, maxy);
            base::At(std::span(TheWorld().team_max_speed()), team) = std::min(
                obj->Class->MaxSpeed,
                base::At(std::span(TheWorld().team_max_speed()), team));
          }
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
            obj->Mark(MARK_CHANGE);
          }
        }
      }

      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl &&
            std::cmp_equal(obj->Group, team) && obj->IsSelected) {
          // When a team is first created, they're created without a
          // formation offset, so they will not be created in
          // formation.  Later, if they're assigned a formation, the
          // XFormOffset & YFormOffset numbers will change to valid
          // offsets, and they'll move in formation.
          obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
        }
      }

      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House->IsPlayerControl) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = kNoGroup;
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
          }
          if (std::cmp_equal(obj->Group, team) && obj->IsSelected) {
            obj->XFormOffset = obj->YFormOffset = kNoFormationOffset;
          }
        }
      }
      break;
    }

    default:
      break;
  }
  TheGameState().allow_voice() = true;
}
