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

#include "ra/ai/trigger_type.h"

#include <array>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/core/object_heap.h"
#include "game/ui/standard_colors.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/assets/palettes.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/objects/type.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

TriggerType::TriggerType()
    : AbstractTypeClass(RTTI_TRIGGERTYPE,
                        TheObjectHeaps().trigger_type().SlotIndexOf(this),
                        TXT_NONE, "x") {}

TriggerType::~TriggerType() = default;

void* TriggerType::operator new(std::size_t /*unused*/) noexcept {
  void* memory = TheObjectHeaps().trigger_type().Allocate();
  if (memory) {
    static_cast<TriggerType*>(memory)->is_active = true;
  }

  return memory;
}

void TriggerType::operator delete(void* memory) {
  if (memory) {
    static_cast<TriggerType*>(memory)->is_active = false;
  }
  TheObjectHeaps().trigger_type().Free(static_cast<TriggerType*>(memory));
}

void TriggerType::Detach(const Target target) {
  if (target.kind() == RTTI_TEAMTYPE) {
    for (TEventClass* event : {&event1, &event2}) {
      if (event->Team == target.AsTeamType()) {
        event->Team = nullptr;
      }
    }
  }
  action1->Detach(target);
  action2->Detach(target);
}

std::string TriggerType::Description() const {
  if (!config::kCheatKeysEnabled && !config::kScenarioEditorEnabled) {
    return {};
  }
  // Marks for how the second event and action are used: '.' for unused.
  char event_mark = '.';
  switch (event_control) {
    case MULTI_AND:
      event_mark = '&';
      break;

    case MULTI_OR:
      event_mark = '|';
      break;

    case MULTI_LINKED:
      event_mark = '=';
      break;

    case MultiStyle::MULTI_ONLY:
    default:
      break;
  }

  char action_mark = '.';
  if (action_control == MULTI_AND) {
    action_mark = '&';
  }

  // The first event's value, when it has one worth showing.
  std::string value_text;
  switch (Event_Needs(event1.Event)) {
    case NEED_NUMBER:
      value_text = absl::StrCat(event1.Data.Value);
      break;

    case NEED_UNIT:
      value_text =
          TextString(UnitTypeClass::As_Reference(event1.Data.Unit).Full_Name());
      break;

    case NEED_AIRCRAFT:
      value_text = TextString(
          AircraftTypeClass::As_Reference(event1.Data.Aircraft).Full_Name());
      break;

    case NEED_STRUCTURE:
      value_text = TextString(
          BuildingTypeClass::As_Reference(event1.Data.Structure).Full_Name());
      break;

    case NEED_INFANTRY:
      value_text = TextString(
          InfantryTypeClass::As_Reference(event1.Data.Infantry).Full_Name());
      break;

    case NeedType::NEED_NONE:
    case NeedType::NEED_THEME:
    case NeedType::NEED_MOVIE:
    case NeedType::NEED_SOUND:
    case NeedType::NEED_SPEECH:
    case NeedType::NEED_TRIGGER:
    case NeedType::NEED_WAYPOINT:
    case NeedType::NEED_TEAM:
    case NeedType::NEED_HOUSE:
    case NeedType::NEED_TIME:
    case NeedType::NEED_QUARRY:
    case NeedType::NEED_FORMATION:
    case NeedType::NEED_BOOL:
    case NeedType::NEED_SPECIAL:
    case NeedType::NEED_MISSION:
    case NeedType::NEED_HEX_NUMBER:
    default:
      break;
  }

  // Persistence mark: V(olatile), S(emi-persistent) or P(ersistent).
  char persistence_mark = 'V';
  if (persistence == SEMIPERSISTANT) {
    persistence_mark = 'S';
  }
  if (persistence == PERSISTANT) {
    persistence_mark = 'P';
  }

  return absl::StrFormat("%4.4s\t %s %c%c%c  %s%s", IniName,
                         HouseTypeClass::As_Reference(house).Suffix,
                         persistence_mark, event_mark, action_mark,
                         Name_From_Event(event1.Event), value_text);
}

AttachType TriggerType::AttachesTo() const {
  AttachType attach = ra::Attaches_To(event1.Event);

  if (event_control != MULTI_ONLY) {
    attach = attach | ra::Attaches_To(event2.Event);
  }
  return attach;
}

void TriggerType::ReadIni(const engine::file::IniFile& ini) {
  // An action may name a trigger type that comes later in the section, so
  // every trigger type is created and named before any entry is read.
  struct Pending {
    TriggerType* trigger_type;
    std::string_view name;
    std::string_view entry;
  };
  std::vector<Pending> pending;
  for (const auto& [name, entry] : ini.Entries(kIniSection)) {
    auto* const trigger_type = base::NewPooled<TriggerType>();
    // The scenario lists more trigger types than the heap holds; the rest
    // are dropped.
    if (trigger_type == nullptr) {
      break;
    }
    trigger_type->Set_Name(name);
    pending.push_back(
        {.trigger_type = trigger_type, .name = name, .entry = entry});
  }

  for (const Pending& read : pending) {
    read.trigger_type->ReadIniEntry(read.name, read.entry);
  }
}

void TriggerType::ReadIniEntry(const std::string_view name,
                               const std::string_view entry) {
  DCHECK_HEAP_SLOT(TheObjectHeaps().trigger_type(), this);

  Set_Name(name);

  base::Tokenizer tokens(entry);
  persistence =
      static_cast<Persistence>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  house = static_cast<HousesType>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  event_control =
      static_cast<MultiStyle>(base::ParseIntegerOr<int>(tokens.Next(), 0));
  action_control =
      static_cast<MultiStyle>(base::ParseIntegerOr<int>(tokens.Next(), 0));

  event1.Read_INI(tokens);
  event2.Read_INI(tokens);
  action1 = MakeTriggerAction(ReadTriggerActionRecord(tokens));
  action2 = MakeTriggerAction(ReadTriggerActionRecord(tokens));
}

void TriggerType::WriteIni(engine::file::IniFile& ini) {
  // [Triggers] is where older formats kept them; a file with both would
  // describe the triggers twice.
  ini.Clear("Triggers");
  ini.Clear(kIniSection);

  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    const TriggerType* trigger_type =
        TheObjectHeaps().trigger_type().ActiveAt(index);
    ini.PutString(kIniSection, trigger_type->IniName,
                  trigger_type->ToIniEntry());
  }
}

std::string TriggerType::ToIniEntry() const {
  // The enums are stored as their numbers, which ReadIniEntry() casts back.
  std::string entry =
      std::format("{},{},{},{},", std::to_underlying(persistence),
                  std::to_underlying(house), std::to_underlying(event_control),
                  std::to_underlying(action_control));

  event1.Build_INI_Entry(entry);
  entry += ',';

  event2.Build_INI_Entry(entry);
  entry += ',';

  entry += ra::ToIniEntry(action1->ToRecord());
  entry += ',';

  entry += ra::ToIniEntry(action2->ToRecord());
  return entry;
}

void TriggerType::Draw_It(PixelView& view, int /*unused*/, const int x,
                          const int y, const int width, const int height,
                          const bool selected, const FontType font,
                          TextPrintType flags) const {
  if (!config::kCheatKeysEnabled && !config::kScenarioEditorEnabled) {
    return;
  }
  RemapControlType* scheme = Get_Color_Scheme();
  // Tab stops in pixels: Description() puts a tab after the name.
  static constexpr std::array<int, 2> kTabStops = {13, 40};
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
                            game::kTransparentBlack, font, flags, width,
                            kTabStops);
  } else {
    Conquer_Clip_Text_Print(
        view, Description().c_str(), x, y,
        (selected ? &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE)
                  : &ThePalettes().color_remaps().at(PCOLOR_GREY)),
        game::kTransparentBlack, font, flags, width, kTabStops);
  }
}

void TriggerType::DestroyAll() {
  // Not FreeAll(): a trigger type owns its two actions.
  TheObjectHeaps().trigger_type().DestroyAll();
}

TriggerType* TriggerType::FromName(const std::string_view name) {
  if (!name.empty()) {
    for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
         index++) {
      TriggerType* const type = TheObjectHeaps().trigger_type().ActiveAt(index);
      if (absl::EqualsIgnoreCase(type->Name(), name)) {
        return type;
      }
    }
  }
  return nullptr;
}

}  // namespace ra
