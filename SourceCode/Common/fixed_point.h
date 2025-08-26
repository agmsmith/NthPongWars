/******************************************************************************
 * Nth Pong Wars, fixed_point.h for fixed point math operations.
 *
 * I wanted to use 10.6 (10 bits of integer, 6 bits of fraction, total 16 bits)
 * math for our velocities and positions (screens up to 1024 pixels wide,
 * fractions in steps of 1/32) but on the Z80 processor, shifting bits to get
 * at the integer portion is quite slow (one instruction per single bit shift),
 * so it's better to align on byte boundaries.  A 3 byte fixed number (2 bytes
 * integer, 1 byte fraction) would be ideal.  But that's hard to get a C
 * compiler to do it natively, so we're using 4 byte numbers (16 bits of
 * integer, 16 bits of fraction).  If speed isn't enough, we may do the 3 byte
 * number format in Z80 assembler and 4 bytes on more modern processors.
 *
 * Addition and subtraction are done as plain integers, just the interpretation
 * of the bits as integer with fraction is different.  Multiplication and
 * division would need some extra scaling.
 *
 * AGMS20240718 - Start this header file.
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
#ifndef _FIXED_POINT_H
#define _FIXED_POINT_H 1

#include <sys/types.h> /* For __uint16_t, __int32_t, u16_t, uint32_t etc. */
#include <endian.h> /* For __BYTE_ORDER, __LITTLE_ENDIAN */

/*******************************************************************************
 * Constants, types.
 */

/* Our floating point data type "fx", can be interpreted as a 32 bit integer
   to do math (addition, subtraction) or parts can be extracted, or converted
   to or from a 32 bit float (very slowly).

   Later we may change it to 3 bytes by reducing the size of the fraction,
   but then addition and subtraction will need some custom assembler code.  So
   write the game code in a subroutine call style (using #defines instead of
   actual subroutines for now) and avoid using + and - directly on the 32 bit
   integers in higher level code.
*/
typedef struct fx_bits_struct {
  /* Fields are ordered so that they combine 16 bit values into a 32 bit
     integer of the same endianness.  So we can use a single 32 bit add
     to add both fraction and integer in one shot.

     Note that fractional parts are unsigned, and may be weird if the whole
     number is negative (take the absolute value of the whole fx, then look at
     the fractional part). */
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t fraction;
    int16_t integer;
  #else /* __BIG_ENDIAN */
    int16_t integer;
    uint16_t fraction;
  #endif
} fx_bits;

typedef union fx_union {
  /* So you can treat the whole thing as an integer to do additions in one shot,
     but best not to use this in game code in case we switch to 3 byte fx. */
  int32_t as_int32;

  /* Or access individual bytes in the number. */
  uint8_t as_bytes[4];

  fx_bits portions;
} fx, *pfx;

/* Related constants. */
#define _MAX_FX_FRACTION 0xFFFF
#define _MAX_FX_INT 0x7FFF
#define _MAX_FX_POSITIVE(x) {x.as_int32 = 0x7FFFFFFF;}
#define _FX_UNITY_FLOAT ((float) (((int32_t) 1) << 16))
extern fx gfx_Constant_Zero;
extern fx gfx_Constant_One;
extern fx gfx_Constant_MinusOne;
extern fx gfx_Constant_Eighth;
extern fx gfx_Constant_MinusEighth;

/* Setting and getting.  Mostly inline code, limited by 8 bit compilers. */
extern void COPY_FX(pfx x, pfx y); /* Copy value of X to Y. */
extern void SWAP_FX(pfx x, pfx y); /* Exchange values of X and Y. */
#define FLOAT_TO_FX(fpa, x) {x.as_int32 = fpa * _FX_UNITY_FLOAT;}
#define GET_FX_FRACTION(x) (x.portions.fraction)
#define GET_FX_INTEGER(x) (x.portions.integer)
#define GET_FX_FLOAT(x) (x.as_int32 / _FX_UNITY_FLOAT)
#define INT_TO_FX(inta, x) {x.portions.integer = inta; x.portions.fraction = 0; }
#define INT_FRACTION_TO_FX(inta, fracb, x) {x.portions.integer = inta; x.portions.fraction = fracb; }
#define ZERO_FX(x) {x.as_int32 = 0; }

/* Negate, done by subtracting from 0 and overwriting the value. */
extern void NEGATE_FX(pfx x);

/* Negate and copy in one step.  Y is set to -X. */
extern void COPY_NEGATE_FX(pfx x, pfx y);

/* Add fx values x and y and put the result in fx value z (which can safely
   overwrite x or y if it is the same address as them). */
extern void ADD_FX(pfx x, pfx y, pfx z);

/* Put fx value x - y into z (which can safely overwrite x or y even if they
   have the same address as z). */
extern void SUBTRACT_FX(pfx x, pfx y, pfx z);

/* Put fx absolute value of x into x. */
extern void ABS_FX(pfx x);

/* Copies the absolute value of x into y. */
extern void COPY_ABS_FX(pfx x, pfx y);

/* IS_NEGATIVE_FX(pfx x) returns TRUE if the number is negative. */
#if __BYTE_ORDER == __LITTLE_ENDIAN
  #define IS_NEGATIVE_FX(x) ((x)->as_bytes[3] & 0x80)
#else /* Big endian. */
  #define IS_NEGATIVE_FX(x) ((x)->as_bytes[0] & 0x80)
#endif

/* Compare two values X & Y, return a small integer (so it can be returned in
   a register rather than on the stack) which is -1 if X < Y, zero if X = Y,
   +1 if X > Y. */
extern int8_t COMPARE_FX(pfx x, pfx y);

/* Compare value X against zero and return a small integer which is
   -1 if X < 0, zero if X == 0, +1 if X > 0. */
extern int8_t TEST_FX(pfx x);

/* Divide the FX by two.  Same as shifting the given value arithmetic right
   (sign bit extended, so works with negative numbers too) by one bit.  1 bit
   is extra efficient in that we can shift directly in memory.
*/
extern void DIV2_FX(pfx x);

/* Divide the FX by two to the Nth.  Same as shifting the given value arithmetic
   right by N bits (sign bit extended, so works with negative numbers too) */
extern void DIV2Nth_FX(pfx x, uint8_t n);

/* Divide a by 4 and put into b. */
#define DIV4_FX(a, b) {b.as_int32 = a.as_int32 / 4; }

/* Divide a by 256 and put into b. */
#define DIV256_FX(a, b) {b.as_int32 = a.as_int32 / 256; }

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

extern uint8_t VECTOR_FX_TO_OCTANT(pfx vector_x, pfx vector_y);
extern uint8_t INT16_TO_OCTANT(int16_t vector_x, int16_t vector_y);

#endif /* _FIXED_POINT_H */

