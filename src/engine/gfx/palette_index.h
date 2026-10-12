#ifndef CNC_RED_ALERT_ENGINE_GFX_PALETTE_INDEX_H_
#define CNC_RED_ALERT_ENGINE_GFX_PALETTE_INDEX_H_

// The number of a palette slot, as its own type.

#include <cstdint>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"

namespace engine::gfx {

// The number of a palette slot, 0..255: what one pixel of a page holds, and
// what a drawing function is told to draw with.
//
// It is a type of its own so that a count, a shape number or a player-color id
// cannot be passed where a color is expected. It has no arithmetic and no
// ordering; code that means a neighboring slot or a range of slots says so
// through value(). Pixel data, remap tables and fading tables stay bytes.
//
// Example:
//   constexpr engine::gfx::PaletteIndex kHealthGreen{4};
//   view.FillRect(bar, kHealthGreen);
//   const auto shaded = kHealthGreen.RemappedBy(fading_table);
class PaletteIndex {
 public:
  // Slot 0.
  constexpr PaletteIndex() = default;

  constexpr explicit PaletteIndex(const uint8_t value) noexcept
      : value_(value) {}

  // Returns the slot `value`, for a color that was computed as an int. Fails in
  // every build if `value` is outside 0..255.
  [[nodiscard]] static constexpr PaletteIndex FromInt(const int value) {
    CHECK(value >= 0 && value <= kMaxValue) << "palette index " << value;
    return PaletteIndex(static_cast<uint8_t>(value));
  }

  // The slot as a number, for arithmetic and for indexing a table.
  [[nodiscard]] constexpr int value() const { return value_; }

  // The slot as the byte a pixel or a table entry stores.
  [[nodiscard]] constexpr uint8_t byte() const { return value_; }

  // True for slot 0, which a transparent blit and a text background skip.
  [[nodiscard]] constexpr bool IsTransparent() const { return value_ == 0; }

  // Returns the slot `table`, a remap or fading table, draws in place of this
  // one.
  [[nodiscard]] constexpr PaletteIndex RemappedBy(
      const std::span<const uint8_t, 256> table) const {
    return PaletteIndex(base::At(table, int{value_}));
  }

  // Transitional (docs/PALETTE_INDEX_PLAN.md): lets a PaletteIndex reach the
  // parameters and variables that are still int. Task 7 of the plan removes it.
  // NOLINTNEXTLINE(google-explicit-constructor,misc-explicit-constructor)
  constexpr operator int() const { return value_; }

  friend constexpr bool operator==(PaletteIndex, PaletteIndex) = default;

  // Saved as one byte, like the uint8_t it replaces.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(value_);
  }

 private:
  static constexpr int kMaxValue = 255;

  uint8_t value_{0};
};

// A page of PaletteIndex-sized pixels stays one byte a pixel.
static_assert(sizeof(PaletteIndex) == 1);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_PALETTE_INDEX_H_
