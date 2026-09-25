// File: PixelSurface, pixels that a PixelBuffer draws into but does not own.

#ifndef CNC_RED_ALERT_SDLLIB_PIXEL_SURFACE_H_
#define CNC_RED_ALERT_SDLLIB_PIXEL_SURFACE_H_

#include <cstdint>
#include <optional>
#include <span>

// 8-bit paletted pixels that belong to someone else and exist only while they
// are locked; each lock may hand them out at a different address. The window
// is the one surface the games draw on, and Display implements it.
//
// A PixelBuffer attaches to a surface through its Init() overload and counts
// its own nested locks, so an implementation only sees the outermost Lock()
// and its matching Unlock(), never two locks at once.
//
// Example:
//   PixelBuffer page;
//   page.Init(640, 400, TheDisplay());
//   page.view().Clear();  // Locks the surface, clears it, unlocks it.
class PixelSurface {
 public:
  // The pixels of a locked surface.
  struct Pixels {
    // Every byte of the surface, `pitch` bytes to a row.
    std::span<uint8_t> bytes;
    // Bytes from the start of one row to the start of the next.
    int pitch = 0;
  };

  PixelSurface() = default;
  virtual ~PixelSurface() = default;
  PixelSurface(const PixelSurface&) = delete;
  PixelSurface& operator=(const PixelSurface&) = delete;
  PixelSurface(PixelSurface&&) = delete;
  PixelSurface& operator=(PixelSurface&&) = delete;

  // Locks the surface and returns its pixels, valid until Unlock(). Returns
  // nullopt if there are no pixels to lock or they could not be locked, in
  // which case Unlock() must not be called.
  virtual std::optional<Pixels> Lock() = 0;
  // Gives back the pixels the last successful Lock() returned.
  virtual void Unlock() = 0;
};

#endif  // CNC_RED_ALERT_SDLLIB_PIXEL_SURFACE_H_
