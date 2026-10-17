// File: Screenshot_Key(), the screenshot on K the gadget list offers every
// key to.

#ifndef CNC_RED_ALERT_RA_UI_SCREENSHOT_H_
#define CNC_RED_ALERT_RA_UI_SCREENSHOT_H_

#include "engine/window/keyboard.h"

namespace ra {

// For game::GadgetList::SetKeyHook(): on K, in a build with cheat keys
// and in developer or playtest mode outside the map editor and motion
// capture, saves the visible page as the first free scrshtNN.pcx and beeps.
void Screenshot_Key(engine::window::KeyNumber key);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_UI_SCREENSHOT_H_
