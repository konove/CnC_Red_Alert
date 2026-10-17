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

#include "base/flags.h"
#include "base/rect.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"

namespace game {

Gadget::Gadget(const base::Rect& region, const InputFlags flags,
               const bool sticky) noexcept
    : x_(region.x()),
      y_(region.y()),
      width_(region.width()),
      height_(region.height()),

      sticky_(sticky),

      event_mask_(flags) {
  if (sticky_) {
    event_mask_ |= InputFlags::kLeftPress | InputFlags::kLeftRelease;
  }
}

Gadget::~Gadget() {
  // The focus and the hold on the mouse go with the gadget: they are its own
  // state, and nothing else points at it but its list.
  //
  // Not through Remove(): its hook is a virtual call this destructor cannot
  // make any more.
  if (list_ != nullptr) {
    list_->Erase(*this);
  }
}

void Gadget::Disable() {
  disabled_ = true;
  to_redraw_ = true;
  ClearFocus();
}

void Gadget::Enable() {
  disabled_ = false;
  to_redraw_ = true;
  ClearFocus();
}

void Gadget::FlagToRedraw() { to_redraw_ = true; }

void Gadget::SetFocus() {
  // The gadget that loses the focus may be this one, which then runs its own
  // ClearFocus() first. Out of a list there is no other to look for.
  Gadget* const holder = list_ != nullptr ? list_->FocusedGadget() : this;
  if (holder != nullptr && holder->has_focus_) {
    holder->FlagToRedraw();
    holder->ClearFocus();
  }
  has_focus_ = true;
}

void Gadget::ClearFocus() { has_focus_ = false; }

void Gadget::SetPosition(const int x, const int y) {
  x_ = x;
  y_ = y;
}

bool Gadget::Draw(PixelView& /*view*/, const bool forced) {
  if (forced || to_redraw_) {
    to_redraw_ = false;
    return true;
  }
  return false;
}

void Gadget::UpdateMouseCapture(const InputFlags flags) {
  if (sticky_ && base::Any(flags & InputFlags::kLeftPress)) {
    holds_mouse_ = true;
  }
  if (base::Any(flags & InputFlags::kLeftRelease)) {
    holds_mouse_ = false;
  }
}

bool Gadget::Action(const InputFlags flags, engine::window::KeyNumber& /*key*/,
                    int /*x*/, int /*y*/) {
  // ActionIfConcerned() also calls this with no events for the gadget holding
  // the mouse; that is not an action, and the sweep goes on.
  if (base::Any(flags)) {
    to_redraw_ = true;
    UpdateMouseCapture(flags);
    return true;
  }
  return false;
}

bool Gadget::Offer(PixelView& view, const bool forced, PendingInput& input) {
  Draw(view, forced);

  // A gadget that acted has used up the event, so the sweep stops there.
  if (!disabled_ && ActionIfConcerned(input)) {
    // Action() flags a gadget whose look it changed; show the change now.
    Draw(view, false);
    return true;
  }
  return false;
}

void Gadget::DropFocusAndMouse() {
  has_focus_ = false;
  holds_mouse_ = false;
}

bool Gadget::ActionIfConcerned(PendingInput& input) {
  // Keep only the events this gadget listens for; none left means a mouse
  // event is not for it. The sweep offers `input` to the next gadget too, so
  // its flags stay as they are.
  const InputFlags mask =
      has_focus_ ? event_mask_ | InputFlags::kKeyboard : event_mask_;
  const InputFlags flags = input.flags & mask;

  // A key always reaches Action(), which decides whether it wants it; so does
  // anything while this gadget holds the mouse, even a drag outside it. Other
  // mouse events must fall inside the region. The unsigned casts turn a
  // position left of or above the region into a huge value, so one compare
  // per axis checks both edges.
  if (holds_mouse_ || base::Any(flags & InputFlags::kKeyboard) ||
      (base::Any(flags) &&
       static_cast<unsigned>(input.x - x_) < static_cast<unsigned>(width_) &&
       static_cast<unsigned>(input.y - y_) < static_cast<unsigned>(height_))) {
    return Action(flags, input.key, input.x, input.y);
  }
  return false;
}

}  // namespace game
