// HSV to RGB color conversion, shared by both games' palette code.
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

// Divides and rounds to nearest, with halves rounding up.
//
// Both arguments must be non-negative and 'denominator' must be non-zero.
// This reproduces the original WWLIB Divide_With_Round; the palette
// conversions depend on its exact rounding.
constexpr int DivideWithRound(int numerator, int denominator) {
  return numerator / denominator +
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
      value * (kMaxComponent - DivideWithRound(saturation *
                                                   (kMaxComponent - fraction),
                                               kMaxComponent)),
      kMaxComponent);

  switch (sextant) {
    case 0:
      return {value, rising, minimum};
    case 1:
      return {falling, value, minimum};
    case 2:
      return {minimum, value, rising};
    case 3:
      return {minimum, falling, value};
    case 4:
      return {rising, minimum, value};
    default:  // Sextant 5.
      return {value, minimum, falling};
  }
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_HSV_H_
