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

/* $Header:   F:\projects\c&c\vcs\code\vector.cpv   2.17   16 Oct 1995 16:49:26
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VECTOR.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 02/19/95 *
 *                                                                                             *
 *                  Last Update : July 18, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "game/core/vector.h"
#include "game/core/vector_dynamic.h"
#include "td/ai/base.h"
#include "td/ai/trigger.h"
#include "td/net/nodename.h"
#include "td/net/phone.h"
#include "td/objects/cell.h"
#include "td/objects/object.h"

// An explicit instantiation of a game:: template must sit in a namespace
// that encloses game::, so these are at global scope.
template class game::VectorClass<td::NodeNameTag*>;
template class game::VectorClass<td::PhoneEntryClass*>;
template class game::VectorClass<td::ObjectClass*>;
template class game::VectorClass<td::TriggerClass*>;
template class game::VectorClass<td::BaseNodeClass>;
template class game::VectorClass<td::CellClass>;
template class game::VectorClass<char>;
template class game::VectorClass<int>;
template class game::VectorClass<char*>;
template class game::VectorClass<unsigned char*>;
template class game::VectorClass<const char*>;
template class game::VectorClass<void*>;
template class game::VectorClass<unsigned char>;
template class game::DynamicVectorClass<td::NodeNameTag*>;
template class game::DynamicVectorClass<td::PhoneEntryClass*>;
template class game::DynamicVectorClass<td::ObjectClass*>;
template class game::DynamicVectorClass<td::TriggerClass*>;
template class game::DynamicVectorClass<td::BaseNodeClass>;
template class game::DynamicVectorClass<char>;
template class game::DynamicVectorClass<int>;
template class game::DynamicVectorClass<char*>;
template class game::DynamicVectorClass<unsigned char*>;
template class game::DynamicVectorClass<const char*>;
template class game::DynamicVectorClass<void*>;
