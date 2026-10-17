#include "game/ui/gadget_list.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "base/algorithm.h"
#include "base/array.h"
#include "base/flags.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"

namespace game {

void (*GadgetList::key_hook_)(engine::window::KeyNumber key) = nullptr;

GadgetList::~GadgetList() {
  Clear();
  if (last_input_ == this) {
    last_input_ = nullptr;
  }
}

void GadgetList::Add(Gadget& gadget) {
  Detach(gadget);
  InsertAt(std::ssize(gadgets_), gadget);
}

void GadgetList::AddFront(Gadget& gadget) {
  Detach(gadget);
  InsertAt(0, gadget);
}

void GadgetList::AddAfter(Gadget& gadget, const Gadget& predecessor) {
  if (&gadget == &predecessor) {
    return;
  }
  // Before the search: taking `gadget` out moves what stands behind it.
  Detach(gadget);
  const base::ssize index = base::IndexOf(gadgets_, &predecessor);
  DCHECK(index >= 0) << "the predecessor is not in this list";
  InsertAt(index < 0 ? std::ssize(gadgets_) : index + 1, gadget);
}

void GadgetList::Remove(Gadget& gadget) {
  if (gadget.list_ != this) {
    return;
  }
  gadget.ClearFocus();
  gadget.holds_mouse_ = false;
  Detach(gadget);
}

void GadgetList::Clear() {
  // Emptied first, so that a gadget's OnRemoved() finds its parts gone already
  // and may add to the list again.
  const std::vector<Gadget*> removed = std::exchange(gadgets_, {});
  for (Gadget* gadget : removed) {
    gadget->list_ = nullptr;
  }
  for (Gadget* gadget : removed) {
    gadget->OnRemoved(*this);
  }
}

engine::window::InputEvent GadgetList::Input(PixelView& view) {
  bool forced = false;
  if (last_input_ != this) {
    if (last_input_ != nullptr) {
      last_input_->DropFocusAndMouse();
    }
    DropFocusAndMouse();
    last_input_ = this;
    forced = true;
  }

  Gadget::PendingInput input = ReadInput();
  if (!OfferToStuckOrFocused(view, input)) {
    // By index, and with the size read again every time: a gadget's action may
    // add or remove gadgets (a drop list opening, the message list).
    for (base::ssize i = 0; i < std::ssize(gadgets_); ++i) {
      if (base::At(gadgets_, i)->Offer(view, forced, input)) {
        break;
      }
    }
  }
  return FinishInput(input);
}

void GadgetList::DrawAll(PixelView& view, bool forced) {
  for (base::ssize i = 0; i < std::ssize(gadgets_); ++i) {
    base::At(gadgets_, i)->Draw(view, forced);
  }
}

void GadgetList::FlagToRedraw() {
  if (last_input_ == this) {
    last_input_ = nullptr;
  }
}

bool GadgetList::IsToRedraw() const {
  return std::ranges::any_of(
      gadgets_, [](const Gadget* gadget) { return gadget->IsToRedraw(); });
}

ControlClass* GadgetList::Extract(unsigned id) const {
  if (id == 0) {
    return nullptr;
  }
  const auto found = std::ranges::find_if(
      gadgets_, [id](const Gadget* gadget) { return gadget->id() == id; });
  return found == gadgets_.end() ? nullptr
                                 : dynamic_cast<ControlClass*>(*found);
}

void GadgetList::Detach(Gadget& gadget) {
  GadgetList* const list = gadget.list_;
  if (list == nullptr) {
    return;
  }
  list->Erase(gadget);
  gadget.OnRemoved(*list);
}

void GadgetList::Erase(Gadget& gadget) {
  std::erase(gadgets_, &gadget);
  gadget.list_ = nullptr;
}

void GadgetList::InsertAt(base::ssize index, Gadget& gadget) {
  // Before the insertion, so that the search finds only the others.
  if (gadget.has_focus_) {
    if (Gadget* const holder = FocusedGadget()) {
      holder->FlagToRedraw();
      holder->ClearFocus();
    }
  }
  gadgets_.insert(gadgets_.begin() + index, &gadget);
  gadget.list_ = this;
  gadget.OnAdded(*this);
}

Gadget* GadgetList::FocusedGadget() const {
  const auto found = std::ranges::find_if(
      gadgets_, [](const Gadget* gadget) { return gadget->has_focus_; });
  return found == gadgets_.end() ? nullptr : *found;
}

Gadget* GadgetList::MouseHolder() const {
  const auto found = std::ranges::find_if(
      gadgets_, [](const Gadget* gadget) { return gadget->holds_mouse_; });
  return found == gadgets_.end() ? nullptr : *found;
}

void GadgetList::DropFocusAndMouse() {
  for (Gadget* const gadget : gadgets_) {
    gadget->DropFocusAndMouse();
  }
}

Gadget::PendingInput GadgetList::ReadInput() {
  Gadget::PendingInput input;

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
    input.flags |= Gadget::InputFlags::kLeftPress;
  }
  if (event.IsPress(MouseButton::kRight)) {
    input.flags |= Gadget::InputFlags::kRightPress;
  }
  if (event.IsRelease(MouseButton::kLeft)) {
    input.flags |= Gadget::InputFlags::kLeftRelease;
  }
  if (event.IsRelease(MouseButton::kRight)) {
    input.flags |= Gadget::InputFlags::kRightRelease;
  }

  // An event with a key but no mouse bits came from the keyboard.
  if (input.key && !base::Any(input.flags)) {
    input.flags |= Gadget::InputFlags::kKeyboard;
  }

  // Held and up are reported only when there is no event. Otherwise a gadget
  // listening for Gadget::InputFlags::kLeftUp under the mouse would act on, and
  // swallow, every key and click.
  if (!event) {
    // The event queue reports only changes, so the held state comes from the
    // buttons themselves.
    input.flags |= engine::window::IsLeftButtonDown()
                       ? Gadget::InputFlags::kLeftHeld
                       : Gadget::InputFlags::kLeftUp;
    input.flags |= engine::window::IsRightButtonDown()
                       ? Gadget::InputFlags::kRightHeld
                       : Gadget::InputFlags::kRightUp;
  }
  return input;
}

bool GadgetList::OfferToStuckOrFocused(PixelView& view,
                                       Gadget::PendingInput& input) {
  // While a gadget holds the mouse it gets every event and no other gadget
  // is drawn or offered anything.
  if (Gadget* const captor = MouseHolder()) {
    captor->Draw(view, false);
    // A release lets go of the mouse; the gadget is still redrawn so it can
    // show itself raised.
    captor->ActionIfConcerned(input);
    captor->Draw(view, false);
    return true;
  }

  // Keys go only to the focused gadget; its mouse events go through the
  // normal sweep.
  if (Gadget* const focused = FocusedGadget();
      focused != nullptr &&
      base::Any(input.flags & Gadget::InputFlags::kKeyboard)) {
    focused->Draw(view, false);
    focused->ActionIfConcerned(input);
    // The focus may have moved on to another gadget, which shows it now.
    if (Gadget* const now_focused = FocusedGadget()) {
      now_focused->Draw(view, false);
    }
    return true;
  }
  return false;
}

engine::window::InputEvent GadgetList::FinishInput(
    const Gadget::PendingInput& input) {
  engine::window::InputEvent event = input.event;
  // A gadget that acted replaced the key, and used up the click with it.
  if (input.key != event.key) {
    event.key = input.key;
    event.button = engine::window::MouseButton::kNone;
  }
  return event;
}

}  // namespace game
