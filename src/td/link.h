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

/* $Header:   F:\projects\c&c\vcs\code\link.h_v   2.17   16 Oct 1995 16:45:52
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LINK.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_LINK_H_
#define CNC_RED_ALERT_TD_LINK_H_

#include "absl/base/attributes.h"

/*
**	This implements a simple linked list. It is possible to add, remove, and
*traverse the *	list. Since this is a doubly linked list, it is possible to
*remove an entry from the *	middle of an existing list.
*/
class LinkClass {
 public:
  LinkClass() noexcept;
  virtual ~LinkClass();
  LinkClass(LinkClass&&) = delete;
  LinkClass& operator=(LinkClass&&) = delete;

  [[nodiscard]] virtual LinkClass* Get_Next() const;
  [[nodiscard]] virtual LinkClass* Get_Prev() const;
  virtual LinkClass& Add(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  virtual LinkClass& Add_Tail(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  virtual LinkClass& Add_Head(LinkClass& list) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  [[nodiscard]] virtual const LinkClass& Head_Of_List() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND final;
  virtual LinkClass& Head_Of_List() ABSL_ATTRIBUTE_LIFETIME_BOUND final {
    return (LinkClass&)static_cast<const LinkClass*>(this)->Head_Of_List();
  }
  [[nodiscard]] virtual const LinkClass& Tail_Of_List() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND final;
  virtual LinkClass& Tail_Of_List() ABSL_ATTRIBUTE_LIFETIME_BOUND final {
    return (LinkClass&)static_cast<const LinkClass*>(this)->Tail_Of_List();
  }
  virtual void Zap();
  virtual LinkClass* Remove() ABSL_ATTRIBUTE_LIFETIME_BOUND;

  // Not copyable. The original copy operations did not copy: they spliced the
  // destination into the source object's list, which mutates the source. Use
  // Add() to put an object into a list.
  LinkClass(const LinkClass&) = delete;
  LinkClass& operator=(const LinkClass&) = delete;

 private:
  /*
  **	Pointers to previous and next link objects in chain.
  */
  LinkClass* Next = nullptr;
  LinkClass* Prev = nullptr;
};

#endif  // CNC_RED_ALERT_TD_LINK_H_
