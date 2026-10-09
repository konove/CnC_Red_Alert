#include "game/ui/gadget_list.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"

namespace game {

GadgetList::~GadgetList() {
  Clear();
  if (last_input_ == this) {
    last_input_ = nullptr;
  }
}

void GadgetList::Add(GadgetClass& gadget) {
  Detach(gadget);
  InsertAt(std::ssize(gadgets_), gadget);
}

void GadgetList::AddFront(GadgetClass& gadget) {
  Detach(gadget);
  InsertAt(0, gadget);
}

void GadgetList::AddAfter(GadgetClass& gadget, const GadgetClass& predecessor) {
  if (&gadget == &predecessor) {
    return;
  }
  // Before the search: taking `gadget` out moves what stands behind it.
  Detach(gadget);
  const auto found = std::ranges::find(gadgets_, &predecessor);
  DCHECK(found != gadgets_.end()) << "the predecessor is not in this list";
  if (found == gadgets_.end()) {
    InsertAt(std::ssize(gadgets_), gadget);
    return;
  }
  InsertAt(std::distance(gadgets_.begin(), found) + 1, gadget);
}

void GadgetList::Remove(GadgetClass& gadget) {
  if (gadget.list_ != this) {
    return;
  }
  gadget.Clear_Focus();
  if (GadgetClass::StuckOn == &gadget) {
    GadgetClass::StuckOn = nullptr;
  }
  Detach(gadget);
}

void GadgetList::Clear() {
  // Emptied first, so that a gadget's OnRemoved() finds its parts gone already
  // and may add to the list again.
  const std::vector<GadgetClass*> removed = std::exchange(gadgets_, {});
  for (GadgetClass* gadget : removed) {
    gadget->list_ = nullptr;
  }
  for (GadgetClass* gadget : removed) {
    gadget->OnRemoved(*this);
  }
}

engine::window::InputEvent GadgetList::Input(PixelView& view) {
  bool forced = false;
  if (last_input_ != this) {
    last_input_ = this;
    GadgetClass::LastList = nullptr;
    forced = true;
    GadgetClass::StuckOn = nullptr;
    GadgetClass::Focused = nullptr;
  }

  GadgetClass::PendingInput input = GadgetClass::ReadInput();
  if (!GadgetClass::OfferToStuckOrFocused(view, input)) {
    // By index, and with the size read again every time: a gadget's action may
    // add or remove gadgets (a drop list opening, the message list).
    for (base::ssize i = 0; i < std::ssize(gadgets_); ++i) {
      if (base::At(gadgets_, i)->Offer(view, forced, input)) {
        break;
      }
    }
  }
  return GadgetClass::FinishInput(input);
}

void GadgetList::DrawAll(PixelView& view, bool forced) {
  for (base::ssize i = 0; i < std::ssize(gadgets_); ++i) {
    base::At(gadgets_, i)->Draw_Me(view, forced);
  }
}

void GadgetList::FlagToRedraw() {
  if (last_input_ == this) {
    last_input_ = nullptr;
  }
}

bool GadgetList::IsToRedraw() const {
  return std::ranges::any_of(
      gadgets_, [](const GadgetClass* gadget) { return gadget->IsToRepaint; });
}

ControlClass* GadgetList::Extract(unsigned id) const {
  if (id == 0) {
    return nullptr;
  }
  const auto found = std::ranges::find_if(
      gadgets_,
      [id](const GadgetClass* gadget) { return gadget->Get_ID() == id; });
  return found == gadgets_.end() ? nullptr
                                 : dynamic_cast<ControlClass*>(*found);
}

void GadgetList::Detach(GadgetClass& gadget) {
  GadgetList* const list = gadget.list_;
  if (list == nullptr) {
    return;
  }
  list->Erase(gadget);
  gadget.OnRemoved(*list);
}

void GadgetList::Erase(GadgetClass& gadget) {
  std::erase(gadgets_, &gadget);
  gadget.list_ = nullptr;
}

void GadgetList::InsertAt(base::ssize index, GadgetClass& gadget) {
  gadgets_.insert(gadgets_.begin() + index, &gadget);
  gadget.list_ = this;
  gadget.OnAdded(*this);
}

}  // namespace game
