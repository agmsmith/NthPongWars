/******************************************************************************
 * Nth Pong Wars, fixed_point.h for fixed point math operations.
 *
 * I want to use 11.5 (11 bits of integer, 5 bits of fraction, total 16 bits)
 * math for our velocities and positions.  That means screens up to 1024 pixels
 * wide (plus a sign bit), and slowest speed at 1/32 of a pixel per update, so
 * players can move as slow as one pixel per second.  If bit shifting is too
 * slow I could use 3 byte numbers with an 8 bit fraction.  But that's hard to
 * get a C compiler to do natively, so we're defaulting to 4 byte numbers
 * (16 bits of integer, 16 bits of fraction) for the generic version of this
 * math code.
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

#ifdef NABU_H /* Z80, which is little endian, 5 bit fraction, 11 bit integer. */
  #define FX_BITS_WHOLE 16
  #define FX_BYTES_WHOLE 2
  #define FX_BITS_INT 11
  #define FX_BITS_FRACTION 5
  #define MAX_FX_FRACTION 0x1F /* Internal use, fraction is unsigned int. */
  #define MAX_FX_INT 0x03FF /* Internal use, max positive integer. */
  typedef int16_t fx_whole_integer; /* The whole FX number fits this integer. */
#else /* More generic code, int and fractional parts are both 16 bits. */
  #define FX_BITS_WHOLE 32
  #define FX_BYTES_WHOLE 4
  #define FX_BITS_INT 16
  #define FX_BITS_FRACTION 16
  #define MAX_FX_FRACTION 0xFFFF /* Internal use, fraction is unsigned int. */
  #define MAX_FX_INT 0x7FFF /* Internal use, max positive integer. */
  typedef int32_t fx_whole_integer;
#endif
#define FX_UNITY_FLOAT ((float) (((int32_t) 1) << FX_BITS_FRACTION))

/* Our floating point data type "fx", can be interpreted as an integer
   to do math (addition, subtraction) or parts can be extracted, or converted
   to or from a 32 bit float (very slowly).

   Write the game code in a macro call style (using #defines instead of
   actual subroutines unless the generated code is long enough to make a
   common subroutine to save space over time) and avoid using + and - directly
   on the fx as integers in higher level code.
*/
typedef struct fx_bits_struct {
  /* Fields are ordered so that they combine fraction and whole portions into
     a full size integer of the same endianness.  So we can use a single
     integer add to add both fraction and integer in one shot.

     Note that fractional parts are unsigned, and may be weird if the whole
     number is negative (take the absolute value of the whole fx, then look at
     the fractional part). */
  #ifdef NABU_H /* Z80, which is little endian and only using 16 bits total. */
    unsigned int fraction : FX_BITS_FRACTION; /* SDCC compiler doesn't pack */
    unsigned int int_low : (8 - FX_BITS_FRACTION); /* bits across bytes, */
    int int_high : 8; /* so integer part is split up somewhat awkwardly. */
  #else /* More generic code. */
    #if __BYTE_ORDER == __LITTLE_ENDIAN
      uint16_t fraction;
      int16_t integer;
    #else /* __BIG_ENDIAN */
      int16_t integer;
      uint16_t fraction;
    #endif
  #endif
} fx_bits;


typedef union fx_union {
  /* So you can treat the whole thing as an integer to do additions in one shot,
     but best not to use this in game code in case we switch to 3 byte fx or
     some other custom integer format. */
  fx_whole_integer as_int;

  /* Or access individual bytes in the number. */
  uint8_t as_bytes[FX_BYTES_WHOLE];

  /* Or access the fraction and integer separately. */
  fx_bits portions;
} fx, *pfx;

/* Related constants. */
extern fx gfx_Constant_Zero;
extern fx gfx_Constant_One;
extern fx gfx_Constant_MinusOne;
extern fx gfx_Constant_Eighth;
extern fx gfx_Constant_MinusEighth;

/* Setting and getting.  Mostly inline code, limited by 8 bit compilers. */
extern void COPY_FX(pfx x, pfx y); /* Copy value of X to Y. */
extern void SWAP_FX(pfx x, pfx y); /* Exchange values of X and Y. */
#define FLOAT_TO_FX(fpa, x) {x.as_int = fpa * FX_UNITY_FLOAT;}
#define GET_FX_FRACTION(x) (x.portions.fraction)

#ifdef NABU_H
  #define GET_FX_INTEGER(x) (x.as_int >> FX_BITS_FRACTION)
#else /* More generic 16.16 fixed point implementation. */
  #define GET_FX_INTEGER(x) (x.portions.integer)
#endif

#define GET_FX_FLOAT(x) (x.as_int / FX_UNITY_FLOAT)

#ifdef NABU_H
  #define INT_TO_FX(inta, x) {x.as_int = (inta << FX_BITS_FRACTION); }
#else /* More generic 16.16 fixed point implementation. */
  #define INT_TO_FX(inta, x) {x.portions.integer = inta; x.portions.fraction = 0; }
#endif

#ifdef NABU_H
  #define INT_FRACTION_TO_FX(inta, fracb, x) {x.as_int = (inta << FX_BITS_FRACTION) | (fracb); }
#else /* More generic 16.16 fixed point implementation. */
  #define INT_FRACTION_TO_FX(inta, fracb, x) {x.portions.integer = inta; x.portions.fraction = fracb; }
#endif

#define ZERO_FX(x) {x.as_int = 0; }

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
#ifdef NABU_H
  #define IS_NEGATIVE_FX(x) ((x)->portions.int_high & 0x80)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
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
#define DIV4_FX(a, b) {b.as_int = a.as_int / 4; }

/* Divide a by 256 and put into b. */
#define DIV256_FX(a, b) {b.as_int = a.as_int / 256; }

/* Multiply by 4 and return the integer portion. */
extern fx_whole_integer MUL4INT_FX(pfx x);

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

