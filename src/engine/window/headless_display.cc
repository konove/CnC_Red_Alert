#include "engine/window/headless_display.h"

#include <SDL_pixels.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/ico_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display.h"
#include "engine/window/display_client.h"

namespace engine::window {

HeadlessDisplay::HeadlessDisplay() = default;

HeadlessDisplay::HeadlessDisplay(DisplayClient& client) : Display(client) {}

HeadlessDisplay::~HeadlessDisplay() {
  if (palette_surface_ != nullptr) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
  }
}

bool HeadlessDisplay::Init(const char* /*title*/, int /*width*/,
                           int /*height*/) {
  return true;
}

void HeadlessDisplay::SetIcon(engine::gfx::IcoImage /*icon*/) {}

bool HeadlessDisplay::SetVideoMode(const int width, const int height) {
  ResetVideoMode();
  // Creating a surface needs no SDL subsystem.
  palette_surface_ = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
  return palette_surface_ != nullptr;
}

void HeadlessDisplay::ResetVideoMode() {
  if (palette_surface_ != nullptr) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
    palette_surface_ = nullptr;
  }
}

std::optional<PixelSurface::Pixels> HeadlessDisplay::Lock() {
  auto* surface = static_cast<SDL_Surface*>(palette_surface_);
  if (surface == nullptr) {
    return std::nullopt;
  }
  // A surface made by SDL_CreateRGBSurface owns pitch bytes for each row.
  const auto bytes =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint8_t*>(surface->pixels),
                base::ToSize(surface->pitch) * base::ToSize(surface->h));
  return Pixels{.bytes = bytes, .pitch = surface->pitch};
}

void HeadlessDisplay::Unlock() {}

void HeadlessDisplay::SetPalette(const engine::gfx::Palette& palette) {
  if (palette_surface_ == nullptr) {
    return;
  }
  SDL_Palette* sdl_palette =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_palette->colors, base::ToSize(sdl_palette->ncolors));
  for (int i = 0;
       i < std::min(sdl_palette->ncolors, engine::gfx::Palette::kColorCount);
       i++) {
    SDL_Color& color = base::At(colors, base::ToSize(i));
    color.r = static_cast<Uint8>(palette.at(i).red());
    color.g = static_cast<Uint8>(palette.at(i).green());
    color.b = static_cast<Uint8>(palette.at(i).blue());
  }
}

const void* HeadlessDisplay::palette() const {
  if (palette_surface_ == nullptr) {
    return nullptr;
  }
  return static_cast<SDL_Surface*>(palette_surface_)->format->palette;
}

void HeadlessDisplay::EndFrame() {
  if (palette_surface_ == nullptr) {
    return;
  }
  NotifyFrameEnd();
  PumpEvents();
}

void HeadlessDisplay::PresentScaledFrame(const std::span<const uint8_t> frame,
                                         const int width, const int height) {
  if (width <= 0 || height <= 0 ||
      base::ToSize(width) > frame.size() / base::ToSize(height)) {
    return;
  }
  EndFrame();
}

void HeadlessDisplay::PresentBackdrop(const std::span<const uint8_t> frame,
                                      const int width, const int height) {
  PresentScaledFrame(frame, width, height);
}

void HeadlessDisplay::PumpEvents() {
  NotifyPump();
  client().Pump();
}

}  // namespace engine::window
