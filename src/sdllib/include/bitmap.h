#ifndef CNC_RED_ALERT_SDLLIB_BITMAP_H_
#define CNC_RED_ALERT_SDLLIB_BITMAP_H_

class BitmapClass {
 public:
  BitmapClass(int w, int h, unsigned char* data)
      : Width(w), Height(h), Data(data) {}

  int Width;
  int Height;
  unsigned char* Data;
};

class TPoint2D {
 public:
  TPoint2D(int xx, int yy) : x(xx), y(yy) {}
  TPoint2D() : x(0), y(0) {}

  int x;
  int y;
};

#endif  // CNC_RED_ALERT_SDLLIB_BITMAP_H_
