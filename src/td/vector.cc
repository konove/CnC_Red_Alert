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

#include "td/vector.h"

#include <algorithm>
#include <new>

#include "td/base.h"
#include "td/cell.h"
#include "td/loaddlg.h"
#include "td/nodename.h"
#include "td/phone.h"
#include "td/vector_impl.h"

template class VectorClass<NodeNameTag*>;
template class VectorClass<PhoneEntryClass*>;
template class VectorClass<ObjectClass*>;
template class VectorClass<TriggerClass*>;
template class VectorClass<FileEntryClass*>;
template class VectorClass<BaseNodeClass>;
template class VectorClass<CellClass>;
template class VectorClass<char>;
template class VectorClass<int>;
template class VectorClass<char*>;
template class VectorClass<unsigned char*>;
template class VectorClass<const char*>;
template class VectorClass<void*>;
template class VectorClass<unsigned char>;
template class DynamicVectorClass<NodeNameTag*>;
template class DynamicVectorClass<PhoneEntryClass*>;
template class DynamicVectorClass<ObjectClass*>;
template class DynamicVectorClass<TriggerClass*>;
template class DynamicVectorClass<FileEntryClass*>;
template class DynamicVectorClass<BaseNodeClass>;
template class DynamicVectorClass<char>;
template class DynamicVectorClass<int>;
template class DynamicVectorClass<char*>;
template class DynamicVectorClass<unsigned char*>;
template class DynamicVectorClass<const char*>;
template class DynamicVectorClass<void*>;
