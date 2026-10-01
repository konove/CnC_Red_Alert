#include "engine/net/global_conn.h"

namespace engine::net {

// Out of line, so the vtable is emitted once.
GlobalConnClass::~GlobalConnClass() = default;

}  // namespace engine::net
