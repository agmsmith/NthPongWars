/******************************************************************************
 * Nth Pong Wars, fixed_point.h for fixed point math operations.
 *
 * I want to use 16.8 (16 bits of integer, 8 bits of fraction, total 24 bits)
 * math for our velocities and positions.  That means screens up to 32K pixels
 * wide (plus a sign bit), and slowest speed at 1/256 of a pixel per update, so
 * players can move as slow as one pixel per second (1/32 of a pixel wasn't
 * small enough, players were getting stuck due to rounding errors).  But that's
 * hard to get a C compiler to do natively, so we're defaulting to 4 byte
 * numbers (16 bits of integer, 16 bits of fraction) for the generic version of
 * this math code.
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

#ifdef NABU_H /* Z80, which is little endian, 8 bit fraction, 16 bit integer. */
  #define FX_BITS_WHOLE 24
  #define FX_BYTES_WHOLE 3
  #define FX_BITS_INT 16
  typedef int16_t fx_type_for_integer_part;
  #define FX_BITS_FRACTION 8
  typedef uint8_t fx_type_for_fraction_part;
  #define MAX_FX_FRACTION 0xFF /* Internal use, fraction is unsigned int. */
  #define MAX_FX_INT 0x7FFF /* Internal use, max positive integer. */
#else /* More generic code, int and fractional parts are both 16 bits. */
  #define FX_BITS_WHOLE 32
  #define FX_BYTES_WHOLE 4
  #define FX_BITS_INT 16
  typedef int16_t fx_type_for_integer_part;
  #define FX_BITS_FRACTION 16
  typedef uint16_t fx_type_for_fraction_part;
  #define MAX_FX_FRACTION 0xFFFF /* Internal use, fraction is unsigned int. */
  #define MAX_FX_INT 0x7FFF /* Internal use, max positive integer. */
  typedef int32_t fx_whole_integer; /* Can add whole FX in a plain integer. */
#endif
#define FX_UNITY_FLOAT ((float) (((int32_t) 1) << FX_BITS_FRACTION))

/* Our floating point data type "fx", can be interpreted as an integer
   to do math (addition, subtraction) or parts can be extracted, or converted
   to or from a 32 bit float (very slowly).

   Write the game code in a macro call style (using #defines instead of
   actual subroutines unless the generated code is long enough to make a
   common subroutine to save space over time) and avoid using + and - directly
   on the fx as integers in higher level code (not supported in 24 bit mode).
*/
typedef struct fx_bits_struct {
  /* Fields are ordered so that they combine fraction and whole portions into
     a full size integer of the same endianness.  So we can use a single
     integer add to add both fraction and integer in one shot.  But not on the
     Z80 without the waste of doing a 32 bit add and throwing away a byte.

     Note that fractional parts are unsigned, and may be weird if the whole
     number is negative (take the absolute value of the whole fx, then look at
     the fractional part). */
  #ifdef NABU_H /* Z80, which is little endian and only using 24 bits total. */
    fx_type_for_fraction_part fraction; /* Just 8 bits, unsigned, first since */
    fx_type_for_integer_part integer; /* little endian order. 16 bits signed. */
  #else /* More generic code.  Has 16.16 number format. */
    #if __BYTE_ORDER == __LITTLE_ENDIAN
      fx_type_for_fraction_part fraction;
      fx_type_for_integer_part integer;
    #else /* __BIG_ENDIAN */
      fx_type_for_integer_part integer;
      fx_type_for_fraction_part fraction;
    #endif
  #endif
} fx_bits;


typedef union fx_union {
  #ifndef NABU_H
  /* So you can treat the whole thing as an integer to do additions in one shot,
     but best not to use this in game code in case we switch to 3 byte fx or
     some other custom integer format. */
  fx_whole_integer as_int;
  #else /* bleeble - remove else case once no more Nabu as_int references. */
  int32_t as_int;
  #endif

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

/* Setting and getting.  Can be inline code rather than a function call. */

/* COPY_FX(x, y) - Copy value of X to Y. */
#ifdef NABU_H
  #define COPY_FX(x, y) { \
  uint8_t *px = (x).as_bytes + 0; \
  uint8_t *py = (y).as_bytes + 0; \
  *py++ = *px++; \
  *py++ = *px++; \
  *py = *px; \
  }
#else /* Generic version. */
  #define COPY_FX(x, y) { (y).as_int = (x).as_int; }
#endif

/* SWAP_FX(x, y) - Exchange values of X, Y. */
#ifdef NABU_H
#define SWAP_FX(x, y); { \
  uint8_t *px = (x).as_bytes + 0; \
  uint8_t *py = (y).as_bytes + 0; \
  uint8_t temp; \
  temp = *px ; *px = *py; *py = temp; px++; py++; \
  temp = *px ; *px = *py; *py = temp; px++; py++; \
  temp = *px ; *px = *py; *py = temp; px++; py++; \
  }
#else /* Generic version. */
#define SWAP_FX(x, y); { fx_whole_integer temp = (y).as_int; \
  (y).as_int = (x).as_int; (x).as_int = temp; }
#endif

/* FLOAT_TO_FX(fpa, x) - convert floating point number fpa to FX in x. */
#ifdef NABU_H
#define FLOAT_TO_FX(fpa, x) { \
  int32_t tempwhole = (int32_t) ((fpa) * FX_UNITY_FLOAT); \
  (x).portions.fraction = (fx_type_for_fraction_part) tempwhole; \
  (x).portions.integer = \
    (fx_type_for_integer_part) (tempwhole >> FX_BITS_FRACTION); \
  }
#else /* Generic version. */
#define FLOAT_TO_FX(fpa, x) { (x).as_int = (fpa) * FX_UNITY_FLOAT; }
#endif

#define GET_FX_FRACTION(x) ((x).portions.fraction)
#define GET_FX_INTEGER(x) ((x).portions.integer)

/* GET_FX_FLOAT(x) - get the floating point value of x. */
#ifdef NABU_H
#define GET_FX_FLOAT(x) ( \
  ((x).portions.fraction + \
  ((int32_t) (x).portions.integer << FX_BITS_FRACTION)) \
  / FX_UNITY_FLOAT)
#else /* Generic version. */
#define GET_FX_FLOAT(x) ((x).as_int / FX_UNITY_FLOAT)
#endif

/* INT_TO_FX(inta, x) - Convert an integer to an FX. */
#define INT_TO_FX(inta, x) { \
  (x).portions.integer = (inta); (x).portions.fraction = 0; }

/* INT_FRACTION_TO_FX(inta, fracb, x) - Assign an integer and fraction to FX. */
#define INT_FRACTION_TO_FX(inta, fracb, x) { \
  (x).portions.integer = (inta); (x).portions.fraction = (fracb); }

/* ZERO_FX(x) - Set an FX to zero. */
#define ZERO_FX(x) { (x).portions.integer = 0; (x).portions.fraction = 0;}

/* NEGATE_FX(x) - Done by subtracting from 0 and overwriting the value. */
#ifdef NABU_H
extern void NEGATE_FX_ASM(pfx x);
#define NEGATE_FX(x) { NEGATE_FX_ASM(&(x)); }
#else /* Generic version. */
#define NEGATE_FX(x) { (x).as_int = -(x).as_int; }
#endif

/* COPY_NEGATE_FX(x, y) - Negate and copy in one step.  Y is set to -X. */
#ifdef NABU_H
extern void COPY_NEGATE_FX_ASM(pfx x, pfx y);
#define COPY_NEGATE_FX(x, y) { COPY_NEGATE_FX_ASM(&(x), &(y)); }
#else /* Generic version. */
#define COPY_NEGATE_FX(x, y) { (y).as_int = -(x).as_int; }
#endif

/* ADD_FX(x, y, z) - Add fx values x and y and put the result in fx value z
   (which can safely overwrite x or y if it is the same address as them). */
#ifdef NABU_H
extern void ADD_FX_ASM(pfx x, pfx y, pfx z);
#define ADD_FX(x, y, z) { ADD_FX_ASM(&(x), &(y), &(z)); }
#else /* Generic version. */
#define ADD_FX(x, y, z) { (z).as_int = (x).as_int + (y).as_int; }
#endif

/* SUBTRACT_FX(x, y, z) - Put fx value x - y into z (which can safely overwrite
   x or y even if they have the same address as z). */
#ifdef NABU_H
extern void SUBTRACT_FX_ASM(pfx x, pfx y, pfx z);
#define SUBTRACT_FX(x, y, z) { SUBTRACT_FX_ASM(&(x), &(y), &(z)); }
#else /* Generic version. */
#define SUBTRACT_FX(x, y, z) { (z).as_int = (x).as_int - (y).as_int; }
#endif

/* IS_NEGATIVE_FX(x) returns TRUE if the number is negative. */
#if __BYTE_ORDER == __LITTLE_ENDIAN /* Look at high byte's high bit. */
  #define IS_NEGATIVE_FX(x) (((x).as_bytes[FX_BYTES_WHOLE-1]) & 0x80)
#else /* Big endian, look at lower address byte for the sign. */
  #define IS_NEGATIVE_FX(x) (((x).as_bytes[0]) & 0x80)
#endif

/* ABS_FX(x) - Put fx absolute value of x into x. */
#define ABS_FX(x) { if (IS_NEGATIVE_FX(x)) NEGATE_FX(x); }

/* COPY_ABS_FX(x, y) - Copies the absolute value of x into y. */
#define COPY_ABS_FX(x, y) { if (IS_NEGATIVE_FX(x)) \
  COPY_NEGATE_FX(x, y) else COPY_FX(x, y); }

/* COMPARE_FX(x, y) - Compare two values X & Y, return a small integer (so it
   can be returned in a register rather than on the stack) which is -1 if X < Y,
   zero if X = Y, +1 if X > Y. */
#ifdef NABU_H
extern int8_t COMPARE_FX_ASM(pfx x, pfx y);
#define COMPARE_FX(x, y) ( COMPARE_FX_ASM(&(x), &(y)) )
#else /* Generic version. */
#define COMPARE_FX(x, y) ( ((x).as_int < (y).as_int) ? (int8_t) -1 : \
  ((x).as_int == (y).as_int) ? (int8_t) 0 : (int8_t) 1 )
#endif

/* TEST_FX(x) - Compare value X against zero and return a small integer which
   is -1 if X < 0, zero if X == 0, +1 if X > 0. */
#ifdef NABU_H
extern int8_t TEST_FX_ASM(pfx x);
#define TEST_FX(x)  ( TEST_FX_ASM(&(x)) )
#else /* Generic version. */
#define TEST_FX(x)  ( ((x).as_int < 0) ? (int8_t) -1 : \
  ((x).as_int == 0) ? (int8_t) 0 : (int8_t) 1 )
#endif

/* DIV2_FX(x) - Divide the FX by two.  Same as shifting the given value
   arithmetic right (sign bit extended, so works with negative numbers too) by
   one bit.  1 bit is extra efficient on the Z80 in that we can shift directly
   in memory. */
#ifdef NABU_H
extern void DIV2_FX_ASM(pfx x);
#define DIV2_FX(x) { DIV2_FX_ASM(&(x)); }
#else /* Generic version. */
#define DIV2_FX(x) { (x).as_int >>= 1; }
#endif

/* DIV2Nth_FX(x, n) - Divide the FX by two to the Nth.  Same as shifting the
   given value arithmetic right by N bits (sign bit extended, so works with
   negative numbers too) */
#ifdef NABU_H
extern void DIV2Nth_FX_ASM(pfx x, uint8_t n);
#define DIV2Nth_FX(x, n) { DIV2Nth_FX_ASM(&(x), (n)); }
#else /* Generic version. */
#define DIV2Nth_FX(x, n) { (x).as_int >>= n; }
#endif

/* MUL4INT_FX(x) - Multiply by 4 and return the integer portion. */
#ifdef NABU_H
#define MUL4INT_FX(x) (((x).portions.fraction >> (FX_BITS_FRACTION - 2)) + ((x).portions.integer << 2))
#else /* Generic version. */
#define MUL4INT_FX(x) ((x).as_int >> (FX_BITS_FRACTION - 2))
#endif


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

