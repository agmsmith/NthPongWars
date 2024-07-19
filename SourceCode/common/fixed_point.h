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

#include <stdint.h> /* For __int32_t */

/*******************************************************************************
 * Constants, types.
 */

typedef struct fx_bits_struct {
  __int_16_t integer;
  __int_16_t fraction;
} fx_bits;

typedef union fx_union {
  __int_32_t as_int;
  fx_bits portions;
} fx; /* Fixed point fraction currently in a 32 bit integer. */

#define GET_FX_FRACTION(x) (x.portions.fraction)
#define GET_FX_INTEGER(x) (x.portions.integer)
#define INT_TO_FX(int, x) (x.portions.fraction = 0, x.portions.integer = int)

#define _FX_FLOAT ((float) (1L << 16))
#define FLOAT_TO_FX(fp, x) (x.as_int = fp * _FX_FLOAT)
#define FX_TO_FLOAT(x) (x.as_int / _FX_FLOAT)

