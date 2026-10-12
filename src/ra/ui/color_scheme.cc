#include "ra/ui/color_scheme.h"

#include "game/ui/standard_colors.h"
#include "ra/core/defines.h"

namespace ra {

namespace {

// White for the bright color and slot 0 for everything else, until the game
// sets a scheme.
RemapControlType default_color_scheme = [] noexcept {
  RemapControlType scheme;
  scheme.BrightColor = game::kWhite;
  return scheme;
}();
RemapControlType* color_scheme = &default_color_scheme;

}  // namespace

RemapControlType* Get_Color_Scheme() { return color_scheme; }

void Set_Color_Scheme(RemapControlType* scheme) { color_scheme = scheme; }

}  // namespace ra
