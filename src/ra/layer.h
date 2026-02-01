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

/* $Header: /CounterStrike/LAYER.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LAYER.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 31, 1994 *
 *                                                                                             *
 *                  Last Update : May 31, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_LAYER_H_
#define CNC_RED_ALERT_RA_LAYER_H_

#include "ra/vector_dynamic.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class ObjectClass;

class LayerClass : public DynamicVectorClass<ObjectClass*> {
 public:
  //-----------------------------------------------------------------
  void Sort();
  bool Submit(const ObjectClass* object, bool sort = false);
  int Sorted_Add(const ObjectClass* object);

  virtual void Init() { Clear(); }
  virtual void One_Time() {}

  /*
  **	File I/O.
  */
  bool Load(Straw& file);
  bool Save(Pipe& file) const;
  virtual void Code_Pointers();
  virtual void Decode_Pointers();
};

#endif  // CNC_RED_ALERT_RA_LAYER_H_
