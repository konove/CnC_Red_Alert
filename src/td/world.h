// File: World, the scenario state Tiberian Dawn saves and loads.

#ifndef CNC_RED_ALERT_TD_WORLD_H_
#define CNC_RED_ALERT_TD_WORLD_H_

#include <cstdint>

#include "absl/base/attributes.h"
#include "base/enum_array.h"
#include "base/installed.h"
#include "port/platform.h"
#include "td/base.h"
#include "td/defines.h"
#include "td/ftimer.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/score.h"
#include "td/trigger.h"
#include "td/vector.h"

class HouseClass;
class ObjectClass;

// The state of the scenario being played: the map, which scenario it is,
// the house the player is, the layers of objects, the triggers, the
// waypoints and what the mission leaves behind. Nearly all of it is written
// to a saved game, so the order it is serialized in must not change.
//
// Red Alert gathers most of this into a ScenarioClass; Tiberian Dawn keeps
// the fields loose, as the original did, so World holds them directly.
//
// Game owns the one World, declared after the heaps it points into.
// Everything else reaches it through TheWorld(), or the shorthands TheMap()
// and ThePlayer().
//
// Example:
//   TheMap().Flag_To_Redraw(true);
class World {
 public:
  // The number of remembered view positions the scenario stores.
  static constexpr int kViewCount = 4;

  // Most of these hand out a reference: the state is read and written all
  // over the simulation, and a getter and setter pair for each would only
  // spell the same thing longer.
  // Zeroes the home waypoint, which MapEditClass's constructor did while
  // the waypoints were a global it could reach.
  World() : score_() { waypoint_[kWayptHome] = 0; }
  ~World() = default;

  World(const World&) = delete;
  World& operator=(const World&) = delete;
  World(World&&) = delete;
  World& operator=(World&&) = delete;

  // The map: the cells, the layers of objects drawn on them, and the
  // display, sidebar and radar built on top.
  MapEditClass& map() ABSL_ATTRIBUTE_LIFETIME_BOUND { return map_; }

  // The house the player is playing. Null until the scenario is read.
  HouseClass*& player() ABSL_ATTRIBUTE_LIFETIME_BOUND { return player_; }

  // Drives one game frame of AI over everything that thinks.
  LogicClass& logic() ABSL_ATTRIBUTE_LIFETIME_BOUND { return logic_; }

  // The computer's record of what its base should look like.
  BaseClass& base() ABSL_ATTRIBUTE_LIFETIME_BOUND { return base_; }

  // What the score screen shows when the scenario ends.
  ScoreClass& score() ABSL_ATTRIBUTE_LIFETIME_BOUND { return score_; }

  // The objects the player has selected, in selection order.
  DynamicVectorClass<ObjectClass*>& current_object()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return current_object_;
  }

  // The triggers attached to cells, and those attached to a house.
  DynamicVectorClass<TriggerClass*>& cell_triggers()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return cell_triggers_;
  }
  base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>, kHouseCount>&
  house_triggers() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_triggers_;
  }

  // The cells the scenario names, for reinforcements and for the view the
  // player starts on.
  auto& waypoint() ABSL_ATTRIBUTE_LIFETIME_BOUND { return waypoint_; }
  auto& views() ABSL_ATTRIBUTE_LIFETIME_BOUND { return views_; }

  // Which scenario is being played, for which side and which way the
  // campaign branched.
  int& scenario() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scenario_; }
  ScenarioPlayerType& scen_player() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scen_player_;
  }
  ScenarioDirType& scen_dir() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scen_dir_;
  }
  auto& scenario_name() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scenario_name_; }

  // What the scenario leaves behind for the next one, and the checksum
  // players compare.
  int& carry_over_money() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return carry_over_money_;
  }
  int& carry_over_percent() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return carry_over_percent_;
  }
  uint32_t& scenario_crc() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scenario_crc_;
  }

  // Which variation of the scenario is being played, and the theater whose
  // art is loaded, which tells the loaders when it changes.
  ScenarioVarType& scen_var() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return scen_var_;
  }
  TheaterType& last_theater() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return last_theater_;
  }

  // The crates scattered over the map: how many there are, when the next
  // one appears, and whether the scenario makes more of them.
  int& crate_count() ABSL_ATTRIBUTE_LIFETIME_BOUND { return crate_count_; }
  TCountDownTimerClass& crate_timer() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return crate_timer_;
  }
  bool& crate_maker() ABSL_ATTRIBUTE_LIFETIME_BOUND { return crate_maker_; }

  // What the scenario's synchronized random generator was started from.
  // Every machine in a multiplayer game seeds from this one value.
  int& seed() ABSL_ATTRIBUTE_LIFETIME_BOUND { return seed_; }

  // How much of the tech tree the scenario allows, and the house the
  // command line asked the player to be.
  int& build_level() ABSL_ATTRIBUTE_LIFETIME_BOUND { return build_level_; }
  HousesType& whom() ABSL_ATTRIBUTE_LIFETIME_BOUND { return whom_; }

  // Non-zero while the scenario is being read, which suppresses the side
  // effects that placing an object would otherwise have.
  int& scenario_init() ABSL_ATTRIBUTE_LIFETIME_BOUND { return scenario_init_; }

  // The cell the map editor is working on.
  CELL& current_cell() ABSL_ATTRIBUTE_LIFETIME_BOUND { return current_cell_; }

  // The briefing and the movies the scenario names, and the theme that
  // plays between missions.
  auto& briefing_text() ABSL_ATTRIBUTE_LIFETIME_BOUND { return briefing_text_; }
  auto& intro_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return intro_movie_; }
  auto& brief_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return brief_movie_; }
  auto& action_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return action_movie_; }
  auto& win_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return win_movie_; }
  auto& lose_movie() ABSL_ATTRIBUTE_LIFETIME_BOUND { return lose_movie_; }
  ThemeType& transit_theme() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return transit_theme_;
  }

  // Frames left before the mission ends however it is going to, the
  // building a saboteur blew up, and whether the temple has been hit by
  // the ion cannon -- all watched by the mission triggers.
  int& end_count_down() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return end_count_down_;
  }
  StructType& sabotaged_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return sabotaged_type_;
  }
  bool& temple_ioned() ABSL_ATTRIBUTE_LIFETIME_BOUND { return temple_ioned_; }

 private:
  MapEditClass map_;
  HouseClass* player_ = nullptr;
  LogicClass logic_;
  BaseClass base_;
  ScoreClass score_;

  DynamicVectorClass<ObjectClass*> current_object_;
  DynamicVectorClass<TriggerClass*> cell_triggers_;
  base::EnumArray<HousesType, DynamicVectorClass<TriggerClass*>, kHouseCount>
      house_triggers_;

  CELL waypoint_[kWayptCount]{};
  CELL views_[kViewCount]{};

  int scenario_ = 0;
  ScenarioPlayerType scen_player_{};
  ScenarioDirType scen_dir_{};
  char scenario_name_[port::kMaxFname + port::kMaxExt]{};

  int carry_over_money_ = 0;
  int carry_over_percent_ = 0;
  uint32_t scenario_crc_ = 0;
  ScenarioVarType scen_var_{};
  TheaterType last_theater_ = THEATER_NONE;

  int crate_count_ = 0;
  TCountDownTimerClass crate_timer_;
  bool crate_maker_ = false;

  int seed_ = 0;
  int build_level_ = 3;
  HousesType whom_{};
  int scenario_init_ = 0;
  CELL current_cell_ = 0;

  char briefing_text_[512]{};
  char intro_movie_[port::kMaxFname + port::kMaxExt]{};
  char brief_movie_[port::kMaxFname + port::kMaxExt]{};
  char action_movie_[port::kMaxFname + port::kMaxExt]{};
  char win_movie_[port::kMaxFname + port::kMaxExt]{};
  char lose_movie_[port::kMaxFname + port::kMaxExt]{};
  ThemeType transit_theme_ = THEME_NONE;

  int end_count_down_ = 0;
  StructType sabotaged_type_{};
  bool temple_ioned_ = false;
};

// Returns the World that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline World& TheWorld() { return base::Installed<World>::Get(); }

// Shorthands for the two pieces of the world the game reads most.
inline MapEditClass& TheMap() { return TheWorld().map(); }
inline HouseClass*& ThePlayer() { return TheWorld().player(); }

#endif  // CNC_RED_ALERT_TD_WORLD_H_
