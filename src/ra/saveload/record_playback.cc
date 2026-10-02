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

#include "ra/saveload/record_playback.h"

#include <cstdint>
#include <iterator>

#include "base/array.h"
#include "engine/stream/byte_stream.h"
#include "ra/core/defines.h"
#include "ra/core/target.h"
#include "ra/editor/mapedit.h"
#include "ra/map/selection.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/startup/game_state.h"

namespace ra {

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
  Target tgt = Target{};
  Coordinate coord{};
  uint32_t sum = 0;
  uint32_t ltgt = 0;

  // Null only outside a recorded or played-back game.
  ByteStream* const record = TheSession().record_stream().get();

  // Record a game
  if (TheSession().Record && record != nullptr) {
    // Save the map's location
    record->WriteObject(TheMap().DesiredTacticalCoord);

    // Save the current object list count
    count = static_cast<int>(std::ssize(TheWorld().current_object()));
    record->WriteObject(count);

    // Save a CRC of the selected-object list.
    sum = 0;
    for (int i = 0; i < count; i++) {
      ltgt = static_cast<uint32_t>(
          base::At(TheWorld().current_object(), i)->As_Target().packed());
      sum += ltgt;
    }
    record->WriteObject(sum);

    // Save all selected objects.
    for (int i = 0; i < count; i++) {
      tgt = base::At(TheWorld().current_object(), i)->As_Target();
      record->WriteObject(tgt);
    }

    // Save team-selection and formation events
    record->WriteObject(TeamEvent);
    record->WriteObject(TeamNumber);
    record->WriteObject(FormationEvent);
    record->WriteObject(TheWorld().team_max_speed());
    record->WriteObject(TheWorld().team_speed());
    record->WriteObject(TheWorld().form_move());
    record->WriteObject(TheWorld().form_speed());
    record->WriteObject(TheWorld().form_max_speed());
    TeamEvent = 0;
    TeamNumber = 0;
    FormationEvent = 0;
  }

  // Play back a game ("attract" mode)
  if (TheSession().Play && record != nullptr) {
    // Read & set the map's location.
    if (record->ReadObject(coord) && coord != TheMap().DesiredTacticalCoord) {
      TheMap().Set_Tactical_Position(coord);
    }

    if (record->ReadObject(count)) {
      uint32_t sum2 = 0;
      // Compute a CRC of the current object-selection list.
      sum = 0;
      for (int i = 0; i < std::ssize(TheWorld().current_object()); i++) {
        ltgt = static_cast<uint32_t>(
            base::At(TheWorld().current_object(), i)->As_Target().packed());
        sum += ltgt;
      }

      // Load the CRC of the objects on disk; if it doesn't match, select
      // all objects as they're loaded.
      record->ReadObject(sum2);
      if (sum2 != sum) {
        Unselect_All();
      }

      TheGameState().allow_voice() = true;

      for (int i = 0; i < count; ++i) {
        if (record->ReadObject(tgt)) {
          ObjectClass* obj = tgt.AsObject();
          if (obj != nullptr && sum2 != sum) {
            obj->Select();
            TheGameState().allow_voice() = false;
          }
        }
      }

      TheGameState().allow_voice() = true;
    }

    // Save team-selection and formation events
    record->ReadObject(TeamEvent);
    record->ReadObject(TeamNumber);
    record->ReadObject(FormationEvent);
    if (TeamEvent) {
      Handle_Team(TeamNumber, TeamEvent - 1);
    }
    if (FormationEvent) {
      Toggle_Formation();
    }

    record->ReadObject(TheWorld().team_max_speed());
    record->ReadObject(TheWorld().team_speed());
    record->ReadObject(TheWorld().form_move());
    record->ReadObject(TheWorld().form_speed());
    record->ReadObject(TheWorld().form_max_speed());
    // The map isn't drawn in playback mode, so draw it here.
    TheMap().Render();
  }
}

}  // namespace ra
