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

#include "game/ui/gadget.h"

#include <cstdint>

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"

namespace game {

using enum engine::window::KeyNumber;

Gadget* Gadget::StuckOn = nullptr;

Gadget* Gadget::Focused = nullptr;

void (*Gadget::key_hook_)(engine::window::KeyNumber key) = nullptr;

Gadget::Gadget(int x, int y, int width, int height, uint32_t flags,
               bool sticky) noexcept
    : X(x),
      Y(y),
      Width(width),
      Height(height),

      IsSticky(sticky),

      event_mask_(flags) {
  if (IsSticky) {
    event_mask_ |= kLeftPress | kLeftRelease;
  }
}

Gadget::~Gadget() {
  // Drop the keyboard focus directly. ClearFocus is virtual -- DropListClass
  // collapses its drop-down in its override -- and a destructor cannot reach
  // that override; a derived class that needs more does it in its own
  // destructor, while the object is still of that type.
  if (Focused == this) {
    event_mask_ &= ~kKeyboard;
    Focused = nullptr;
  }

  if (this == StuckOn) {
    StuckOn = nullptr;
  }

  // Not through Remove(): its hook is a virtual call this destructor cannot
  // make any more.
  if (list_ != nullptr) {
    list_->Erase(*this);
  }

  if (this == Focused) {
    Focused = nullptr;
  }
}

bool Gadget::ActionIfConcerned(engine::window::KeyNumber& key, uint32_t flags,
                               int x, int y) {
  // Keep only the events this gadget listens for; none left means a mouse
  // event is not for it.
  flags &= event_mask_;

  // A key always reaches Action(), which decides whether it wants it; so does
  // anything while this gadget holds the mouse, even a drag outside it. Other
  // mouse events must fall inside the region. The unsigned casts turn a
  // position left of or above the region into a huge value, so one compare
  // per axis checks both edges.
  if (this == StuckOn || flags & kKeyboard ||
      (flags && static_cast<unsigned>(x - X) < static_cast<unsigned>(Width) &&
       static_cast<unsigned>(y - Y) < static_cast<unsigned>(Height))) {
    return Action(flags, key, x, y);
  }
  return false;
}

void Gadget::Enable() {
  IsDisabled = false;
  IsToRepaint = true;
  ClearFocus();
}

void Gadget::Disable() {
  IsDisabled = true;
  IsToRepaint = true;
  ClearFocus();
}

bool Gadget::Action(uint32_t flags, engine::window::KeyNumber& /*key*/,
                    int /*x*/, int /*y*/) {
  // ActionIfConcerned() also calls this with no events for the gadget holding
  // the mouse; that is not an action, and the sweep goes on.
  if (flags) {
    IsToRepaint = true;
    UpdateMouseCapture(flags);
    return true;
  }
  return false;
}

bool Gadget::Draw(PixelView& /*view*/, bool forced) {
  if (forced || IsToRepaint) {
    IsToRepaint = false;
    return true;
  }
  return false;
}

Gadget::PendingInput Gadget::ReadInput() {
  PendingInput input;

  // The keyboard queue holds the mouse clicks as well as the keys. With
  // nothing queued the event stays empty and only the held/up state below is
  // reported.
  if (TheKeyboard().Peek()) {
    input.event = TheKeyboard().Read();
  }
  const engine::window::InputEvent& event = input.event;
  input.key = event.key;

  if (key_hook_ != nullptr) {
    key_hook_(input.key);
  }

  // A click is placed where the mouse was when it happened, not where it is
  // by the time this runs.
  if (event.IsClick()) {
    input.x = event.x;
    input.y = event.y;
  } else {
    input.x = TheMouse().x();
    input.y = TheMouse().y();
  }

  // Translate the event into the k... bits each gadget filters by its
  // event_mask_.
  using engine::window::MouseButton;
  if (event.IsPress(MouseButton::kLeft)) {
    input.flags |= kLeftPress;
  }
  if (event.IsPress(MouseButton::kRight)) {
    input.flags |= kRightPress;
  }
  if (event.IsRelease(MouseButton::kLeft)) {
    input.flags |= kLeftRelease;
  }
  if (event.IsRelease(MouseButton::kRight)) {
    input.flags |= kRightRelease;
  }

  // An event with a key but no mouse bits came from the keyboard.
  if (input.key && !input.flags) {
    input.flags |= kKeyboard;
  }

  // Held and up are reported only when there is no event. Otherwise a gadget
  // listening for kLeftUp under the mouse would act on, and swallow, every
  // key and click.
  if (!event) {
    // The event queue reports only changes, so the held state comes from the
    // buttons themselves.
    if (engine::window::IsLeftButtonDown()) {
      input.flags |= kLeftHeld;
    } else {
      input.flags |= kLeftUp;
    }
    if (engine::window::IsRightButtonDown()) {
      input.flags |= kRightHeld;
    } else {
      input.flags |= kRightUp;
    }
  }
  return input;
}

bool Gadget::OfferToStuckOrFocused(PixelView& view, PendingInput& input) {
  // While a gadget holds the mouse it gets every event and no other gadget
  // is drawn or offered anything.
  if (StuckOn) {
    StuckOn->Draw(view, false);
    // A release in ActionIfConcerned() clears StuckOn; the gadget is still
    // redrawn so it can show itself raised.
    Gadget* captor = StuckOn;
    StuckOn->ActionIfConcerned(input.key, input.flags, input.x, input.y);
    if (StuckOn) {
      StuckOn->Draw(view, false);
    } else {
      captor->Draw(view, false);
    }
    return true;
  }

  // Keys go only to the focused gadget; its mouse events go through the
  // normal sweep.
  if (Focused && input.flags & kKeyboard) {
    Focused->Draw(view, false);
    Focused->ActionIfConcerned(input.key, input.flags, input.x, input.y);
    if (Focused) {
      Focused->Draw(view, false);
    }
    return true;
  }
  return false;
}

bool Gadget::Offer(PixelView& view, bool forced, PendingInput& input) {
  Draw(view, forced);

  // A gadget that acted has used up the event, so the sweep stops there.
  if (!IsDisabled &&
      ActionIfConcerned(input.key, input.flags, input.x, input.y)) {
    // Action() flags a gadget whose look it changed; show the change now.
    Draw(view, false);
    return true;
  }
  return false;
}

engine::window::InputEvent Gadget::FinishInput(const PendingInput& input) {
  engine::window::InputEvent event = input.event;
  // A gadget that acted replaced the key, and used up the click with it.
  if (input.key != event.key) {
    event.key = input.key;
    event.button = engine::window::MouseButton::kNone;
  }
  return event;
}

void Gadget::FlagToRedraw() { IsToRepaint = true; }

void Gadget::UpdateMouseCapture(uint32_t flags) {
  if (IsSticky && flags & kLeftPress) {
    StuckOn = this;
  }
  if (StuckOn == this && flags & kLeftRelease) {
    StuckOn = nullptr;
  }
}

void Gadget::SetFocus() {
  if (Focused) {
    Focused->FlagToRedraw();
    Focused->ClearFocus();
  }
  event_mask_ |= kKeyboard;
  Focused = this;
}

void Gadget::ClearFocus() {
  if (Focused == this) {
    event_mask_ &= ~kKeyboard;
    Focused = nullptr;
  }
}

bool Gadget::HasFocus() { return this == Focused; }

void Gadget::SetPosition(int x, int y) {
  X = x;
  Y = y;
}

}  // namespace game
