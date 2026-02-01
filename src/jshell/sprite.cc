/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/SPRITE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/**********************************************************************

        Sprite.cpp

        Dec 28,1995

        GraphicBufferClass member functions for blitting, scaling,
        and rotating bitmaps

**********************************************************************/

#include "base/trig.h"
#include "sdllib/bitmap.h"
#include "sdllib/gbuffer.h"

/***************************************************************
 *
 *	Scale_Rotate
 *
 *	FUNCTION:
 *
 *	Using Bi-Linear Interpolation, draws a scaled and rotated
 *	bitmap onto the buffer.  No clipping is performed so beware.
 *
 *	INPUTS
 *
 *	bmp		- bitmap to draw
 *	pt			- desired position of the center
 *	scale		- 24.8 fixed point scale factor
 *	angle		- 8bit angle (0=0deg, 255=360deg)
 *
 ***************************************************************/

void GraphicBufferClass::Scale_Rotate(BitmapClass& bmp, const TPoint2D& pt,
                                      long scale, unsigned char angle) {
  unsigned int scrpos;
  unsigned int temp;

  int i, j;         // counter vars
  int pxerror = 0;  // these three vars will be used in an
  int pyerror = 0;  // integer difference alg to keep track
  int pixpos = 0;   // of what pixel to draw.
  unsigned char pixel;

  TPoint2D p0;  // "upper left" corner of the rectangle
  TPoint2D p1;  // "upper right" corner of the rectangle
  TPoint2D p2;  // "lower left" corner of the rectangle

  /*-------------------------------------------------
          Compute three corner points of the rectangle
  -------------------------------------------------*/
  {
    angle &= 0x0FF;
    long c = base::kCos256[angle];
    long s = base::kSin256[angle];
    long W = (scale * bmp.Width) >> 1;
    long L = (scale * bmp.Height) >> 1;

    p0.x = pt.x + ((((L * c) >> 7) - ((W * s) >> 7)) >> 8);
    p0.y = pt.y + (((-(L * s) >> 7) - ((W * c) >> 7)) >> 8);
    p1.x = pt.x + ((((L * c) >> 7) + ((W * s) >> 7)) >> 8);
    p1.y = pt.y + (((-(L * s) >> 7) + ((W * c) >> 7)) >> 8);
    p2.x = pt.x + (((-(L * c) >> 7) - ((W * s) >> 7)) >> 8);
    p2.y = pt.y + ((((L * s) >> 7) - ((W * c) >> 7)) >> 8);
  }

  /*-----------------------------------
          Initialize Breshnam constants
  -----------------------------------*/

  // This breshnam line goes across the FRONT of the rectangle
  // In the bitmap, this will step from left to right

  int f_deltax = p1.x - p0.x;
  int f_deltay = p1.y - p0.y;
  int f_error = 0;
  int f_xstep = 1;
  int f_ystep = Width;

  // This breshnam line goes down the SIDE of the rectangle
  // In the bitmap, this line will step from top to bottom

  int s_deltax = p2.x - p0.x;
  int s_deltay = p2.y - p0.y;
  int s_error = 0;
  int s_xstep = 1;
  int s_ystep = Width;

  /*--------------------------------
          fixup deltas and step values
  --------------------------------*/

  if (f_deltay < 0) {
    f_deltay = -f_deltay;
    f_ystep = -Width;
  }

  if (f_deltax < 0) {
    f_deltax = -f_deltax;
    f_xstep = -1;
  }

  if (s_deltay < 0) {
    s_deltay = -s_deltay;
    s_ystep = -Width;
  }

  if (s_deltax < 0) {
    s_deltax = -s_deltax;
    s_xstep = -1;
  }

  scrpos = p0.x + Width * p0.y;  // address of initial screen pos.
  temp = scrpos;

  /*---------------------------------------------------------------------
          Now all of the differences, errors, and steps are set up so we can
          begin drawing the bitmap...

          There are two cases here,
          1 - the "Front" line has a slope of <  1.0 (45 degrees)
          2 - the "Front" line has a slope of >= 1.0

          For case 1, we step along the X direction, for case 2, step in y
  ---------------------------------------------------------------------*/

  if (f_deltax > f_deltay) {  // CASE 1, step front in X, side in Y

    // outer loop steps from top to bottom of the rectangle
    for (j = 0; j < s_deltay; j++) {
      temp = scrpos;

      // The inner loop steps across the rectangle
      for (i = 0; i < f_deltax; i++) {
        pixel = bmp.Data[pixpos];  // read pixel
        if (pixel) {
          static_cast<unsigned char*>(Get_Buffer())[scrpos] =
              pixel;  // draw if not transparent
        }
        //				if (pixel) Data[scrpos]=pixel;	//draw
        // if not transparent
        pxerror += bmp.Width;  // update position in bitmap
        while (pxerror > f_deltax) {
          pixpos++;
          pxerror -= f_deltax;
        }
        scrpos += f_xstep;  // step to next screen pos
        f_error += f_deltay;
        if (f_error > f_deltax) {
          if (pixel) {
            static_cast<unsigned char*>(Get_Buffer())[scrpos] = pixel;
          }
          f_error -= f_deltax;
          scrpos += f_ystep;
        }
      }
      pxerror = 0;
      pixpos -= bmp.Width - 1;
      pyerror += bmp.Height;

      while (pyerror > s_deltay) {
        pixpos += bmp.Width;
        pyerror -= s_deltay;
      }

      f_error = 0;
      scrpos = temp;
      scrpos += s_ystep;
      s_error += s_deltax;

      if (s_error > s_deltay) {
        s_error -= s_deltay;
        scrpos += s_xstep;
      }
    }

  } else {  // CASE 2, Step front line in X, side line in Y

    // outer loop steps from top to bottom of the rectangle
    for (j = 0; j < s_deltax; j++) {
      temp = scrpos;

      // The inner loop steps across the rectangle
      for (i = 0; i < f_deltay; i++) {
        pixel = bmp.Data[pixpos];  // read pixel
        if (pixel) {
          static_cast<unsigned char*>(Get_Buffer())[scrpos] =
              pixel;  // draw if not transparent
        }
        pxerror += bmp.Width;  // update position in bitmap
        while (pxerror > f_deltay) {
          pixpos++;
          pxerror -= f_deltay;
        }

        scrpos += f_ystep;  // step to next screen pos
        f_error += f_deltax;
        if (f_error > f_deltay) {
          if (pixel) {
            static_cast<unsigned char*>(Get_Buffer())[scrpos] = pixel;
          }
          f_error -= f_deltay;
          scrpos += f_xstep;
        }
      }

      pxerror = 0;
      pixpos -= bmp.Width - 1;
      pyerror += bmp.Height;
      while (pyerror > s_deltax) {
        pixpos += bmp.Width;
        pyerror -= s_deltax;
      }

      scrpos = temp;
      scrpos += s_xstep;
      s_error += s_deltay;
      f_error = 0;
      if (s_error > s_deltax) {
        s_error -= s_deltax;
        scrpos += s_ystep;
      }
    }
  }
}
