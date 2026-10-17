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
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"

namespace game {

Gadget* Gadget::mouse_captor_ = nullptr;

Gadget* Gadget::focused_ = nullptr;

Gadget::Gadget(int x, int y, int width, int height, uint32_t flags,
               bool sticky) noexcept
    : x_(x),
      y_(y),
      width_(width),
      height_(height),

      sticky_(sticky),

      event_mask_(flags) {
  if (sticky_) {
    event_mask_ |= kLeftPress | kLeftRelease;
  }
}

Gadget::~Gadget() {
  // The base version: by now the overrides (DropListClass collapses its
  // drop-down in its own) are gone, so a derived class that needs more does
  // it in its own destructor, while the object is still of that type.
  Gadget::ClearFocus();

  if (this == mouse_captor_) {
    mouse_captor_ = nullptr;
  }

  // Not through Remove(): its hook is a virtual call this destructor cannot
  // make any more.
  if (list_ != nullptr) {
    list_->Erase(*this);
  }
}

bool Gadget::ActionIfConcerned(PendingInput& input) {
  // Keep only the events this gadget listens for; none left means a mouse
  // event is not for it. The sweep offers `input` to the next gadget too, so
  // its flags stay as they are.
  const uint32_t flags = input.flags & event_mask_;

  // A key always reaches Action(), which decides whether it wants it; so does
  // anything while this gadget holds the mouse, even a drag outside it. Other
  // mouse events must fall inside the region. The unsigned casts turn a
  // position left of or above the region into a huge value, so one compare
  // per axis checks both edges.
  if (this == mouse_captor_ || flags & kKeyboard ||
      (flags &&
       static_cast<unsigned>(input.x - x_) < static_cast<unsigned>(width_) &&
       static_cast<unsigned>(input.y - y_) < static_cast<unsigned>(height_))) {
    return Action(flags, input.key, input.x, input.y);
  }
  return false;
}

void Gadget::Enable() {
  disabled_ = false;
  to_redraw_ = true;
  ClearFocus();
}

void Gadget::Disable() {
  disabled_ = true;
  to_redraw_ = true;
  ClearFocus();
}

bool Gadget::Action(uint32_t flags, engine::window::KeyNumber& /*key*/,
                    int /*x*/, int /*y*/) {
  // ActionIfConcerned() also calls this with no events for the gadget holding
  // the mouse; that is not an action, and the sweep goes on.
  if (flags) {
    to_redraw_ = true;
    UpdateMouseCapture(flags);
    return true;
  }
  return false;
}

bool Gadget::Draw(PixelView& /*view*/, bool forced) {
  if (forced || to_redraw_) {
    to_redraw_ = false;
    return true;
  }
  return false;
}



bool Gadget::Offer(PixelView& view, bool forced, PendingInput& input) {
  Draw(view, forced);

  // A gadget that acted has used up the event, so the sweep stops there.
  if (!disabled_ && ActionIfConcerned(input)) {
    // Action() flags a gadget whose look it changed; show the change now.
    Draw(view, false);
    return true;
  }
  return false;
}

void Gadget::FlagToRedraw() { to_redraw_ = true; }

void Gadget::UpdateMouseCapture(uint32_t flags) {
  if (sticky_ && flags & kLeftPress) {
    mouse_captor_ = this;
  }
  if (mouse_captor_ == this && flags & kLeftRelease) {
    mouse_captor_ = nullptr;
  }
}

void Gadget::SetFocus() {
  if (focused_) {
    focused_->FlagToRedraw();
    focused_->ClearFocus();
  }
  event_mask_ |= kKeyboard;
  focused_ = this;
}

void Gadget::ClearFocus() {
  if (focused_ == this) {
    event_mask_ &= ~kKeyboard;
    focused_ = nullptr;
  }
}

bool Gadget::HasFocus() const { return this == focused_; }

void Gadget::SetPosition(int x, int y) {
  x_ = x;
  y_ = y;
}

}  // namespace game
