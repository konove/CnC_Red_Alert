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
#include "game/core/vector.h"

#include "ra/ai/base.h"
#include "ra/ai/taction.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/tevent.h"
#include "ra/core/ccptr.h"
#include "ra/core/defines.h"
#include "ra/data/ccini.h"
#include "ra/net/session.h"
#include "ra/net/wsproto.h"
#include "ra/objects/cell.h"
#include "ra/objects/object.h"
#include "ra/screens/egos.h"

// An explicit instantiation of a game:: template must sit in a namespace
// that encloses game::, so these are at global scope.
/*
**	The following template function can be located here ONLY if all the
*instantiations are *	declared in a header file this module includes. By
*placing the template functions here, *	it speeds up compiler operation and
*reduces object module size.
*/

template class game::VectorClass<ra::CCPtr<ra::TeamTypeClass> >;
template class game::VectorClass<ra::CCPtr<ra::TriggerTypeClass> >;
template class game::VectorClass<ra::TeamMissionClass*>;
template class game::VectorClass<ra::EventChoiceClass*>;
template class game::VectorClass<ra::ActionChoiceClass*>;
template class game::VectorClass<ra::EgoClass*>;
template class game::VectorClass<ra::NodeNameTag*>;
template class game::VectorClass<ra::PhoneEntryClass*>;
template class game::VectorClass<ra::MultiMission*>;
template class game::VectorClass<ra::ObjectClass*>;
template class game::VectorClass<ra::TriggerClass*>;
template class game::VectorClass<ra::RemapControlType*>;
template class game::VectorClass<ra::BaseNodeClass>;
template class game::VectorClass<ra::CellClass>;
template class game::VectorClass<char*>;
template class game::VectorClass<unsigned char*>;
template class game::VectorClass<const char*>;
template class game::VectorClass<void*>;
template class game::VectorClass<unsigned char>;

template class game::VectorClass<ra::WinsockInterfaceClass::WinsockBufferType*>;
