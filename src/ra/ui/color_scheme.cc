#include "ra/ui/color_scheme.h"

#include "ra/core/defines.h"

namespace ra {

namespace {

RemapControlType default_color_scheme = {15, 0, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0};
RemapControlType* color_scheme = &default_color_scheme;

}  // namespace

RemapControlType* Get_Color_Scheme() { return color_scheme; }

void Set_Color_Scheme(RemapControlType* scheme) { color_scheme = scheme; }

}  // namespace ra
