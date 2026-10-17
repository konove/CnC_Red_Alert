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

Gadget* Gadget::StuckOn = nullptr;

Gadget* Gadget::Focused = nullptr;

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
  // The base version: by now the overrides (DropListClass collapses its
  // drop-down in its own) are gone, so a derived class that needs more does
  // it in its own destructor, while the object is still of that type.
  Gadget::ClearFocus();

  if (this == StuckOn) {
    StuckOn = nullptr;
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
  if (this == StuckOn || flags & kKeyboard ||
      (flags &&
       static_cast<unsigned>(input.x - X) < static_cast<unsigned>(Width) &&
       static_cast<unsigned>(input.y - Y) < static_cast<unsigned>(Height))) {
    return Action(flags, input.key, input.x, input.y);
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



bool Gadget::Offer(PixelView& view, bool forced, PendingInput& input) {
  Draw(view, forced);

  // A gadget that acted has used up the event, so the sweep stops there.
  if (!IsDisabled && ActionIfConcerned(input)) {
    // Action() flags a gadget whose look it changed; show the change now.
    Draw(view, false);
    return true;
  }
  return false;
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

bool Gadget::HasFocus() const { return this == Focused; }

void Gadget::SetPosition(int x, int y) {
  X = x;
  Y = y;
}

}  // namespace game
