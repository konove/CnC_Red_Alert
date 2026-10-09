// GadgetList, the gadgets a dialog or the map screen offers input to.

#ifndef CNC_RED_ALERT_GAME_UI_GADGET_LIST_H_
#define CNC_RED_ALERT_GAME_UI_GADGET_LIST_H_

#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"

namespace game {

class ControlClass;
class GadgetClass;

// The gadgets of one dialog, in the order they are offered input and drawn.
// The list does not own them: a gadget is usually a local next to the list, and
// either may be destroyed first.
//
// A gadget is in one list at most. Adding it to a list takes it out of the one
// it was in, so a gadget two dialogs share is handed over by adding it. The
// gadget keeps its keyboard focus on the way. The parts of a composite gadget
// (a list box's scroll gadgets, a drop list's button and dropped list) do not:
// their owner takes them out with Remove(), so adding a list box again lets go
// of a scroll thumb that is being dragged.
//
// Input() gives an event to the first gadget that takes it, so the order is
// part of a dialog's behavior where gadgets overlap.
//
// Example:
//   game::GadgetList commands;
//   commands.Add(ok_button);
//   commands.Add(cancel_button);
//   while (process) {
//     const auto input = commands.Input(view);
//     ...
//   }
class GadgetList {
 public:
  GadgetList() = default;
  // Not copyable or movable: its gadgets point back at it.
  GadgetList(const GadgetList&) = delete;
  GadgetList& operator=(const GadgetList&) = delete;
  GadgetList(GadgetList&&) = delete;
  GadgetList& operator=(GadgetList&&) = delete;
  // Empties the list as Clear() does.
  ~GadgetList();

  // Adds `gadget` at the end, where it is offered input last.
  void Add(GadgetClass& gadget);
  // Adds `gadget` at the front, where it is offered input first.
  void AddFront(GadgetClass& gadget);
  // Adds `gadget` right behind `predecessor`, which has to be in this list.
  // Does nothing when the two are the same gadget.
  void AddAfter(GadgetClass& gadget, const GadgetClass& predecessor);

  // Takes `gadget` out and drops its keyboard focus and its hold on the mouse
  // (a sticky gadget being pressed), and those of its parts. Does nothing when
  // it is not in this list.
  void Remove(GadgetClass& gadget);

  // Takes every gadget out. Unlike Remove() this keeps the focus where it is,
  // so a dialog that builds its list again on every redraw does not take the
  // focus from the edit field being typed in.
  void Clear();

  // Reads one input event, if any, offers it to the gadgets and redraws them on
  // `view`. Returns the event. When a gadget acted on it, its key is what the
  // gadget left there (its ButtonKey(), or KN_NONE) and it is no longer a
  // click, so a click still in the event is one no gadget took. The event
  // keeps the click's position either way.
  //
  // The first call after another list had its turn draws every gadget and
  // drops the focus and the hold on the mouse, which belonged to that list.
  engine::window::InputEvent Input(PixelView& view);

  // Draws every gadget on `view`; with `forced` false only those flagged to
  // be redrawn.
  void DrawAll(PixelView& view, bool forced = true);

  // Makes the next Input() draw every gadget. Like a change of list it also
  // drops the focus and the hold on the mouse.
  void FlagToRedraw();

  // Returns whether any gadget is flagged to be redrawn.
  [[nodiscard]] bool IsToRedraw() const;

  // Returns the first gadget whose Get_ID() is `id`, or nullptr when there is
  // none, when it is no ControlClass or when `id` is 0, which is no ID.
  [[nodiscard]] ControlClass* Extract(unsigned id) const;

  [[nodiscard]] bool empty() const { return gadgets_.empty(); }
  [[nodiscard]] std::span<GadgetClass* const> gadgets() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return gadgets_;
  }

 private:
  // GadgetClass's destructor takes the gadget out without the hooks.
  friend class GadgetClass;

  // Takes `gadget` out of the list it is in, if any, and tells it so. Keeps
  // its focus.
  static void Detach(GadgetClass& gadget);

  // Erases `gadget` from gadgets_ and clears its back pointer.
  void Erase(GadgetClass& gadget);

  // Puts `gadget`, which is in no list, at `index` and tells it so.
  void InsertAt(base::ssize index, GadgetClass& gadget);

  // The list Input() ran on last; nullptr when the next call is to draw
  // everything.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static inline const GadgetList* last_input_ = nullptr;

  std::vector<GadgetClass*> gadgets_;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_GADGET_LIST_H_
