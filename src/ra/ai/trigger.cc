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

/* $Header: /CounterStrike/TRIGGER.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRIGGER.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/12/94 *
 *                                                                                             *
 *                  Last Update : August 13, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Find_Or_Make -- Find or create a trigger of the type specified.
 ** TriggerClass::As_Target -- Converts trigger to a target value *
 *   TriggerClass::Attaches_To -- Determines what trigger can attach to. *
 *   TriggerClass::Description -- Fetch a one line ASCII description of the
 *trigger.           * TriggerClass::Detach -- Detach specified target from this
 *trigger.                        * TriggerClass::Draw_It -- Draws this trigger
 *as if it were part of a list box.             * TriggerClass::Init -- clears
 *triggers for new scenario                                    *
 *   TriggerClass::Spring -- Spring the trigger (possibly). *
 *   TriggerClass::TriggerClass -- constructor * TriggerClass::operator delete
 *-- Returns a trigger to the special memory pool.            *
 *   TriggerClass::operator new -- 'new' operator * TriggerClass::~TriggerClass
 *-- Destructor for trigger objects.                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/ai/trigger.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include "absl/log/check.h"
#include "base/algorithm.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "game/core/object_heap.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/palettes.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/map/world.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/tracker.h"
#include "ra/startup/game_state.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

/***********************************************************************************************
 * TriggerClass::Description -- Fetch a one line ASCII description of the
 *trigger.             *
 *                                                                                             *
 *    In the rare (possibly never?) case of requiring a description for this
 *trigger, then     * just have the model class generate a description and
 *return that.                        *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with an ASCII description of this trigger. *
 *                                                                                             *
 * WARNINGS:   see TriggerType::Description() *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
std::string TriggerClass::Description() const { return Class->Description(); }

/***********************************************************************************************
 * TriggerClass::Draw_It -- Draws this trigger as if it were part of a list box.
 **
 *                                                                                             *
 *    This routine is called when the trigger has been assigned to a list box.
 *It will         * display a description of the trigger. *
 *                                                                                             *
 * INPUT:   see below... *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerClass::Draw_It(PixelView& view, int /*unused*/, int x, int y,
                           int width, int height, bool selected, FontType font,
                           TextPrintType flags) const {
  if constexpr (config::kCheatKeysEnabled || config::kScenarioEditorEnabled) {
    RemapControlType* scheme = Get_Color_Scheme();
    static const int _tabs[] = {13, 40};
    if (font == FontType::k6PointGradient || font == FontType::kEditor) {
      if (selected) {
        flags = flags | TPF_BRIGHT_COLOR;
        view.FillRect(x, y, x + width - 1, y + height - 1, scheme->Shadow);
      } else {
        if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
          flags = flags | TPF_MEDIUM_COLOR;
        }
      }

      Conquer_Clip_Text_Print(view, Description().c_str(), x, y, scheme,
                              kTBlack, font, flags, width, _tabs);
    } else {
      Conquer_Clip_Text_Print(
          view, Description().c_str(), x, y,
          (selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                    : &ThePalettes().color_remaps().at(PCOLOR_GREY)),
          kTBlack, font, flags, width, _tabs);
    }
  }
}

/***********************************************************************************************
 * TriggerClass::TriggerClass -- constructor *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
TriggerClass::TriggerClass(TriggerType* trigtype)
    : RTTI(RTTI_TRIGGER),
      ID(TheObjectHeaps().trigger().SlotIndexOf(this)),
      Class(trigtype) {
  Class->event1.Reset(Event1);
  Class->event2.Reset(Event2);
}

TriggerClass::TriggerClass()
    : RTTI(RTTI_TRIGGER), ID(TheObjectHeaps().trigger().SlotIndexOf(this)) {}

/***********************************************************************************************
 * TriggerClass::~TriggerClass -- Destructor for trigger objects. *
 *                                                                                             *
 *    This destructor will update the house blockage value if necessary. No
 *other action need  * be performed on trigger destruction. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
TriggerClass::~TriggerClass() {
  if ((TheGameState().active() && Class != nullptr &&
       base::Any(Class->AttachesTo() & ATTACH_GENERAL)) &&
      (TheWorld().logic_trigger_id() >=
       base::IndexOf(TheWorld().logic_triggers(), this))) {
    TheWorld().logic_trigger_id()--;
    if (TheWorld().logic_trigger_id() < 0 &&
        std::ssize(TheWorld().logic_triggers()) == 0) {
      TheWorld().logic_trigger_id() = 0;
    }
  }

  if ((TheGameState().active() && Class != nullptr &&
       base::Any(Class->AttachesTo() & ATTACH_MAP)) &&
      (TheWorld().map_trigger_id() >=
       base::IndexOf(TheWorld().map_triggers(), this))) {
    TheWorld().map_trigger_id()--;
    if (TheWorld().map_trigger_id() < 0 &&
        std::ssize(TheWorld().map_triggers()) == 0) {
      TheWorld().map_trigger_id() = 0;
    }
  }

  if (TheGameState().active() && Class != nullptr &&
      Class->house != HOUSE_NONE && Class->action1.type == TACTION_ALLOWWIN) {
    if (HouseClass::As_Pointer(Class->house)->Blockage) {
      HouseClass::As_Pointer(Class->house)->Blockage--;
    }
    HouseClass::As_Pointer(Class->house)
        ->BorrowedTime.Set(int64_t{kTicksPerSecond} * 4);
  }
  ID = -1;
}

/***********************************************************************************************
 * TriggerClass::Init -- clears triggers for new scenario *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::Init() { TheObjectHeaps().trigger().FreeAll(); }

/***********************************************************************************************
 * TriggerClass::Spring -- Spring the trigger (possibly). *
 *                                                                                             *
 *    This routine is called when a potential trigger even has occurred. The
 *event is matched  * with the trigger event needed by this trigger. If the
 *condition warrants, the trigger    * action is performed. *
 *                                                                                             *
 * INPUT:   event    -- The event that is occurring. *
 *                                                                                             *
 *          obj      -- If the trigger is attached to an object, this points to
 *the object.    *
 *                                                                                             *
 *          cell     -- If the trigger is attached to a cell, this is the cell
 *number.         *
 *                                                                                             *
 *          forced   -- Should the trigger be forced to execute regardless of
 *the event?       *
 *                                                                                             *
 * OUTPUT:  bool; Was the trigger sprung? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1996 JLB : Created. * 08/13/1996 JLB : Linked triggers
 *supported.                                               *
 *=============================================================================================*/
bool TriggerClass::Spring(TEventType event, ObjectClass* obj, ra::Cell cell,
                          bool forced) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().trigger(), this);

  const bool e1 = Class->event1(Event1, event, Class->house, obj, forced);
  bool e2 = false;
  bool execute = false;

  /*
  **	Forced triggers must presume that the cell parameter is invalid. It then
  **	uses the embedded cell value in the trigger as the official location
  *where *	the trigger will detonate at.
  */
  if (forced) {
    cell = Cell;
  } else {
    /*
    **	Determine if the trigger event is considered to have been sprung
    *according to the *	event control value. This might require that both events
    *be triggered, one event *	triggered, or either event triggered to activate
    *the trigger action.
    */
    switch (Class->event_control) {
      case MULTI_ONLY:
        execute = e1;
        break;

      case MULTI_AND:
        e2 = Class->event2(Event2, event, Class->house, obj, forced);
        execute = e1 && e2;
        break;

      case MULTI_LINKED:
      case MULTI_OR:
        e2 = Class->event2(Event2, event, Class->house, obj, forced);
        execute = e1 || e2;
        break;
      default:
        break;
    }
  }

  /*
  **	See if the trigger is sprung with a qualifying event.
  */
  if (execute || forced) {
    /*
    **	Verify that the trigger event should really be sprung. Exceptions
    **	would include semi-persistent triggers that don't actually
    **	spring until all triggers have sprung.
    */
    if (Class->persistence == TriggerType::SEMIPERSISTANT) {
      /*
      ** Detach ourselves from the object and record that there
      **	is one less attachment to keep track of.
      */
      if (obj) {
        obj->Trigger = nullptr;
      }
      if (cell.index() != 0) {
        TheMap().at(cell).Trigger = nullptr;
      }

      /*
      ** If we're attached to more objects, don't spring; otherwise, spring.
      ** And, mark ourselves as volatile so we'll completely remove ourselves
      ** from the game after we go off.
      */
      AttachCount--;
      if (AttachCount > 0) {
        return false;
      }
    }

    /*
    **	For linked trigger events, perform the action associated with the
    *matching *	trigger event. Otherwise perform the action for both events.
    */
    bool ok = false;
    const HousesType hh = Class->house;
    if (Class->event_control == MULTI_LINKED) {
      if (e1 || forced) {
        ok |= Class->action1.Perform(hh, obj, ID, cell);
      }
      if (e2 && !forced) {
        ok |= Class->action2.Perform(hh, obj, ID, cell);
      }
    } else {
      switch (Class->action_control) {
        case MULTI_ONLY:
          ok |= Class->action1.Perform(hh, obj, ID, cell);
          break;

        case MultiStyle::MULTI_OR:
        case MultiStyle::MULTI_LINKED:
        default:
        case MULTI_AND:
          ok |= Class->action1.Perform(hh, obj, ID, cell);
          ok |= Class->action2.Perform(hh, obj, ID, cell);
          break;
      }
    }
    if (!IsActive) {
      return true;
    }

    /*
    **	If at least one action was performed, then consider this
    **	trigger to have completed and thus will be deleted if
    **	necessary.
    */
    if (ok) {
      if (Class->persistence == TriggerType::VOLATILE ||
          (Class->persistence == TriggerType::SEMIPERSISTANT &&
           AttachCount <= 1)) {
        Detach_This_From_All(As_Target(), true);
        delete this;
        return true;
      }
      /*
       **	Reset event data so that the event will
       **	repeat as necessary.
       */
      Class->event1.Reset(Event1);
      Class->event2.Reset(Event2);
    }
  }

  return false;
}

/***********************************************************************************************
 * TriggerClass::operator new -- 'new' operator *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * pointer to new trigger *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void* TriggerClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().trigger().Allocate();
  if (ptr) {
    static_cast<TriggerClass*>(ptr)->IsActive = true;
  }

  return ptr;
}

/***********************************************************************************************
 * TriggerClass::operator delete -- Returns a trigger to the special memory
 *pool.              *
 *                                                                                             *
 *    This routine will return a previously allocated trigger object back to the
 *memory        * pool from which it came. *
 *                                                                                             *
 * INPUT:   pointer  -- Pointer to the trigger to return to the memory pool. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerClass::operator delete(void* pointer) {
  if (pointer) {
    static_cast<TriggerClass*>(pointer)->IsActive = false;
  }
  TheObjectHeaps().trigger().Free(static_cast<TriggerClass*>(pointer));
}

/***********************************************************************************************
 * TriggerClass::As_Target -- Converts trigger to a target value *
 *                                                                                             *
 *    Converts the trigger object into a target identifier. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Target value *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
Target TriggerClass::As_Target() const {
  DCHECK_HEAP_SLOT(TheObjectHeaps().trigger(), this);

  return Target::Make(RTTI_TRIGGER, ID);
}

/***********************************************************************************************
 * Find_Or_Make -- Find or create a trigger of the type specified. *
 *                                                                                             *
 *    This routine is used when, given a trigger type, an actual trigger object
 *is needed.     * In this case, an existing trigger of the correct type must be
 *located, or a trigger      * object must be created. In either case, this
 *routine will return a trigger object that   * corresponds to the trigger type
 *class specified.                                         *
 *                                                                                             *
 * INPUT:   trigtype -- Pointer to the trigger type to find (or create) a
 *matching trigger     * object. *
 *                                                                                             *
 * OUTPUT:  Returns a pointer to a matching trigger object. If no more triggers
 *could be       * allocated and no matching trigger could be found, then this
 *routine will return    * NULL (a very rare case). *
 *                                                                                             *
 * WARNINGS:   This routine could return NULL. *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
TriggerClass* Find_Or_Make(TriggerType* trigtype) {
  if (!trigtype) {
    return nullptr;
  }

  for (int index = 0; index < TheObjectHeaps().trigger().Count(); index++) {
    if (trigtype == TheObjectHeaps().trigger().ActiveAt(index)->Class) {
      return TheObjectHeaps().trigger().ActiveAt(index);
    }
  }

  /*
  **	No trigger was found, so make one.
  */
  auto* trig = base::NewPooled<TriggerClass>(trigtype);
  return trig;
}

/***********************************************************************************************
 * TriggerClass::Detach -- Detach specified target from this trigger. *
 *                                                                                             *
 *    This routine is called when the specified trigger MUST be detached from
 *all references   * to it. The only reference maintained by a trigger is the
 *reference to the trigger        * type class it is modeled after. *
 *                                                                                             *
 * INPUT:   target   -- The target identifier to remove all attachments to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   You must never detach the trigger type class from a trigger. Such
 *a process     * will leave the trigger orphan and in a 'crash the game
 *immediately if used'     * state. As such, this routine will throw an
 *assertion if this is tried.          *
 *                                                                                             *
 * HISTORY: * 07/09/1996 JLB : Created. *
 *=============================================================================================*/
void TriggerClass::Detach(Target target, bool /*unused*/) const {
  if (target.kind() == RTTI_TRIGGERTYPE) {
    DCHECK(Class != target.AsTriggerType());
    //		if (Class == target.AsTriggerType()) {
    //			Class = NULL;
    //		}
  }
}

}  // namespace ra
