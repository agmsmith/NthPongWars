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

#include <sys/types.h> /* For __uint16_t, __int32_t, u16_t, i32_t */
#if __GNUC__
  #define i16_t __int16_t
  #define u16_t __uint16_t
  #define i32_t __int32_t
  #define u32_t __uint32_t
#endif

#include <endian.h> /* For __BYTE_ORDER, __LITTLE_ENDIAN */

/*******************************************************************************
 * Constants, types.
 */

/* Our floating point data type "fx", can be interpreted as a 32 bit integer
   to do math (addition, subtraction) or parts can be extracted, or converted
   to or from a 32 bit float (very slowly).

   Later we may change it to 3 bytes by reducing the size of the fraction,
   but then addition and subtraction will need some custom assembler code. */

typedef struct fx_bits_struct {\
  /* Fields are ordered so that they combine 16 bit values into a 32 bit
     integer of the same endianness.  So we can use a single 32 bit add
     to add both fraction and integer in one shot.

     Note that fractional parts are unsigned, and may be weird if the whole
     number is negative (take the absolute value of the whole fx, then look at
     the fractional part). */
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    u16_t fraction;
    i16_t integer;
  #else /* __BIG_ENDIAN we assume. */
    i16_t integer;
    u16_t fraction;
  #endif
} fx_bits;

typedef union fx_union {
  i32_t as_int;
  fx_bits portions;
} fx;

#define MAX_FX_FRACTION 0xFFFF
#define MAX_FX_INT 0x7FFF

#define _FX_FLOAT ((float) (((i32_t) 1) << 16))
#define GET_FX_FRACTION(x) (x.portions.fraction)
#define GET_FX_INTEGER(x) (x.portions.integer)
#define GET_FX_FLOAT(x) (x.as_int / _FX_FLOAT)
#define INT_TO_FX(inta, x) (x.portions.integer = inta, x.portions.fraction = 0)
#define INT_FRACTION_TO_FX(inta, fracb, x) (x.portions.integer = inta, x.portions.fraction = fracb)
#define FLOAT_TO_FX(fpa, x) (x.as_int = fpa * _FX_FLOAT)

/* Add fx values a and b and put the result in fx value c. */
#define ADD_FX(a, b, c) (c.as_int = a.as_int + b.as_int)

/* Put fx value a - b into c. */
#define SUBTRACT_FX(a, b, c) (c.as_int = a.as_int - b.as_int)

/* Compare two values A and B, return an integer of some sort (could be byte
   sized on the Z80, 32 bits on x86 processors) which is negative if A < B,
   zero if A = B, positive if A > B. */
#define COMPARE_FX(a, b) (a.as_int - b.as_int)

/* Divide a by 4 and put into b. */
#define DIV4_FX(a, b) (b.as_int = a.as_int / 4)

