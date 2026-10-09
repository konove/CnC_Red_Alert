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
// The Add functions expect this node to be in no list: they overwrite its
// pointers without unlinking it, so a node that is still linked has to be
// taken out with Remove() first. The one case they allow for is a node that is
// already where it is asked to go (the head for Add_Head(), the tail for
// Add_Tail(), `list` itself for Add()), which they leave alone.
//
// Example:
//   Link a, b, c;
//   b.Add_Tail(a);  // a b
//   c.Add_Tail(a);  // a b c
//   b.Remove();     // a c
class Link {
 public:
  Link() noexcept = default;
  virtual ~Link();
  Link(Link&&) = delete;
  Link& operator=(Link&&) = delete;

  // Return the node after or before this one, or nullptr at that end of the
  // list.
  [[nodiscard]] virtual Link* Get_Next() const;
  [[nodiscard]] virtual Link* Get_Prev() const;

  // Inserts this node right after `list`, which may be in the middle of a
  // chain; that is how a list that depends on its order, like the gadgets', is
  // put together. Returns the head of the list.
  virtual Link& Add(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Appends this node to the list `list` is in, whichever of its nodes `list`
  // is. Returns the head of the list.
  virtual Link& Add_Tail(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Puts this node in front of the list `list` is in, whichever of its nodes
  // `list` is. Returns the new head, which is this node.
  virtual Link& Add_Head(Link& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Return the first or last node of the list this one is in: this node itself
  // when it is in none.
  [[nodiscard]] const Link& Head_Of_List() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  Link& Head_Of_List() ABSL_ATTRIBUTE_LIFETIME_BOUND { return Head_Of(*this); }
  [[nodiscard]] const Link& Tail_Of_List() const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  Link& Tail_Of_List() ABSL_ATTRIBUTE_LIFETIME_BOUND { return Tail_Of(*this); }

  // Forgets both neighbours without telling them, so they go on pointing at
  // this node. Remove() is the way to take one node out; this is for dropping
  // a whole list at once, by zapping every node of it before the list is built
  // again (the sidebar's buttons, a dialog's gadgets on a redraw).
  virtual void Zap();

  // Takes this node out of its list and closes the gap. Returns the head of
  // what is left, or nullptr if this was the only node.
  virtual Link* Remove() ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Not copyable. The original copy operations did not copy: they spliced the
  // destination into the source object's list, which mutates the source. Use
  // Add() to put an object into a list.
  Link(const Link&) = delete;
  Link& operator=(const Link&) = delete;

 private:
  // The walks behind the Head_Of_List() and Tail_Of_List() overloads. Self is
  // Link or const Link, so neither overload casts away const.
  template <class Self>
  static Self& Head_Of(Self& self ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    Self* link = &self;
    while (link->Prev) {
      link = link->Prev;
      if (link == &self) {
        // The chain is a ring, which no function here makes on purpose. Stop
        // after one lap instead of walking it forever.
        break;
      }
    }
    return *link;
  }
  template <class Self>
  static Self& Tail_Of(Self& self ABSL_ATTRIBUTE_LIFETIME_BOUND) {
    Self* link = &self;
    while (link->Next) {
      link = link->Next;
      if (link == &self) {
        // The chain is a ring, which no function here makes on purpose. Stop
        // after one lap instead of walking it forever.
        break;
      }
    }
    return *link;
  }

  // The neighbours in the chain; nullptr at that end of it.
  Link* Next = nullptr;
  Link* Prev = nullptr;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_LINK_H_
