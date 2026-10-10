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

/* $Header:   F:\projects\c&c\vcs\code\mission.h_v   2.16   16 Oct 1995 16:45:46
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MISSION.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_OBJECTS_MISSION_H_
#define CNC_RED_ALERT_TD_OBJECTS_MISSION_H_

#include <string_view>

#include "base/enum_array.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "td/core/defines.h"

namespace td {

// The functions that carry out the missions, one for each mission or small
// group of them. MissionClass::AI() calls the one for the current mission
// when its timer has run out. Each returns the number of game frames to wait
// before it is called again, and keeps its place between calls in
// MissionClass::step().
//
// TechnoClass implements it, and its subclasses override the missions they
// can perform.
class MissionHandler {
 public:
  MissionHandler() = default;
  virtual ~MissionHandler();
  MissionHandler(const MissionHandler&) = delete;
  MissionHandler& operator=(const MissionHandler&) = delete;
  MissionHandler(MissionHandler&&) = delete;
  MissionHandler& operator=(MissionHandler&&) = delete;

  virtual int Mission_Sleep() = 0;
  virtual int Mission_Ambush() = 0;
  virtual int Mission_Attack() = 0;
  virtual int Mission_Capture() = 0;
  virtual int Mission_Guard() = 0;
  virtual int Mission_Guard_Area() = 0;
  virtual int Mission_Harvest() = 0;
  virtual int Mission_Hunt() = 0;
  virtual int Mission_Timed_Hunt() = 0;
  virtual int Mission_Move() = 0;
  virtual int Mission_Retreat() = 0;
  virtual int Mission_Return() = 0;
  virtual int Mission_Stop() = 0;
  virtual int Mission_Unload() = 0;
  virtual int Mission_Enter() = 0;
  virtual int Mission_Construction() = 0;
  virtual int Mission_Deconstruction() = 0;
  virtual int Mission_Repair() = 0;
  virtual int Mission_Missile() = 0;
};

// What an object has been ordered to do: the mission it is carrying out, the
// one queued to start when the object can change over (Commence()), the one
// set aside by an override, and how far the current mission's state machine
// has come. It also times the calls of the mission function.
//
// It does not know its owner. The owner passes itself to AI() as the
// MissionHandler whose functions do the work.
//
// Example:
//   mission.Assign(MISSION_ATTACK);
//   if (mission.Commence()) { ... }  // at the next cell centre
//   mission.AI(*this);                // every game frame
class MissionClass {
 public:
  // The name of `mission` as scenario files spell it, "None" for
  // MISSION_NONE, and the mission with that name, MISSION_NONE when there is
  // none. Case does not matter.
  static const char* Mission_Name(MissionType mission);
  static MissionType Mission_From_Name(std::string_view name);

  MissionClass();

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(current_, suspended_, queued_, step_, timer_);
    if constexpr (Archive::kIsReading) {
      for (const MissionType mission : {current_, suspended_, queued_}) {
        if (mission < MISSION_NONE || mission >= MISSION_COUNT) {
          ar.Fail("invalid saved mission");
        }
      }
    }
  }

  // The mission being carried out, MISSION_NONE before the first one starts.
  // CurrentOrQueued() is the one to ask what the object is up to.
  [[nodiscard]] MissionType current() const { return current_; }
  void set_current(MissionType mission) { current_ = mission; }
  // The mission set aside by Suspend(); MISSION_NONE when there is none.
  [[nodiscard]] MissionType suspended() const { return suspended_; }
  void set_suspended(MissionType mission) { suspended_ = mission; }
  // The mission that Commence() will start; MISSION_NONE when none waits.
  [[nodiscard]] MissionType queued() const { return queued_; }
  void set_queued(MissionType mission) { queued_ = mission; }
  // The step the current mission's state machine is at. Each mission
  // function gives the numbers its own meaning; Commence() starts at 0.
  [[nodiscard]] int step() const { return step_; }
  void set_step(int step) { step_ = static_cast<char>(step); }

  // The current mission, or the queued one while there is no current one.
  [[nodiscard]] MissionType CurrentOrQueued() const {
    return current_ == MISSION_NONE ? queued_ : current_;
  }

  // Queues `order` to start at the next Commence(), replacing what was
  // queued. MISSION_NONE and the current mission are ignored.
  void Assign(MissionType order);

  // Starts the queued mission at step 0, to run on the next AI(). Returns
  // false, changing nothing, when no mission is queued.
  bool Commence();

  // Makes `mission` the current one at once and drops the queued one. The
  // step and the timer are left alone.
  void Set(MissionType mission) {
    current_ = mission;
    queued_ = MISSION_NONE;
  }

  // Sets the queued mission aside, or the current one when none is queued,
  // for the owner to bring back after an override.
  void Suspend() { suspended_ = queued_ != MISSION_NONE ? queued_ : current_; }

  // Makes the mission function due on the next AI().
  void ShortenTimer() { timer_.Set(0); }

  // Call once a game frame. When the timer has run out it calls the function
  // of `handler` for the current mission and waits the frames it returns.
  void AI(MissionHandler& handler);

 private:
  MissionType current_ = MISSION_NONE;
  MissionType suspended_ = MISSION_NONE;
  MissionType queued_ = MISSION_NONE;
  char step_ = 0;

  // Counts down to the next call of the mission function.
  engine::platform::Timer<game::FrameTickSource> timer_;

  // The mission names, as scenario files spell them.
  static base::EnumArray<MissionType, const char*, kMissionCount> Missions;
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_OBJECTS_MISSION_H_
