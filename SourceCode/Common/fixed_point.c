/******************************************************************************
 * Nth Pong Wars, fixed_point.c for fixed point math operations.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fixed_point.h"

/* Constants for global use.  Don't change them!  Game main() sets them up. */
fx gfx_Constant_Zero;
fx gfx_Constant_One;
fx gfx_Constant_MinusOne;
fx gfx_Constant_Eighth;
fx gfx_Constant_MinusEighth;


/* Convert a 2D vector into an octant angle direction.  Returns octant number
   in lower 3 bits of the result.  Result high bit is set if the vector is
   exactly on the angle, else zero.

   Find out which octant the vector is in.  It will actually be between two
   octant directions, somewhere in a 45 degree segment.  So we'll come up with
   lower and upper angles, with the upper one being 45 degrees clockwise from
   the lower one, and we'll just return the lower one.  If it is exactly on an
   octant angle, that will become the lower angle and we'll set the high bit
   in the result to show that.

           6
       5  -y   7
     4  -x o +x  0
       3  +y   1
           2
*/
uint8_t VECTOR_FX_TO_OCTANT(pfx vector_x, pfx vector_y)
{
  int8_t xDir, yDir;
  uint8_t octantLower = 0xFF; /* An invalid value you should never see. */
  bool rightOnOctant = false; /* If velocity is exactly in octant direction. */

  xDir = TEST_FX(*vector_x);
  yDir = TEST_FX(*vector_y);

  if (xDir == 0) /* X == 0 */
  {
    if (yDir == 0) /* Y == 0 */
      octantLower = 0; /* Actually undefined, zero length vector. */
    else if (yDir >= 0) /* Y > 0, using >= test since it's faster. */
    {
      octantLower = 2;
      rightOnOctant = true;
    }
    else /* Y < 0 */
    {
      octantLower = 6;
      rightOnOctant = true;
    }
  }
  else if (xDir >= 0) /* X > 0 */
  {
    if (yDir == 0) /* X > 0, Y == 0 */
    {
      octantLower = 0;
      rightOnOctant = true;
    }
    else if (yDir >= 0) /* X > 0, Y > 0 */
    {
      int8_t xyDelta;
      xyDelta = COMPARE_FX(*vector_x, *vector_y);
      if (xyDelta > 0) /* |X| > |Y| */
        octantLower = 0;
      else /* |X| <= |Y| */
      {
        octantLower = 1;
        rightOnOctant = (xyDelta == 0);
      }
    }
    else /* X > 0, Y < 0 */
    {
      int8_t xyDelta;
      fx negativeY;
      COPY_NEGATE_FX(*vector_y, negativeY);
      xyDelta = COMPARE_FX(*vector_x, negativeY);
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 7;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 6;
    }
  }
  else /* X < 0 */
  {
    if (yDir == 0) /* X < 0, Y == 0 */
    {
      octantLower = 4;
      rightOnOctant = true;
    }
    else if (yDir >= 0) /* X < 0, Y >= 0 */
    {
      int8_t xyDelta;
      fx negativeX;
      COPY_NEGATE_FX(*vector_x, negativeX);
      xyDelta = COMPARE_FX(negativeX, *vector_y);
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 3;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 2;
    }
    else /* X < 0, Y < 0 */
    {
      int8_t xyDelta;
      xyDelta = COMPARE_FX(*vector_x, *vector_y);
      if (xyDelta < 0) /* |X| > |Y| */
        octantLower = 4;
      else /* |X| <= |Y| */
      {
        octantLower = 5;
        rightOnOctant = (xyDelta == 0);
      }
    }
  }

  if (rightOnOctant)
    octantLower |= 0x80;
  return octantLower;
}


/* Same function, but with int16_t pixel values as the vector coordinates. */
uint8_t INT16_TO_OCTANT(int16_t vector_x, int16_t vector_y)
{
  uint8_t octantLower = 0xFF; /* An invalid value you should never see. */
  bool rightOnOctant = false; /* If velocity is exactly in octant direction. */

  if (vector_x == 0) /* X == 0 */
  {
    if (vector_y == 0) /* Y == 0 */
      octantLower = 0; /* Actually undefined, zero length vector. */
    else if (vector_y >= 0) /* Y > 0, using >= test since it's faster. */
    {
      octantLower = 2;
      rightOnOctant = true;
    }
    else /* Y < 0 */
    {
      octantLower = 6;
      rightOnOctant = true;
    }
  }
  else if (vector_x >= 0) /* X > 0 */
  {
    if (vector_y == 0) /* X > 0, Y == 0 */
    {
      octantLower = 0;
      rightOnOctant = true;
    }
    else if (vector_y >= 0) /* X > 0, Y > 0 */
    {
      int16_t xyDelta;
      xyDelta = vector_x - vector_y;
      if (xyDelta > 0) /* |X| > |Y| */
        octantLower = 0;
      else /* |X| <= |Y| */
      {
        octantLower = 1;
        rightOnOctant = (xyDelta == 0);
      }
    }
    else /* X > 0, Y < 0 */
    {
      int16_t xyDelta;
      xyDelta = vector_x + vector_y;
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 7;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 6;
    }
  }
  else /* X < 0 */
  {
    if (vector_y == 0) /* X < 0, Y == 0 */
    {
      octantLower = 4;
      rightOnOctant = true;
    }
    else if (vector_y >= 0) /* X < 0, Y >= 0 */
    {
      int16_t xyDelta;
      xyDelta = -vector_x - vector_y;
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 3;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 2;
    }
    else /* X < 0, Y < 0 */
    {
      int16_t xyDelta;
      xyDelta = vector_x - vector_y;
      if (xyDelta < 0) /* |X| > |Y| */
        octantLower = 4;
      else /* |X| <= |Y| */
      {
        octantLower = 5;
        rightOnOctant = (xyDelta == 0);
      }
    }
  }

  if (rightOnOctant)
    octantLower |= 0x80;
  return octantLower;
}
