// How ObjectHeaps tears the heaps down and checks them.

#include "ra/core/object_heaps.h"

#include <string>
#include <utility>

#include "absl/strings/str_cat.h"
#include "game/core/object_heap.h"

namespace ra {

ObjectHeaps::~ObjectHeaps() {
  // The heaps free their buffers without running element destructors, and
  // the houses own their unit trackers. The units' and buildings'
  // destructors reach into the game through its globals, so they stay
  // unrun.
  house_.DestroyAll();
}

std::string ObjectHeaps::Validate() const {
  // Listed in the order the save file walks them, so a report reads the same
  // way as a save game does.
  const std::pair<const char*, std::string> reports[] = {
      {"aircraft", aircraft_.Validate()},
      {"anim", anim_.Validate()},
      {"building", building_.Validate()},
      {"bullet", bullet_.Validate()},
      {"factory", factory_.Validate()},
      {"house", house_.Validate()},
      {"infantry", infantry_.Validate()},
      {"overlay", overlay_.Validate()},
      {"smudge", smudge_.Validate()},
      {"team", team_.Validate()},
      {"team_type", team_type_.Validate()},
      {"tmplate", tmplate_.Validate()},
      {"terrain", terrain_.Validate()},
      {"trigger", trigger_.Validate()},
      {"unit", unit_.Validate()},
      {"vessel", vessel_.Validate()},
      {"trigger_type", trigger_type_.Validate()},
  };

  for (const auto& [name, trouble] : reports) {
    if (!trouble.empty()) {
      return absl::StrCat(name, ": ", trouble);
    }
  }
  return {};
}

}  // namespace ra
