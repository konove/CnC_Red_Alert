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

// File: Saves and replays per-frame view and selection state for recorded
// games.

#include "ra/record_playback.h"

#include <cstdint>

#include "ra/defines.h"
#include "ra/game_state.h"
#include "ra/house.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/selection.h"
#include "ra/session.h"
#include "ra/target.h"
#include "ra/vector_dynamic.h"
#include "ra/world.h"
#include "tech/game_file.h"

// Recording state for the current frame, consumed and cleared by
// Do_Record_Playback().
static char TeamEvent = 0;       // 0 = no event, 1,2,3 = team event type
static char TeamNumber = 0;      // which team was selected? (1-9)
static char FormationEvent = 0;  // 0 = no event, 1 = formation was toggled

void RecordTeamEvent(const int team, const int action) {
  TeamNumber = static_cast<char>(team);
  TeamEvent = static_cast<char>(action + 1);
}

void RecordFormationEvent() { FormationEvent = 1; }

void ResetRecordedEvents() {
  TeamEvent = 0;
  TeamNumber = 0;
  FormationEvent = 0;
}

void Do_Record_Playback() {
  int count = 0;
  TARGET tgt = 0;
  COORDINATE coord = 0;
  uint32_t sum = 0;
  uint32_t ltgt = 0;

  // Record a game
  if (TheSession().Record) {
    // Save the map's location
    TheSession().RecordFile.WriteObject(TheMap().DesiredTacticalCoord);

    // Save the current object list count
    count = static_cast<int>(TheWorld().current_object().Count());
    TheSession().RecordFile.WriteObject(count);

    // Save a CRC of the selected-object list.
    sum = 0;
    for (int i = 0; i < count; i++) {
      ltgt =
          static_cast<uint32_t>(TheWorld().current_object().at(i)->As_Target());
      sum += ltgt;
    }
    TheSession().RecordFile.WriteObject(sum);

    // Save all selected objects.
    for (int i = 0; i < count; i++) {
      tgt = TheWorld().current_object().at(i)->As_Target();
      TheSession().RecordFile.WriteObject(tgt);
    }

    // Save team-selection and formation events
    TheSession().RecordFile.WriteObject(TeamEvent);
    TheSession().RecordFile.WriteObject(TeamNumber);
    TheSession().RecordFile.WriteObject(FormationEvent);
    TheSession().RecordFile.WriteObject(TheWorld().team_max_speed());
    TheSession().RecordFile.WriteObject(TheWorld().team_speed());
    TheSession().RecordFile.WriteObject(TheWorld().form_move());
    TheSession().RecordFile.WriteObject(TheWorld().form_speed());
    TheSession().RecordFile.WriteObject(TheWorld().form_max_speed());
    TeamEvent = 0;
    TeamNumber = 0;
    FormationEvent = 0;
  }

  // Play back a game ("attract" mode)
  if (TheSession().Play) {
    // Read & set the map's location.
    if (TheSession().RecordFile.ReadObject(coord) &&
        coord != TheMap().DesiredTacticalCoord) {
      TheMap().Set_Tactical_Position(coord);
    }

    if (TheSession().RecordFile.ReadObject(count)) {
      uint32_t sum2 = 0;
      // Compute a CRC of the current object-selection list.
      sum = 0;
      for (int i = 0; i < TheWorld().current_object().Count(); i++) {
        ltgt = static_cast<uint32_t>(
            TheWorld().current_object().at(i)->As_Target());
        sum += ltgt;
      }

      // Load the CRC of the objects on disk; if it doesn't match, select
      // all objects as they're loaded.
      TheSession().RecordFile.ReadObject(sum2);
      if (sum2 != sum) {
        Unselect_All();
      }

      TheGameState().allow_voice() = true;

      for (int i = 0; i < count; ++i) {
        if (TheSession().RecordFile.ReadObject(tgt)) {
          ObjectClass* obj = As_Object(tgt);
          if (obj != nullptr && sum2 != sum) {
            obj->Select();
            TheGameState().allow_voice() = false;
          }
        }
      }

      TheGameState().allow_voice() = true;
    }

    // Save team-selection and formation events
    TheSession().RecordFile.ReadObject(TeamEvent);
    TheSession().RecordFile.ReadObject(TeamNumber);
    TheSession().RecordFile.ReadObject(FormationEvent);
    if (TeamEvent) {
      Handle_Team(TeamNumber, TeamEvent - 1);
    }
    if (FormationEvent) {
      Toggle_Formation();
    }

    TheSession().RecordFile.ReadObject(TheWorld().team_max_speed());
    TheSession().RecordFile.ReadObject(TheWorld().team_speed());
    TheSession().RecordFile.ReadObject(TheWorld().form_move());
    TheSession().RecordFile.ReadObject(TheWorld().form_speed());
    TheSession().RecordFile.ReadObject(TheWorld().form_max_speed());
    // The map isn't drawn in playback mode, so draw it here.
    TheMap().Render();
  }
}
