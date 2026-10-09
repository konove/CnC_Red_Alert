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

#include "game/ui/link.h"

namespace game {

// Qualified because a destructor must not make a virtual call: the derived
// part of the object, and with it a derived Remove(), is already gone.
Link::~Link() { Link::Remove(); }

void Link::ForgetNeighbors() {
  next_ = nullptr;
  previous_ = nullptr;
}

Link* Link::Next() const { return next_; }

Link* Link::Previous() const { return previous_; }

const Link& Link::HeadOfList() const { return HeadOf(*this); }

const Link& Link::TailOfList() const { return TailOf(*this); }

Link& Link::InsertAfter(Link& predecessor) {
  // A node cannot follow itself: linking it would make previous_ point at this
  // node and cut the list off in front of it.
  if (&predecessor == this) {
    return HeadOfList();
  }

  Link* old_next = predecessor.next_;

  // Link this node in after `predecessor`.
  predecessor.next_ = this;
  previous_ = &predecessor;

  // Link it to the node that used to follow `predecessor`, if there was one.
  next_ = old_next;
  if (old_next) {
    old_next->previous_ = this;
  }

  return HeadOfList();
}

Link& Link::PrependTo(Link& list) {
  Link* old_head = &list.HeadOfList();

  // Already the head. Linking it in front of itself would make next_ point at
  // this node, and a walk down the list would never end.
  if (old_head == this) {
    return *this;
  }

  // Link this node in front of the old head.
  old_head->previous_ = this;
  next_ = old_head;
  previous_ = nullptr;

  return *this;
}

Link& Link::AppendTo(Link& list) {
  Link* old_tail = &list.TailOfList();

  // Already the tail. Linking it behind itself would make previous_ point at
  // this node and lose every node in front of it.
  if (old_tail == this) {
    return HeadOfList();
  }

  // Link this node in behind the old tail.
  old_tail->next_ = this;
  previous_ = old_tail;
  next_ = nullptr;

  return HeadOfList();
}

Link* Link::Remove() {
  // Found before unlinking: afterwards this node leads nowhere.
  Link* head = &HeadOfList();
  Link* tail = &TailOfList();

  if (previous_) {
    previous_->next_ = next_;
  }
  if (next_) {
    next_->previous_ = previous_;
  }
  previous_ = nullptr;
  next_ = nullptr;

  // The head is unchanged unless this node was it. Then the new head is found
  // from the tail, the one other node known to be still in the list.
  if (head == this) {
    if (tail == this) {
      return nullptr;
    }
    return &tail->HeadOfList();
  }
  return head;
}

}  // namespace game
