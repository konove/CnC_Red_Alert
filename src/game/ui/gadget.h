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

// Gadget, the base of every on-screen control both games share: a
// rectangle of the screen that reacts to the mouse and the keyboard. A
// GadgetList holds the gadgets of one screen and feeds them input.
//
// The hierarchy, as it stands in game/ui:
//
//   Gadget         a screen region; dispatches input to Action()
//     TextLabelClass    a line of text
//     ControlClass      adds an ID that Input() returns like a keystroke, and
//                       a peer it tells when it is actioned (a list box
//                       watches its slider this way)
//       GaugeClass      a value control without a thumb, like a volume slider
//         SliderClass   a scroll bar with a thumb; scrolls a list box
//       ListClass       a scrolling list of text lines with its own slider
//       EditClass       a line of editable text
//       ToggleClass     button logic: triggers on a release over the button,
//                       and tracks pressed and raised; draws nothing itself
//         TextButtonClass   a button showing text ("OK", "Cancel")
//         ShapeButtonClass  a button showing a shape (a scroll arrow)
//         CheckBoxClass     a box that is checked or not
//
// Each game derives its own gadgets from these as well (the sidebar strips,
// the tactical map, the radar).

#ifndef CNC_RED_ALERT_GAME_UI_GADGET_H_
#define CNC_RED_ALERT_GAME_UI_GADGET_H_

#include <cstdint>

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"

namespace game {

class ControlClass;
class GadgetList;

// A rectangle of the screen that reacts to input. Derived classes override
// Action() to do their job and Draw() to draw themselves; a GadgetList
// calls both from its Input().
//
// Keyboard focus and the mouse capture ("sticky" processing) are global: one
// gadget at a time holds each, in focused_ and mouse_captor_.
//
// Example:
//   GadgetList buttons;
//   buttons.Add(ok_button);  // A ControlClass with the ID kOk.
//   if (buttons.Input(view).key == engine::window::ButtonKey(kOk)) ...
class Gadget {
 public:
  // Input events a gadget listens for, combined in event_mask_. A press or
  // release is reported once, when it happens; held (the button is down) and
  // up (it is not) describe the button's state on each Input() call that has
  // no event to report.
  static constexpr uint32_t kLeftPress = 0x0001;
  static constexpr uint32_t kLeftHeld = 0x0002;
  static constexpr uint32_t kLeftRelease = 0x0004;
  static constexpr uint32_t kLeftUp = 0x0008;
  static constexpr uint32_t kRightPress = 0x0010;
  static constexpr uint32_t kRightHeld = 0x0020;
  static constexpr uint32_t kRightRelease = 0x0040;
  static constexpr uint32_t kRightUp = 0x0080;
  // A key that came from the keyboard, not a mouse button.
  static constexpr uint32_t kKeyboard = 0x0100;

  // Owns the region at (`x`, `y`), `width` by `height` screen pixels, and acts
  // on the events in `flags`. A `sticky` gadget also listens for left presses
  // and releases, which it needs to capture the mouse.
  Gadget(int x, int y, int width, int height, uint32_t flags,
         bool sticky = false) noexcept;
  Gadget() noexcept = default;
  // Not copyable or movable: the list it is in points at it.
  Gadget(const Gadget&) = delete;
  Gadget& operator=(const Gadget&) = delete;
  Gadget(Gadget&&) = delete;
  Gadget& operator=(Gadget&&) = delete;
  // Releases the keyboard focus and the mouse capture if this gadget holds
  // them, and leaves its list.
  virtual ~Gadget();

  // Returns the list this gadget is in, or nullptr when it is in none.
  [[nodiscard]] GadgetList* list() const { return list_; }

  // Disable() makes the gadget ignore input; it is still drawn, so a gadget
  // that should vanish draws nothing while disabled or leaves its list.
  // Enable() undoes it. Both flag a redraw and drop the keyboard focus.
  void Disable();
  void Enable();
  // Returns the ID GadgetList::Input() reports for this gadget; 0 for a gadget
  // without one (only a ControlClass has one).
  [[nodiscard]] virtual unsigned id() const { return 0; }
  // Has Draw() draw the gadget the next time its list's Input() runs.
  virtual void FlagToRedraw();
  // Called by a ControlClass this gadget is the peer of, after it was
  // actioned with `flags` and `key`; the peer may replace `key`. Does nothing
  // here.
  virtual void OnPeerAction(uint32_t /*flags*/,
                            engine::window::KeyNumber& /*key*/,
                            ControlClass& /*peer*/) {}
  // Routes every keyboard event to this gadget, taking the focus from the
  // gadget that had it (which is redrawn).
  virtual void SetFocus();
  // Gives up the keyboard focus if this gadget has it; otherwise does nothing.
  // A derived class overrides it to react to losing the focus.
  virtual void ClearFocus();
  // Returns whether keyboard events are routed to this gadget.
  [[nodiscard]] bool HasFocus() const;
  // Returns whether the gadget waits to be drawn.
  [[nodiscard]] bool IsToRedraw() const { return to_redraw_; }
  // Moves the gadget's top left corner to (`x`, `y`), in screen pixels.
  virtual void SetPosition(int x, int y);

  // Returns whether the gadget is to be drawn now, which it is when `forced`
  // or flagged, and clears the flag. Draws nothing itself: an override calls
  // it and draws when it returns true.
  virtual bool Draw(PixelView& view, bool forced = false);

  // The gadget's region, in screen pixels. The setters move or resize only
  // this gadget; SetPosition() also moves the parts of a composite one.
  [[nodiscard]] int x() const { return x_; }
  [[nodiscard]] int y() const { return y_; }
  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }
  void set_x(const int x) { x_ = x; }
  void set_y(const int y) { y_ = y; }
  void set_width(const int width) { width_ = width; }
  void set_height(const int height) { height_ = height; }

  // A sticky gadget captures the mouse: pressed over it, it alone gets the
  // input until the left button is released, as a Windows button does. The
  // sidebar's buttons are static and are made sticky after they are
  // constructed.
  void set_sticky(const bool sticky) { sticky_ = sticky; }

  // The sticky gadget that holds the mouse while the left button is down;
  // nullptr when none does.
  [[nodiscard]] static Gadget* mouse_captor() { return mouse_captor_; }

 protected:
  // Captures the mouse for a sticky gadget on a left press in `flags`, and
  // releases it on a left release. The base Action() calls it; an override
  // that clears the press bits before calling the base Action() calls it
  // first itself.
  void UpdateMouseCapture(uint32_t flags);

  // Acts on the input `flags` and `key` describe, the main way a gadget does
  // its job. `x` and `y` are where it happened: a click's position for a
  // press or release, the mouse's for anything else. A gadget that acts on a
  // key replaces `key` with its ButtonKey(). Returns whether it acted, which
  // ends the list's sweep. The base flags a redraw and handles the mouse
  // capture when `flags` holds anything.
  virtual bool Action(uint32_t flags, engine::window::KeyNumber& key, int x,
                      int y);

  // Called by a GadgetList after it added this gadget or took it out. A gadget
  // made of several (a list box and its scroll bar) adds and removes its
  // parts here. OnRemoved() is not called for a gadget that leaves its list by
  // being destroyed.
  virtual void OnAdded(GadgetList& /*list*/) {}
  virtual void OnRemoved(GadgetList& /*list*/) {}

  // Whether Disable() is in effect.
  [[nodiscard]] bool disabled() const { return disabled_; }

 private:
  // GadgetList::Input() runs the steps below over its gadgets.
  friend class GadgetList;

  // One GadgetList::Input() call's event on its way through the gadgets.
  struct PendingInput {
    engine::window::InputEvent event;
    // Starts as the event's key; a gadget that acts replaces it.
    engine::window::KeyNumber key = engine::window::KeyNumber::KN_NONE;
    uint32_t flags = 0;  // The k... event bits above.
    int x = 0;
    int y = 0;
  };

  // Draws this gadget if it has to be (always when `forced`) and offers it
  // `input`, as a step of GadgetList::Input(). Returns whether it acted, which
  // ends the sweep.
  bool Offer(PixelView& view, bool forced, PendingInput& input);

  // Calls Action() with the events of `input` this gadget listens for, when
  // they concern it: always for the gadget holding the mouse and for a key,
  // otherwise only for mouse events inside its region. Action() may replace
  // `input.key`. Returns what Action() returned, or false when it was not
  // called.
  bool ActionIfConcerned(PendingInput& input);

  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;

  // The gadget every keyboard event goes to, to the exclusion of all others;
  // nullptr when no gadget has the focus.
  static Gadget* focused_;

  // Set when the gadget's look has changed and Draw() has to draw it; the
  // base Action() sets it whenever it acts.
  bool to_redraw_ : 1 = false;

  // See set_sticky().
  bool sticky_ : 1 = false;

  // See mouse_captor().
  static Gadget* mouse_captor_;

  // Set by Disable(): the list's sweep draws the gadget but does not offer it
  // input.
  bool disabled_ : 1 = false;

  // The k... events Action() is called for: kLeftPress alone calls it only
  // for a left press over the region. kKeyboard is added while the gadget
  // has the focus.
  uint32_t event_mask_ = 0;

  // The list this gadget is in; nullptr when in none.
  GadgetList* list_ = nullptr;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_GADGET_H_
