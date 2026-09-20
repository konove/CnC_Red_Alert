// File: World, the scenario state Red Alert saves and loads.

#ifndef CNC_RED_ALERT_RA_WORLD_H_
#define CNC_RED_ALERT_RA_WORLD_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "base/installed.h"
#include "ra/base.h"
#include "ra/carry.h"
#include "ra/defines.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/trigger.h"
#include "ra/vector_dynamic.h"
#include "ra/vortex.h"

class HouseClass;
class ObjectClass;

// The state of the scenario being played: the map, the scenario record, the
// house the player is, the layers of objects, the triggers and the loose
// flags the mission logic keeps. Nearly all of it is written to a saved
// game, so the order it is serialized in must not change.
//
// Game owns the one World, declared after GameClock and the heaps it points
// into. Everything else reaches it through TheWorld(), or through the
// shorthands TheMap(), TheScenario() and ThePlayer() for the three pieces
// the game asks for most.
//
// Example:
//   TheMap().Flag_To_Redraw(true);
class World {
 public:
  // The number of teams the player can assign with the number keys.
  static constexpr int kTeamCount = 10;

  // How many objects one scenario may rename.
  static constexpr int kNameOverrideCount = 25;

  // Most of these hand out a reference: the state is read and written all
  // over the simulation, and a getter and setter pair for each would only
  // spell the same thing longer.

  // Zeroes the home waypoint, which MapEditClass's constructor did while
  // the scenario was a global it could reach.
  World() { scen_.Waypoint[ScenarioClass::kHomeWaypoint] = 0; }
  ~World() = default;

  World(const World&) = delete;
  World& operator=(const World&) = delete;
  World(World&&) = delete;
  World& operator=(World&&) = delete;

  // The map: the cells, the layers of objects drawn on them, and the
  // display, sidebar and radar built on top.
  MapEditClass& map() ABSL_ATTRIBUTE_LIFETIME_BOUND { return map_; }

  // What the scenario asked for and how far it has got.
  ScenarioClass& scen() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scen_; }

  // The house the player is playing. Null until the scenario is read.
  HouseClass*& player() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_; }

  // Drives one game frame of AI over everything that thinks.
  LogicClass& logic() ABSL_ATTRIBUTE_LIFETIME_BOUND { return logic_; }

  // The computer's record of what its base should look like.
  BaseClass& base() ABSL_ATTRIBUTE_LIFETIME_BOUND { return base_; }

  // What the score screen shows when the scenario ends.
  ScoreClass& score() ABSL_ATTRIBUTE_LIFETIME_BOUND { return score_; }

  // The chronosphere's lightning storm.
  ChronalVortexClass& chronal_vortex() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return chronal_vortex_;
  }

  // The objects the player has selected, in selection order.
  DynamicVectorClass<ObjectClass*>& current_object()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return current_object_;
  }

  // The triggers that fire on a game event rather than on a cell or an
  // object, and the id counters that name new ones.
  DynamicVectorClass<TriggerClass*>& logic_triggers()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return logic_triggers_;
  }
  DynamicVectorClass<TriggerClass*>& map_triggers()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return map_triggers_;
  }
  base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>>&
  house_triggers() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_triggers_;
  }
  int& map_trigger_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return map_trigger_id_;
  }
  int& logic_trigger_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return logic_trigger_id_;
  }

  // What the scenario leaves behind for the next one in the campaign.
  std::vector<CarryoverClass>& carryover() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return carryover_;
  }

  // The speed the team being built moves at, filled in while its members
  // are gathered, and the same for a formation.
  std::array<MPHType, kTeamCount>& team_max_speed()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_max_speed_;
  }
  std::array<SpeedType, kTeamCount>& team_speed()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_speed_;
  }
  bool& form_move() ABSL_ATTRIBUTE_LIFETIME_BOUND { return form_move_; }
  SpeedType& form_speed() ABSL_ATTRIBUTE_LIFETIME_BOUND { return form_speed_; }
  MPHType& form_max_speed() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return form_max_speed_;
  }

  // Tanya's fate, which carries from one Allied mission to the next.
  bool& is_tanya_dead() ABSL_ATTRIBUTE_LIFETIME_BOUND { return is_tanya_dead_; }
  bool& save_tanya() ABSL_ATTRIBUTE_LIFETIME_BOUND { return save_tanya_; }

  // Whether a time quake is running this frame, whether one is due, and
  // where it is centred.
  bool& time_quake() ABSL_ATTRIBUTE_LIFETIME_BOUND { return time_quake_; }
  bool& pending_time_quake() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return pending_time_quake_;
  }
  TARGET& time_quake_center() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return time_quake_center_;
  }

  // Non-zero while the scenario is being read, which suppresses the side
  // effects that placing an object would otherwise have.
  int& scenario_init() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scenario_init_; }

  // The checksum of the scenario file, compared between players.
  uint32_t& scenario_crc() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scenario_crc_;
  }

  // Whether this is one of the giant ant missions, which the scenario name
  // gives away, and whether a submarine surfaced this frame so the sonar
  // pulse can reveal it.
  bool& ants_enabled() ABSL_ATTRIBUTE_LIFETIME_BOUND { return ants_enabled_; }
  bool& auto_sonar_pulse() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return auto_sonar_pulse_;
  }

  // Names the scenario gives particular objects in place of their usual
  // ones. AbstractTypeClass::Full_Name() returns a negative index into
  // these when an object has one, and Text_String() looks it up here.
  // A slot is free when its id is zero.
  auto& name_override() ABSL_ATTRIBUTE_LIFETIME_BOUND { return name_override_; }
  auto& name_override_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return name_override_id_;
  }

  // What the scenario's synchronized random generator was started from.
  // Every machine in a multiplayer game seeds from this one value, so the
  // saved game and the recording both store it.
  int& seed() ABSL_ATTRIBUTE_LIFETIME_BOUND { return seed_; }

  // How much of the tech tree the scenario allows, and the house the
  // command line asked the player to be.
  int& build_level() ABSL_ATTRIBUTE_LIFETIME_BOUND { return build_level_; }
  HousesType& whom() ABSL_ATTRIBUTE_LIFETIME_BOUND { return whom_; }

  // The cell the map editor is working on, and the theater whose art is
  // loaded, which tells the loaders when it changes.
  CELL& current_cell() ABSL_ATTRIBUTE_LIFETIME_BOUND { return current_cell_; }
  TheaterType& last_theater() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return last_theater_;
  }

  // Whether the scenario file being read uses the newer INI layout.
  int& new_ini_format() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return new_ini_format_;
  }

 private:
  MapEditClass map_;
  ScenarioClass scen_;
  HouseClass* player_ = nullptr;
  LogicClass logic_;
  BaseClass base_;
  ScoreClass score_;
  ChronalVortexClass chronal_vortex_;

  DynamicVectorClass<ObjectClass*> current_object_;
  DynamicVectorClass<TriggerClass*> logic_triggers_;
  DynamicVectorClass<TriggerClass*> map_triggers_;
  base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>>
      house_triggers_;
  int map_trigger_id_ = 0;
  int logic_trigger_id_ = 0;

  std::vector<CarryoverClass> carryover_;

  std::array<MPHType, kTeamCount> team_max_speed_{};
  std::array<SpeedType, kTeamCount> team_speed_{};
  bool form_move_ = false;
  SpeedType form_speed_{};
  MPHType form_max_speed_{};

  bool is_tanya_dead_ = false;
  bool save_tanya_ = false;

  bool time_quake_ = false;
  bool pending_time_quake_ = false;
  TARGET time_quake_center_{};

  int scenario_init_ = 0;
  uint32_t scenario_crc_ = 0;
  std::array<std::string, kNameOverrideCount> name_override_;
  std::array<int, kNameOverrideCount> name_override_id_{};

  int seed_ = 0;
  bool ants_enabled_ = false;
  bool auto_sonar_pulse_ = false;
  int build_level_ = 10;
  HousesType whom_{};
  CELL current_cell_ = 0;
  TheaterType last_theater_ = THEATER_NONE;
  int new_ini_format_ = 0;
};

// Returns the World that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline World& TheWorld() { return base::Installed<World>::Get(); }

// Shorthands for the three pieces of the world the game reads most.
inline MapEditClass& TheMap() { return TheWorld().map(); }
inline ScenarioClass& TheScenario() { return TheWorld().scen(); }
inline HouseClass*& ThePlayer() { return TheWorld().player(); }

#endif  // CNC_RED_ALERT_RA_WORLD_H_
