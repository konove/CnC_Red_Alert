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
#include "ra/externs.h"
#include "ra/house.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/selection.h"
#include "ra/session.h"
#include "ra/target.h"
#include "ra/vector_dynamic.h"
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
  if (Session.Record) {
    // Save the map's location
    Session.RecordFile.WriteObject(Map.DesiredTacticalCoord);

    // Save the current object list count
    count = static_cast<int>(CurrentObject.Count());
    Session.RecordFile.WriteObject(count);

    // Save a CRC of the selected-object list.
    sum = 0;
    for (int i = 0; i < count; i++) {
      ltgt = static_cast<uint32_t>(CurrentObject.at(i)->As_Target());
      sum += ltgt;
    }
    Session.RecordFile.WriteObject(sum);

    // Save all selected objects.
    for (int i = 0; i < count; i++) {
      tgt = CurrentObject.at(i)->As_Target();
      Session.RecordFile.WriteObject(tgt);
    }

    // Save team-selection and formation events
    Session.RecordFile.WriteObject(TeamEvent);
    Session.RecordFile.WriteObject(TeamNumber);
    Session.RecordFile.WriteObject(FormationEvent);
    Session.RecordFile.WriteObject(TeamMaxSpeed);
    Session.RecordFile.WriteObject(TeamSpeed);
    Session.RecordFile.WriteObject(FormMove);
    Session.RecordFile.WriteObject(FormSpeed);
    Session.RecordFile.WriteObject(FormMaxSpeed);
    TeamEvent = 0;
    TeamNumber = 0;
    FormationEvent = 0;
  }

  // Play back a game ("attract" mode)
  if (Session.Play) {
    // Read & set the map's location.
    if (Session.RecordFile.ReadObject(coord) &&
        coord != Map.DesiredTacticalCoord) {
      Map.Set_Tactical_Position(coord);
    }

    if (Session.RecordFile.ReadObject(count)) {
      uint32_t sum2 = 0;
      // Compute a CRC of the current object-selection list.
      sum = 0;
      for (int i = 0; i < CurrentObject.Count(); i++) {
        ltgt = static_cast<uint32_t>(CurrentObject.at(i)->As_Target());
        sum += ltgt;
      }

      // Load the CRC of the objects on disk; if it doesn't match, select
      // all objects as they're loaded.
      Session.RecordFile.ReadObject(sum2);
      if (sum2 != sum) {
        Unselect_All();
      }

      AllowVoice = true;

      for (int i = 0; i < count; ++i) {
        if (Session.RecordFile.ReadObject(tgt)) {
          ObjectClass* obj = As_Object(tgt);
          if (obj != nullptr && sum2 != sum) {
            obj->Select();
            AllowVoice = false;
          }
        }
      }

      AllowVoice = true;
    }

    // Save team-selection and formation events
    Session.RecordFile.ReadObject(TeamEvent);
    Session.RecordFile.ReadObject(TeamNumber);
    Session.RecordFile.ReadObject(FormationEvent);
    if (TeamEvent) {
      Handle_Team(TeamNumber, TeamEvent - 1);
    }
    if (FormationEvent) {
      Toggle_Formation();
    }

    Session.RecordFile.ReadObject(TeamMaxSpeed);
    Session.RecordFile.ReadObject(TeamSpeed);
    Session.RecordFile.ReadObject(FormMove);
    Session.RecordFile.ReadObject(FormSpeed);
    Session.RecordFile.ReadObject(FormMaxSpeed);
    // The map isn't drawn in playback mode, so draw it here.
    Map.Render();
  }
}
