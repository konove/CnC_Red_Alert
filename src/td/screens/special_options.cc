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

#include "td/screens/special_options.h"

#include <array>
#include <memory>
#include <vector>

#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/core/special_flags.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/event.h"
#include "td/net/network.h"
#include "td/net/session.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/ui/checkbox.h"
#include "td/ui/dialog.h"
#include "td/ui/textbtn.h"

namespace td {

using enum engine::window::KeyNumber;

namespace {

// One flag the dialog offers. A bit-field has no address, hence the accessor
// pair.
struct Option {
  int label;  // String-table number of the text beside the check box.
  bool (*get)(const SpecialFlags& flags);
  void (*set)(SpecialFlags& flags, bool on);
};

// The flags the dialog offers, from the top check box down. The defender's
// advantage and Tiberium growing and spreading are not offered.
constexpr std::array<Option, 10> kOptions = {{
    {.label = TXT_SEPARATE_HELIPAD,
     .get =
         [](const SpecialFlags& flags) { return flags.separate_helipad != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.separate_helipad = on; }},
    {.label = TXT_VISIBLE_TARGET,
     .get = [](const SpecialFlags& flags) { return flags.visible_target != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.visible_target = on; }},
    {.label = TXT_TREE_TARGET,
     .get = [](const SpecialFlags& flags) { return flags.tree_target != 0; },
     .set = [](SpecialFlags& flags, const bool on) { flags.tree_target = on; }},
    {.label = TXT_MCV_DEPLOY,
     .get = [](const SpecialFlags& flags) { return flags.mcv_deploy != 0; },
     .set = [](SpecialFlags& flags, const bool on) { flags.mcv_deploy = on; }},
    {.label = TXT_SMART_DEFENCE,
     .get = [](const SpecialFlags& flags) { return flags.smart_defense != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.smart_defense = on; }},
    {.label = TXT_THREE_POINT,
     .get =
         [](const SpecialFlags& flags) { return flags.three_point_turns != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.three_point_turns = on; }},
    {.label = TXT_TIBERIUM_FAST,
     .get = [](const SpecialFlags& flags) { return flags.tiberium_fast != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.tiberium_fast = on; }},
    {.label = TXT_ROAD_PIECES,
     .get = [](const SpecialFlags& flags) { return flags.no_road_pieces != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.no_road_pieces = on; }},
    {.label = TXT_SCATTER,
     .get = [](const SpecialFlags& flags) { return flags.scatter != 0; },
     .set = [](SpecialFlags& flags, const bool on) { flags.scatter = on; }},
    {.label = TXT_SHOW_NAMES,
     .get =
         [](const SpecialFlags& flags) { return flags.named_civilians != 0; },
     .set = [](SpecialFlags& flags,
               const bool on) { flags.named_civilians = on; }},
}};

}  // namespace

void RunSpecialOptionsDialog() {
  // The dialog, centred on the 320x200 screen the game was laid out for.
  constexpr int kDialogWidth = 236;
  constexpr int kDialogHeight = 162;
  constexpr int kDialogX = (320 - kDialogWidth) / 2;
  constexpr int kDialogY = (200 - kDialogHeight) / 2;

  // OK sits in the bottom left corner and Cancel in the bottom right.
  constexpr int kOkButtonX = kDialogX + 5;
  constexpr int kCancelButtonX = kDialogX + kDialogWidth - 50;
  constexpr int kButtonY = kDialogY + kDialogHeight - 15;

  // The check boxes form a column, each with its label to the right.
  constexpr int kCheckBoxX = kDialogX + 7;
  constexpr int kFirstCheckBoxY = kDialogY + 20;
  constexpr int kCheckBoxSpacing = 10;
  constexpr int kLabelOffsetX = 10;  // From the box's left edge.

  // Gadget ids; Input() reports a clicked button as ButtonKey(id). The check
  // boxes count up from kFirstCheckBoxId.
  constexpr int kFirstCheckBoxId = 100;
  constexpr int kOkButtonId = 200;
  constexpr int kCancelButtonId = 201;

  // A copy the dialog edits. The installed flags change only when the event OK
  // queues is executed.
  SpecialFlags edited_flags = TheSpecialFlags();

  TextButtonClass ok_button(kOkButtonId, TXT_OK, FontType::k6PointGradient,
                            TPF_NOSHADOW, kOkButtonX, kButtonY);
  TextButtonClass cancel_button(kCancelButtonId, TXT_CANCEL,
                                FontType::k6PointGradient, TPF_NOSHADOW,
                                kCancelButtonX, kButtonY);
  game::GadgetList gadgets;
  gadgets.Add(ok_button);
  gadgets.AddAfter(cancel_button, ok_button);

  // One check box per option, in kOptions order. A box toggles itself when
  // clicked, so the flags are only read back from the boxes on OK.
  std::vector<std::unique_ptr<CheckBoxClass>> check_boxes;
  for (int index = 0; index < std::ssize(kOptions); ++index) {
    CheckBoxClass& check_box =
        *check_boxes.emplace_back(std::make_unique<CheckBoxClass>(
            static_cast<unsigned>(kFirstCheckBoxId + index), kCheckBoxX,
            kFirstCheckBoxY + (index * kCheckBoxSpacing)));
    gadgets.AddAfter(check_box, ok_button);
    if (kOptions.at(base::ToSize(index)).get(edited_flags)) {
      check_box.Turn_On();
    }
  }

  TheMap().Override_Mouse_Shape(MOUSE_NORMAL);
  PixelView& view = TheScreen().visible_view();
  bool redraw = true;
  bool process = true;
  while (process) {
    // A multiplayer game cannot pause for a dialog: the other players' frames
    // keep coming, so the game runs on behind it. Main_Loop() returns true
    // when that game is over, which closes the dialog without applying
    // anything.
    if (TheSession().type() == GAME_NORMAL) {
      Call_Back();
    } else {
      if (Main_Loop()) {
        process = false;
      }
    }

    if (redraw) {
      redraw = false;

      game::TheMouse().Hide();
      Dialog_Box(view, kDialogX, kDialogY, kDialogWidth, kDialogHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kDialogX, kDialogY, kDialogWidth);

      for (int index = 0; index < std::ssize(kOptions); ++index) {
        const CheckBoxClass& check_box = *check_boxes.at(base::ToSize(index));
        Fancy_Text_Print(view, kOptions.at(base::ToSize(index)).label,
                         check_box.X + kLabelOffsetX, check_box.Y, kCcGreen,
                         game::kTransparentBlack, FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      gadgets.DrawAll(view);
      game::TheMouse().Show();
    }

    switch (static_cast<int>(gadgets.Input(view).key)) {
      case engine::window::ButtonKey(kOkButtonId):
        process = false;
        for (int index = 0; index < std::ssize(kOptions); ++index) {
          kOptions.at(base::ToSize(index))
              .set(edited_flags, check_boxes.at(base::ToSize(index))->IsOn);
        }
        // Queued as an event rather than assigned, so that every machine
        // changes its flags on the same frame.
        TheNetwork().out_list().Add(EventClass(edited_flags));
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kCancelButtonId):
        process = false;
        break;

      // No input, a check box (which has already toggled itself) or a key.
      default:
        break;
    }
  }

  TheMap().Revert_Mouse_Shape();
  TheScreen().hidden_page().view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}

}  // namespace td
