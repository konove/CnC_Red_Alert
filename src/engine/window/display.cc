// Implements what every Display shares: its client, its observer, and
// MakeDisplay().

#include "engine/window/display.h"

#include <memory>

#include "engine/window/display_client.h"
#include "engine/window/display_observer.h"
#include "engine/window/headless_display.h"
#include "engine/window/window_display.h"

namespace engine::window {

namespace {

// The client of a Display built without one: it drops every event.
class NoClient final : public DisplayClient {
 public:
  void HandleEvent(const SDL_Event& /*event*/) override {}
};

DisplayClient& DroppingClient() {
  static NoClient client;
  return client;
}

}  // namespace

DisplayClient::~DisplayClient() = default;

DisplayObserver::~DisplayObserver() = default;

Display::Display() : client_(&DroppingClient()) {}

Display::Display(DisplayClient& client) : client_(&client) {}

Display::~Display() = default;

void Display::NotifyFrameEnd() {
  if (observer_ != nullptr) {
    observer_->OnFrameEnd();
  }
}

void Display::NotifyPump() {
  if (observer_ != nullptr) {
    observer_->OnPump();
  }
}

std::unique_ptr<Display> MakeDisplay(const DisplayKind kind,
                                     DisplayClient& client) {
  switch (kind) {
    case DisplayKind::kWindow:
      return std::make_unique<WindowDisplay>(client);
    case DisplayKind::kScriptedWindow:
      return std::make_unique<ScriptedWindowDisplay>(client);
    case DisplayKind::kHeadless:
    default:
      return std::make_unique<HeadlessDisplay>(client);
  }
}

}  // namespace engine::window
