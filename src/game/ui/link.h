/*
**	Command & Conquer(tm)
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

// Link, the intrusive doubly linked list node under every gadget.
// Originally LINK.H (Joe L. Bostic, January 1995).

#ifndef CNC_RED_ALERT_GAME_UI_LINK_H_
#define CNC_RED_ALERT_GAME_UI_LINK_H_

#include "absl/base/attributes.h"

namespace game {

// A node of an intrusive doubly linked list: a class derives from it and its
// objects can then be chained, the way a dialog chains its gadgets. There is no
// list object. A list is known by any one of its nodes, usually the head, and
// the functions that change it return the head it has afterwards.
//
// A node does not own its neighbours. Destroying it takes it out of its list.
//
// InsertAfter(), AppendTo() and PrependTo() expect this node to be in no list:
// they overwrite its pointers without unlinking it, so a node that is still
// linked has to be taken out with Remove() first. A build with DCHECKs stops at
// one that is not; any other build links it anyway and leaves its old
// neighbours pointing at it. The one case they allow for is a node that is
// already where it is asked to go (the head for PrependTo(), the tail for
// AppendTo(), its own predecessor for InsertAfter()), which they leave alone.
//
// Example:
//   Link a, b, c;
//   b.AppendTo(a);  // a b
//   c.AppendTo(a);  // a b c
//   b.Remove();     // a c
class Link {
 public:
  Link() noexcept = default;

  // Not copyable. The original copy operations did not copy: they spliced the
  // destination into the source object's list, which mutates the source. Use
  // AppendTo() to put an object into a list.
  Link(const Link&) = delete;
  Link& operator=(const Link&) = delete;
  Link(Link&&) = delete;
  Link& operator=(Link&&) = delete;

  virtual ~Link();

  // Return the node after or before this one, or nullptr at that end of the
  // list.
  [[nodiscard]] virtual Link* Next() const;
  [[nodiscard]] virtual Link* Previous() const;

  // Inserts this node right after `predecessor`, which may be in the middle of
  // a chain; that is how a list that depends on its order, like the gadgets',
  // is put together. Returns the head of the list.
  virtual Link& InsertAfter(Link& predecessor) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Appends this node to the list `list` is in, whichever of its nodes `list`
  // is. Returns the head of the list.
  virtual Link& AppendTo(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Puts this node in front of the list `list` is in, whichever of its nodes
  // `list` is. Returns the new head, which is this node.
  virtual Link& PrependTo(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Return the first or last node of the list this one is in: this node itself
  // when it is in none.
  [[nodiscard]] const Link& HeadOfList() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  Link& HeadOfList() ABSL_ATTRIBUTE_LIFETIME_BOUND { return HeadOf(*this); }
  [[nodiscard]] const Link& TailOfList() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  Link& TailOfList() ABSL_ATTRIBUTE_LIFETIME_BOUND { return TailOf(*this); }

  // Forgets both neighbours without telling them, so they go on pointing at
  // this node. Remove() is the way to take one node out; this is for dropping
  // a whole list at once, by calling this on every node of it before the list
  // is built again (the sidebar's buttons, a dialog's gadgets on a redraw).
  virtual void ForgetNeighbors();

  // Takes this node out of its list and closes the gap. Returns the head of
  // what is left, or nullptr if this was the only node.
  virtual Link* Remove() ABSL_ATTRIBUTE_LIFETIME_BOUND;

 private:
  // The walks behind the HeadOfList() and TailOfList() overloads. Self is
  // Link or const Link, so neither overload casts away const.
  template <class Self>
  static Self& HeadOf(Self& self ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    Self* link = &self;
    while (link->previous_) {
      link = link->previous_;
      if (link == &self) {
        // The chain is a ring, which no function here makes on purpose. Stop
        // after one lap instead of walking it forever.
        break;
      }
    }
    return *link;
  }
  template <class Self>
  static Self& TailOf(Self& self ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    Self* link = &self;
    while (link->next_) {
      link = link->next_;
      if (link == &self) {
        // The chain is a ring, which no function here makes on purpose. Stop
        // after one lap instead of walking it forever.
        break;
      }
    }
    return *link;
  }

  // The neighbours in the chain; nullptr at that end of it.
  Link* next_ = nullptr;
  Link* previous_ = nullptr;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_LINK_H_
