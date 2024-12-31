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
  int32_t as_int32;
  /* So you can treat the whole thing as an integer to do additions in one shot,
     but best not to use this in game code in case we switch to 3 byte fx. */
  fx_bits portions;
  uint8_t as_bytes[4];
} fx;

#define MAX_FX_FRACTION 0xFFFF
#define MAX_FX_INT 0x7FFF
#define MAX_FX_POSITIVE(x) {x.as_int32 = 0x7FFFFFFF;}

#define COPY_FX(x, y) {y.as_int32 = x.as_int32; }
#define _FX_UNITY_FLOAT ((float) (((int32_t) 1) << 16))
#define FLOAT_TO_FX(fpa, x) {x.as_int32 = fpa * _FX_UNITY_FLOAT;}
#define GET_FX_FRACTION(x) (x.portions.fraction)
#define GET_FX_INTEGER(x) (x.portions.integer)
#define GET_FX_FLOAT(x) (x.as_int32 / _FX_UNITY_FLOAT)
#define INT_TO_FX(inta, x) {x.portions.integer = inta; x.portions.fraction = 0; }
#define INT_FRACTION_TO_FX(inta, fracb, x) {x.portions.integer = inta; x.portions.fraction = fracb; }
#define ZERO_FX(x) {x.as_int32 = 0; }

/* Add fx values a and b and put the result in fx value c. */
#define ADD_FX(a, b, c) {c.as_int32 = a.as_int32 + b.as_int32; }

/* Put fx value a - b into c. */
#define SUBTRACT_FX(a, b, c) {c.as_int32 = a.as_int32 - b.as_int32; }

/* Put fx absolute value of x into x. */
#if __BYTE_ORDER == __LITTLE_ENDIAN
  #define ABS_FX(x) { if (x.as_bytes[3] & 0x80) x.as_int32 = -x.as_int32; } 
#else /* Big endian. */
  #define ABS_FX(x) { if (x.as_int32 < 0) x.as_int32 = -x.as_int32; } 
#endif

/* Compare two POSITIVE values A and B, return a small integer which
   is -1 if A < B, zero if A = B, +1 if A > B.  Designed to be faster Z80 code,
   rather than the C compiler which generates lots of temporary copies of the fx
   arguments and copies back and forth between them. */
int8_t COMPARE_FX(fx *x, fx *y) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint8_t *pA, *pB;
  uint8_t a, b;
  pA = &x->as_bytes[0];
  pB = &y->as_bytes[0];
  a = *pA;
  b = *pB;
  if (a > b)
    goto Positive;
  if (a < b)
    goto Negative;

  pA++;
  pB++;
  a = *pA;
  b = *pB;
  if (a > b)
    goto Positive;
  if (a < b)
    goto Negative;

  pA++;
  pB++;
  a = *pA;
  b = *pB;
  if (a > b)
    goto Positive;
  if (a < b)
    goto Negative;

  pA++;
  pB++;
  a = *pA;
  b = *pB;
  if (a > b)
    goto Positive;
  if (a < b)
    goto Negative;
  return 0;

Positive:
  return 1;
Negative:
  return -1;

#else /* Big endian */

  if (x->as_int32 < y->as_int23)
    return -1;
  if (x->as_int32 > y->as_int23)
    return 1;
  return 0;
#endif
}

/* Divide a by 2 and put into a. */
#define DIV2_FX(a, b) {b.as_int32 = a.as_int32 / 2; }

/* Divide a by 4 and put into b. */
#define DIV4_FX(a, b) {b.as_int32 = a.as_int32 / 4; }

#endif /* _FIXED_POINT_H */

