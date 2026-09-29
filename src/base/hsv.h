// HSV/RGB color conversions, shared by both games' palette code.
#ifndef CNC_RED_ALERT_BASE_HSV_H_
#define CNC_RED_ALERT_BASE_HSV_H_

#include <algorithm>

namespace base {

// An RGB color with 8-bit components, each in the range [0, 255].
struct Rgb8 {
  int red;
  int green;
  int blue;
};

// An HSV color with 8-bit components, each in the range [0, 255]. See
// HsvToRgb8() for the hue scale.
struct Hsv8 {
  int hue;
  int saturation;
  int value;
};

// Divides and rounds to nearest, with halves rounding up.
//
// Both arguments must be non-negative and 'denominator' must be non-zero.
// This reproduces the original WWLIB Divide_With_Round; the palette
// conversions depend on its exact rounding.
constexpr int DivideWithRound(const int numerator, const int denominator) {
  return (numerator / denominator) +
         (numerator % denominator >= (denominator + 1) / 2 ? 1 : 0);
}

// Converts an HSV color to RGB using the hexagonal color-wheel model.
//
// All three inputs are in the range [0, 255] and are clamped if they are not.
// The returned components are in the same range; callers that need 6-bit VGA
// guns scale the result down themselves.
//
// Example:
//   const base::Rgb8 color = base::HsvToRgb8(hue, saturation, value);
constexpr Rgb8 HsvToRgb8(int hue, int saturation, int value) {
  constexpr int kMaxComponent = 255;

  hue = std::clamp(hue, 0, kMaxComponent);
  saturation = std::clamp(saturation, 0, kMaxComponent);
  value = std::clamp(value, 0, kMaxComponent);

  // Scaling the hue by six splits the circle into its six sextants: the
  // quotient picks the sextant, the remainder is the position within it. The
  // maximum hue lands exactly on sextant 6, which is sextant 0 one full turn
  // around, so the modulo both closes the circle and keeps the index in [0, 5]
  // — which is what makes the switch below total.
  const int scaled_hue = hue * 6;
  const int sextant = scaled_hue / kMaxComponent % 6;
  const int fraction = scaled_hue % kMaxComponent;

  // Within any sextant one gun sits at the full 'value', one at the
  // desaturated 'minimum', and the third ramps between them — 'rising' as the
  // hue advances through the sextant, 'falling' as it leaves.
  const int minimum =
      DivideWithRound(value * (kMaxComponent - saturation), kMaxComponent);
  const int falling = DivideWithRound(
      value * (kMaxComponent -
               DivideWithRound(saturation * fraction, kMaxComponent)),
      kMaxComponent);
  const int rising = DivideWithRound(
      value * (kMaxComponent -
               DivideWithRound(saturation * (kMaxComponent - fraction),
                               kMaxComponent)),
      kMaxComponent);

  switch (sextant) {
    case 0:
      return {.red = value, .green = rising, .blue = minimum};
    case 1:
      return {.red = falling, .green = value, .blue = minimum};
    case 2:
      return {.red = minimum, .green = value, .blue = rising};
    case 3:
      return {.red = minimum, .green = falling, .blue = value};
    case 4:
      return {.red = rising, .green = minimum, .blue = value};
    default:  // Sextant 5.
      return {.red = value, .green = minimum, .blue = falling};
  }
}

// Converts an RGB color to HSV, the inverse of HsvToRgb8().
//
// All three inputs must be in the range [0, 255]; the returned components are
// in the same range. The value is the brightest gun and the saturation the
// share of it the dimmest gun lacks. A grey (or black) has hue and saturation
// 0. Every division rounds, so HsvToRgb8() of the result is within one step of
// the input at the 6-bit palette scale.
//
// Example:
//   const auto [hue, saturation, value] = base::Rgb8ToHsv(red, green, blue);
constexpr Hsv8 Rgb8ToHsv(const int red, const int green, const int blue) {
  constexpr int kMaxComponent = 255;

  // The dimmest gun is the amount of white mixed in.
  const auto [white, value] = std::minmax({red, green, blue});

  // A grey has no saturation, and so no hue.
  if (value == white) {
    return {.hue = 0, .saturation = 0, .value = value};
  }

  const int spread = value - white;
  const int saturation = DivideWithRound(spread * kMaxComponent, value);

  // Each gun's distance below the brightest, as a fraction of the spread
  // scaled to 0..kMaxComponent. The dimmest gun's is kMaxComponent, the
  // brightest's 0.
  const int red_shortfall =
      DivideWithRound((value - red) * kMaxComponent, spread);
  const int green_shortfall =
      DivideWithRound((value - green) * kMaxComponent, spread);
  const int blue_shortfall =
      DivideWithRound((value - blue) * kMaxComponent, spread);

  // Which gun is brightest and which dimmest picks the sextant of the color
  // hexagon, and the middle gun's shortfall the position within it. The hue is
  // built at six times its final scale, one kMaxComponent per sextant.
  int hue = 0;
  if (value == red) {
    hue = white == green ? (5 * kMaxComponent) + blue_shortfall
                         : (1 * kMaxComponent) - green_shortfall;
  } else if (value == green) {
    hue = white == blue ? (1 * kMaxComponent) + red_shortfall
                        : (3 * kMaxComponent) - blue_shortfall;
  } else {
    hue = white == red ? (3 * kMaxComponent) + green_shortfall
                       : (5 * kMaxComponent) - red_shortfall;
  }

  // Back to one kMaxComponent for the whole circle. A red whose green and
  // blue tie takes the magenta branch and lands on 6 * kMaxComponent, which
  // becomes kMaxComponent, the same hue as 0.
  return {
      .hue = DivideWithRound(hue, 6), .saturation = saturation, .value = value};
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_HSV_H_
