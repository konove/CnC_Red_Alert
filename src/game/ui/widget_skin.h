// File: WidgetSkin, what a game supplies to draw the game::*<Skin> widgets.

#ifndef CNC_RED_ALERT_GAME_UI_WIDGET_SKIN_H_
#define CNC_RED_ALERT_GAME_UI_WIDGET_SKIN_H_

#include <concepts>
#include <cstddef>
#include <span>

#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"

namespace game {

// A game's look, as static functions the widget templates call. The widgets
// keep the logic both games share; the skin draws each part the way its game
// always has.
//
// The widget templates keep their virtual members although the
// portability-template-virtual-member-function check reports them wherever a
// translation unit names a specialization without emitting its vtable. The
// hazard it guards against cannot occur: this concept is the whole of what
// the overrides use of `Skin`, so every skin it admits instantiates every one
// of them.
template <class Skin>
concept WidgetSkin =
    requires(PixelView& view, std::span<const std::byte> shapes, int n) {
      { Skin::Draw_Shape(view, shapes, n, n, n) } -> std::same_as<void>;
    };

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_WIDGET_SKIN_H_
