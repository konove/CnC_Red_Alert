#include "ra/ui/ui_skin.h"

#include <cstddef>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/defines.h"

namespace ra {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

}  // namespace ra
