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

/* $Header: /CounterStrike/LISTNODE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LISTNODE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/16/96 *
 *                                                                                             *
 *                  Last Update : May 16, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef LISTNODE_H
#define LISTNODE_H

#include <cassert>

/*
**	This is a doubly linked list node. Typical use of this node is to derive
**	objects from this node. The interface class for this node can be used
*for *	added convenience.
*/
class GenericList;
class GenericNode {
 public:
  GenericNode() : NextNode(nullptr), PrevNode(nullptr) {}
  virtual ~GenericNode() { Unlink(); }
  GenericNode(GenericNode& node) { node.Link(this); }
  GenericNode& operator=(GenericNode& node) {
    if (&node != this) {
      node.Link(this);
    }
    return *this;
  }

  void Unlink() {
    if (Is_Valid()) {
      assert(PrevNode != nullptr);
      assert(NextNode != nullptr);

      PrevNode->NextNode = NextNode;
      NextNode->PrevNode = PrevNode;
      PrevNode = nullptr;
      NextNode = nullptr;
    }
  }

  GenericList* Main_List() const {
    GenericNode const* node = this;
    while (node->PrevNode) {
      node = PrevNode;
    }
    return (GenericList*)this;
  }
  void Link(GenericNode* node) {
    assert(node != nullptr);
    node->Unlink();
    node->NextNode = NextNode;
    node->PrevNode = this;
    if (NextNode) NextNode->PrevNode = node;
    NextNode = node;
  }

  GenericNode* Next() const { return NextNode; }
  GenericNode* Prev() const { return PrevNode; }
  bool Is_Valid() const { return NextNode != nullptr && PrevNode != nullptr; }

 protected:
  GenericNode* NextNode;
  GenericNode* PrevNode;
};

/*
**	This is a generic list handler. It manages N generic nodes. Use the
*interface class *	to the generic list for added convenience.
*/
class GenericList {
 public:
  GenericList() { FirstNode.Link(&LastNode); }

  GenericNode* First() const { return FirstNode.Next(); }
  GenericNode* Last() const { return LastNode.Prev(); }
  bool Is_Empty() const { return !FirstNode.Next()->Is_Valid(); }
  void Add_Head(GenericNode* node) { FirstNode.Link(node); }
  void Add_Tail(GenericNode* node) { LastNode.Prev()->Link(node); }
  void Delete() {
    GenericNode* node = FirstNode.Next();
    while (node->Is_Valid()) {
      GenericNode* next = node->Next();
      delete node;
      node = next;
    }
  }

 protected:
  GenericNode FirstNode;
  GenericNode LastNode;

 private:
  GenericList(GenericList& list);
  GenericList& operator=(GenericList const&);
};

/*
**	This node class serves only as an "interface class" for the normal node
**	object. In order to use this interface class you absolutely must be sure
**	that the node is the root base object of the "class T". If it is true
*that the *	address of the node is the same as the address of the "class T",
*then this *	interface class will work. You can usually ensure this by
*deriving the *	class T object from this node.
*/
template <class T>
class List;
template <class T>
class Node : public GenericNode {
 public:
  List<T>* Main_List() const { return (List<T>*)GenericNode::Main_List(); }
  T* Next() const { return (T*)GenericNode::Next(); }
  T* Prev() const { return (T*)GenericNode::Prev(); }
};

/*
**	This is an "interface class" for a list of nodes. The rules for the
*class T object *	are the same as the requirements required of the node
*class.
*/
template <class T>
class List : public GenericList {
 public:
  T* First() const { return (T*)GenericList::First(); }
  T* Last() const { return (T*)GenericList::Last(); }
};

#endif
