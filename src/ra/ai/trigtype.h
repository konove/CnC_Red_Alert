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

// The trigger type: a scenario's description of one trigger - the events that
// spring it, the actions it then performs, the house it belongs to and how
// long it lives. TriggerClass (trigger.h) is the live instance that objects,
// cells and houses attach to. The scenario editor's dialog for a trigger type
// lives here too.

#ifndef CNC_RED_ALERT_RA_AI_TRIGTYPE_H_
#define CNC_RED_ALERT_RA_AI_TRIGTYPE_H_

#include <cstddef>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/core/defines.h"
#include "ra/objects/object.h"
#include "ra/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

// How a trigger type's second event or action relates to its first. Scenario
// INI files and saved games store the number, so the order is fixed.
enum class MultiStyleType {
  MULTI_ONLY,   // Only the first event or action counts.
  MULTI_AND,    // Both events must occur; both actions are performed.
  MULTI_OR,     // Events only: either event springs the trigger.
  MULTI_LINKED  // Events only: either event springs the trigger, and each
                // performs just its own action (event 1 -> action 1, event 2
                // -> action 2).
};
using enum MultiStyleType;

// One trigger of a scenario, as its INI file describes it. Trigger types live
// in the trigger type heap (TheObjectHeaps().trigger_type()): create one with
// base::NewPooled<TriggerTypeClass>() and destroy it with base::DeletePooled().
// The name (IniName) is at most four characters in the editor and is how the
// scenario's objects and cells name the trigger they carry.
class TriggerTypeClass : public AbstractTypeClass {
 public:
  // True while the object occupies its heap slot. operator new sets this
  // before the constructor runs; the initializer must agree with it.
  bool IsActive : 1 = true;

  // Whether a trigger of this type survives going off. Scenario INI files and
  // saved games store the number.
  enum class PersistantType {
    // The trigger destroys itself as soon as it has gone off, detaching from
    // everything it is attached to.
    VOLATILE = 0,
    // The trigger counts the objects it is attached to and springs only once
    // every one of them has set it off; then it destroys itself.
    SEMIPERSISTANT = 1,
    // The trigger never goes away and springs every time its events occur.
    PERSISTANT = 2
  };
  using enum PersistantType;

  PersistantType IsPersistant = VOLATILE;

  // The house the trigger belongs to: the one its house events are about and
  // the one its actions affect when they name no house of their own.
  HousesType House = HOUSE_SPAIN;

  // The events that spring the trigger. Event2 only counts when EventControl
  // is not MULTI_ONLY.
  TEventClass Event1;
  TEventClass Event2;
  MultiStyleType EventControl = MULTI_ONLY;

  // The actions performed when the trigger springs. Action2 is only performed
  // when ActionControl is MULTI_AND or EventControl is MULTI_LINKED.
  TriggerAction Action1;
  TriggerAction Action2;
  MultiStyleType ActionControl = MULTI_ONLY;  // MULTI_ONLY or MULTI_AND.

  TriggerTypeClass();
  ~TriggerTypeClass() override;
  TriggerTypeClass(const TriggerTypeClass&) = delete;
  TriggerTypeClass& operator=(const TriggerTypeClass&) = delete;
  TriggerTypeClass(TriggerTypeClass&&) = delete;
  TriggerTypeClass& operator=(TriggerTypeClass&&) = delete;

  // Takes a slot from the trigger type heap; returns nullptr when it is full.
  void* operator new(std::size_t /*unused*/);
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  // Returns the slot to the trigger type heap.
  void operator delete(void* ptr);

  // Destroys every trigger type, in preparation for a new scenario.
  static void Init();

  // Creates a trigger type for every entry of the [Trigs] section of `ini`.
  // Team types must be loaded first: the events and actions look their teams
  // up as they are read. The objects, cells and houses that carry a trigger
  // find its type by name afterwards, so this runs before their sections are
  // read.
  static void Read_INI(engine::file::IniFile& ini);

  // Replaces the [Trigs] section of `ini` with one entry per trigger type, and
  // clears the [Triggers] section the pre-Red Alert format used.
  static void Write_INI(engine::file::IniFile& ini);

  // Names the trigger type `name` and sets it from the INI value `entry`:
  //   persistence,house,event control,action control,event 1,event 2,
  //   action 1,action 2
  // with each event and action a comma separated group of its own (see
  // TEventClass::Read_INI and TriggerAction::ReadIni). The first four are
  // enum numbers; a missing or unreadable one reads as 0.
  void Fill_In(std::string_view name, std::string_view entry);

  // Returns the INI value Fill_In() reads.
  [[nodiscard]] std::string Build_INI_Entry() const;

  // The INI section the trigger types are stored in.
  static constexpr std::string_view INI_Name() { return "Trigs"; }

  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  // Drops the references this trigger type's events and actions hold to the
  // team type or trigger type `target`, which is being destroyed. `all` is not
  // used.
  void Detach(Target target, bool all = true);

  // Returns what triggers of this type can be attached to: the union of what
  // its events allow, the second event counting only when it is in use.
  [[nodiscard]] AttachType Attaches_To() const;

  // Returns the trigger type named `name`, ignoring case, or nullptr when
  // there is none or `name` is empty.
  static TriggerTypeClass* From_Name(std::string_view name);

  // Runs the scenario editor's dialog on this trigger type. Returns true when
  // the user confirmed, with the dialog's values stored in the object; false
  // when the user cancelled, leaving it as it was apart from an unset house,
  // which becomes HOUSE_GOOD either way.
  bool Edit();

  // Returns a one-line summary for the editor's trigger list: name, house,
  // persistence, event and action control marks, and the first event with its
  // value. An empty string in builds without the cheat keys or the scenario
  // editor.
  [[nodiscard]] std::string Description() const;

  // Draws Description() as a list box entry in the box at (`x`, `y`) of
  // `width` by `height` pixels, highlighted when `selected`. The entry's
  // `index` in the list is not used. Draws nothing in builds without the cheat
  // keys or the scenario editor.
  void Draw_It(PixelView& view, int index, int x, int y, int width, int height,
               bool selected, FontType font, TextPrintType flags) const;
};

extern template void TriggerTypeClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TriggerTypeClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_TRIGTYPE_H_
