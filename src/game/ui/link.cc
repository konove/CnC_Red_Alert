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
LinkClass::~LinkClass() { LinkClass::Remove(); }

void LinkClass::Zap() {
  Next = nullptr;
  Prev = nullptr;
}

LinkClass* LinkClass::Get_Next() const { return Next; }

LinkClass* LinkClass::Get_Prev() const { return Prev; }

const LinkClass& LinkClass::Head_Of_List() const { return Head_Of(*this); }

const LinkClass& LinkClass::Tail_Of_List() const { return Tail_Of(*this); }

LinkClass& LinkClass::Add(LinkClass& list) {
  // A node cannot follow itself: linking it would make Prev point at this node
  // and cut the list off in front of it.
  if (&list == this) {
    return Head_Of_List();
  }

  LinkClass* ptr = list.Next;

  // Link this node in after `list`.
  list.Next = this;
  Prev = &list;

  // Link it to the node that used to follow `list`, if there was one.
  Next = ptr;
  if (ptr) {
    ptr->Prev = this;
  }

  return Head_Of_List();
}

LinkClass& LinkClass::Add_Head(LinkClass& list) {
  LinkClass* ptr = &list.Head_Of_List();

  // Already the head. Linking it in front of itself would make Next point at
  // this node, and a walk down the list would never end.
  if (ptr == this) {
    return *this;
  }

  // Link this node in front of the old head.
  ptr->Prev = this;
  Next = ptr;
  Prev = nullptr;

  return *this;
}

LinkClass& LinkClass::Add_Tail(LinkClass& list) {
  LinkClass* ptr = &list.Tail_Of_List();

  // Already the tail. Linking it behind itself would make Prev point at this
  // node and lose every node in front of it.
  if (ptr == this) {
    return Head_Of_List();
  }

  // Link this node in behind the old tail.
  ptr->Next = this;
  Prev = ptr;
  Next = nullptr;

  return Head_Of_List();
}

LinkClass* LinkClass::Remove() {
  // Found before unlinking: afterwards this node leads nowhere.
  LinkClass* head = &Head_Of_List();
  LinkClass* tail = &Tail_Of_List();

  if (Prev) {
    Prev->Next = Next;
  }
  if (Next) {
    Next->Prev = Prev;
  }
  Prev = nullptr;
  Next = nullptr;

  // The head is unchanged unless this node was it. Then the new head is found
  // from the tail, the one other node known to be still in the list.
  if (head == this) {
    if (tail == this) {
      return nullptr;
    }
    return &tail->Head_Of_List();
  }
  return head;
}

}  // namespace game
