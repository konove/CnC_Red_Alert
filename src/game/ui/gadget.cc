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

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"

namespace game {

using enum engine::window::KeyNumber;

GadgetClass* GadgetClass::StuckOn = nullptr;

GadgetClass* GadgetClass::Focused = nullptr;

void (*GadgetClass::KeyHook)(engine::window::KeyNumber key) = nullptr;

GadgetClass::GadgetClass(int x, int y, int w, int h, unsigned flags,
                         bool sticky) noexcept
    : X(x),
      Y(y),
      Width(w),
      Height(h),

      IsSticky(sticky),

      Flags(flags) {
  if (IsSticky) {
    Flags |= kLeftPress | kLeftRelease;
  }
}

GadgetClass::~GadgetClass() {
  // Drop the keyboard focus directly. Clear_Focus is virtual -- DropListClass
  // collapses its drop-down in its override -- and a destructor cannot reach
  // that override; a derived class that needs more does it in its own
  // destructor, while the object is still of that type.
  if (Focused == this) {
    Flags &= ~kKeyboard;
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

bool GadgetClass::Clicked_On(engine::window::KeyNumber& key, unsigned flags,
                             int mousex, int mousey) {
  // Keep only the events this gadget listens for; none left means a mouse
  // event is not for it.
  flags &= Flags;

  // A key always reaches Action(), which decides whether it wants it; so does
  // anything while this gadget holds the mouse, even a drag outside it. Other
  // mouse events must fall inside the region. The unsigned casts turn a
  // position left of or above the region into a huge value, so one compare
  // per axis checks both edges.
  if (this == StuckOn || flags & kKeyboard ||
      (flags &&
       static_cast<unsigned>(mousex - X) < static_cast<unsigned>(Width) &&
       static_cast<unsigned>(mousey - Y) < static_cast<unsigned>(Height))) {
    return Action(flags, key, mousex, mousey);
  }
  return false;
}

void GadgetClass::Enable() {
  IsDisabled = false;
  IsToRepaint = true;
  Clear_Focus();
}

void GadgetClass::Disable() {
  IsDisabled = true;
  IsToRepaint = true;
  Clear_Focus();
}

bool GadgetClass::Action(unsigned flags, engine::window::KeyNumber& /*unused*/,
                         int /*x*/, int /*y*/) {
  // Clicked_On() also calls this with no events for the gadget holding the
  // mouse; that is not an action, and the sweep goes on.
  if (flags) {
    IsToRepaint = true;
    Sticky_Process(flags);
    return true;
  }
  return false;
}

bool GadgetClass::Draw_Me(PixelView& /*view*/, bool forced) {
  if (forced || IsToRepaint) {
    IsToRepaint = false;
    return true;
  }
  return false;
}

GadgetClass::PendingInput GadgetClass::ReadInput() {
  PendingInput input;

  // The keyboard queue holds the mouse clicks as well as the keys. With
  // nothing queued the event stays empty and only the held/up state below is
  // reported.
  if (TheKeyboard().Peek()) {
    input.event = TheKeyboard().Read();
  }
  const engine::window::InputEvent& event = input.event;
  input.key = event.key;

  if (KeyHook != nullptr) {
    KeyHook(input.key);
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

  // Translate the event into the k... bits each gadget filters by its Flags.
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

bool GadgetClass::OfferToStuckOrFocused(PixelView& view, PendingInput& input) {
  // While a gadget holds the mouse it gets every event and no other gadget
  // is drawn or offered anything.
  if (StuckOn) {
    StuckOn->Draw_Me(view, false);
    // A release in Clicked_On() clears StuckOn; the gadget is still redrawn
    // so it can show itself raised.
    GadgetClass* oldstuck = StuckOn;
    StuckOn->Clicked_On(input.key, input.flags, input.x, input.y);
    if (StuckOn) {
      StuckOn->Draw_Me(view, false);
    } else {
      oldstuck->Draw_Me(view, false);
    }
    return true;
  }

  // Keys go only to the focused gadget; its mouse events go through the
  // normal sweep.
  if (Focused && input.flags & kKeyboard) {
    Focused->Draw_Me(view, false);
    Focused->Clicked_On(input.key, input.flags, input.x, input.y);
    if (Focused) {
      Focused->Draw_Me(view, false);
    }
    return true;
  }
  return false;
}

bool GadgetClass::Offer(PixelView& view, bool forced, PendingInput& input) {
  Draw_Me(view, forced);

  // A gadget that acted has used up the event, so the sweep stops there.
  if (!IsDisabled && Clicked_On(input.key, input.flags, input.x, input.y)) {
    // Action() flags a gadget whose look it changed; show the change now.
    Draw_Me(view, false);
    return true;
  }
  return false;
}

engine::window::InputEvent GadgetClass::FinishInput(const PendingInput& input) {
  engine::window::InputEvent event = input.event;
  // A gadget that acted replaced the key, and used up the click with it.
  if (input.key != event.key) {
    event.key = input.key;
    event.button = engine::window::MouseButton::kNone;
  }
  return event;
}

void GadgetClass::Flag_To_Redraw() { IsToRepaint = true; }

void GadgetClass::Sticky_Process(unsigned flags) {
  if (IsSticky && flags & kLeftPress) {
    StuckOn = this;
  }
  if (StuckOn == this && flags & kLeftRelease) {
    StuckOn = nullptr;
  }
}

void GadgetClass::Set_Focus() {
  if (Focused) {
    Focused->Flag_To_Redraw();
    Focused->Clear_Focus();
  }
  Flags |= kKeyboard;
  Focused = this;
}

void GadgetClass::Clear_Focus() {
  if (Focused == this) {
    Flags &= ~kKeyboard;
    Focused = nullptr;
  }
}

bool GadgetClass::Has_Focus() { return this == Focused; }

void GadgetClass::Set_Position(int x, int y) {
  X = x;
  Y = y;
}

}  // namespace game
