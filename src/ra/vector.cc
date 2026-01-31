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
#include "ra/vector.h"

#include "ra/base.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/defines.h"
#include "ra/egos.h"
#include "ra/loaddlg.h"
#include "ra/object.h"
#include "ra/session.h"
#include "ra/taction.h"
#include "ra/teamtype.h"
#include "ra/tevent.h"
#include "ra/wsproto.h"

/*
**	The following template function can be located here ONLY if all the
*instantiations are *	declared in a header file this module includes. By
*placing the template functions here, *	it speeds up compiler operation and
*reduces object module size.
*/

template class VectorClass<CCPtr<TeamTypeClass> >;
template class VectorClass<CCPtr<TriggerTypeClass> >;
template class VectorClass<TeamMissionClass*>;
template class VectorClass<EventChoiceClass*>;
template class VectorClass<ActionChoiceClass*>;
template class VectorClass<EgoClass*>;
template class VectorClass<NodeNameTag*>;
template class VectorClass<PhoneEntryClass*>;
template class VectorClass<MultiMission*>;
template class VectorClass<ObjectClass*>;
template class VectorClass<TriggerClass*>;
template class VectorClass<FileEntryClass*>;
template class VectorClass<RemapControlType*>;
template class VectorClass<BaseNodeClass>;
template class VectorClass<CellClass>;
template class VectorClass<char*>;
template class VectorClass<unsigned char*>;
template class VectorClass<const char*>;
template class VectorClass<void*>;
template class VectorClass<unsigned char>;

#ifdef WINSOCK_IPX
template class VectorClass<WinsockInterfaceClass::WinsockBufferType*>;
#endif
