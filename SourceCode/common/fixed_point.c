/******************************************************************************
 * Nth Pong Wars, fixed_point.c for fixed point math operations.
 */

#include "fixed_point.h"

/* Constants for global use.  Don't change them!  Game main() sets them up. */
fx gfx_Constant_Zero;
fx gfx_Constant_One;
fx gfx_Constant_MinusOne;
fx gfx_Constant_Eighth;
fx gfx_Constant_MinusEighth;

/* Copy value of X to Y. */
void COPY_FX(pfx x, pfx y)
{
  y->as_int32 = x->as_int32;
}


/* Negate, done by subtracting from 0 and overwriting the value. */
void NEGATE_FX(pfx x)
{
#ifdef NABU_H
  x; /* Avoid warning about unused argument, doesn't add any opcodes. */
  __asm
  ld    hl,2      /* Get pointer to x from the arguments on stack. */
  add   hl,sp
  ld    e,(hl)
  inc   hl
  ld    d,(hl)
  ex    de,hl     /* hl now points to 32 bits of fx data from x. */
  xor   a,a       /* Zeroes register A, also annoyingly clears carry flag. */
  ld    b,a       /* Save zero so we can rezero without clearing carry. */
  sub   a,(hl)    /* Less bytes using "sub" than starting with 2 byte "neg". */
  ld    (hl),a    /* Overwrite one byte with the result. */
  inc   hl
  ld    a,b
  sbc   a,(hl)
  ld    (hl),a
  inc   hl
  ld    a,b
  sbc   a,(hl)
  ld    (hl),a
  inc   hl
  ld    a,b
  sbc   a,(hl)
  ld    (hl),a
  __endasm;
#else /* Generic C implementation. */
  x->as_int32 = -x->as_int32;
#endif
}


/* Negate and copy in one step.  Y is set to -X. */
void COPY_NEGATE_FX(pfx x, pfx y)
{
#ifdef NABU_H
  x; /* Avoid warning about unused argument, doesn't add any opcodes. */
  y;
  __asm
  ld    hl,5      /* Hop over return address and x and part of y. */
  add   hl,sp     /* Get pointer to argument y, will put in bc. */
  ld    b,(hl)
  dec   hl
  ld    c,(hl)    /* bc points to argument y. */
  dec   hl
  ld    a,(hl)
  dec   hl
  ld    l,(hl)
  ld    h,a       /* hl points to argument x. */
  xor   a,a       /* Zeroes register A, also annoyingly clears carry flag. */
  ld    e,a       /* Save zero in e so we can rezero without clearing carry. */
  sub   a,(hl)    /* Less bytes using "sub" than starting with 2 byte "neg". */
  ld    (bc),a    /* Write one byte of y with the result. */
  inc   hl
  inc   bc
  ld    a,e
  sbc   a,(hl)
  ld    (bc),a
  inc   hl
  inc   bc
  ld    a,e
  sbc   a,(hl)
  ld    (bc),a
  inc   hl
  inc   bc
  ld    a,e
  sbc   a,(hl)
  ld    (bc),a
  __endasm;
#else /* Generic C implementation. */
  y->as_int32 = -x->as_int32;
#endif
}


/* Add fx values x and y and put the result in fx value z (which can safely
   overwrite x or y if it is the same address as them). */
void ADD_FX(pfx x, pfx y, pfx z)
{
#ifdef NABU_H
  x; /* Just avoid warning about unused argument, doesn't add any opcodes. */
  y;
  z;
  __asm
  ld    hl,2      /* Hop over return address. */
  add   hl,sp     /* Get pointer to argument x, will put in bc. */
  ld    c,(hl)
  inc   hl
  ld    b,(hl)    /* bc now has pointer to x's fx data, a 32 bit integer. */
  inc   hl
  ld    e,(hl)
  inc   hl
  ld    d,(hl)    /* Temporarily de has pointer to y's fx data. */
  inc   hl
  ld    a,(hl)
  inc   hl
  ld    h,(hl)    /* Yes, you can do that, mangle hl with (hl) data. */
  ld    l,a       /* Temporarily hl has pointer to z's fx data. */
  ex    de,hl     /* de has z, hl has y, needed for add a,(hl) instruction. */
  ld    a,(bc)    /* Do the 32 bit addition, x + y = z in effect. */
  add   a,(hl)
  ld    (de),a    /* Save result in z, can overwrite x or y, but that's OK. */
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  adc   a,(hl)
  ld    (de),a
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  adc   a,(hl)
  ld    (de),a
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  adc   a,(hl)
  ld    (de),a
  __endasm;
#else /* Generic C implementation. */
  z->as_int32 = x->as_int32 + y->as_int32;
#endif
}


/* Put fx value x - y into z (which can safely overwrite x or y even if they
   have the same address as z). */
void SUBTRACT_FX(pfx x, pfx y, pfx z)
{
#ifdef NABU_H
  x; /* Just avoid warning about unused argument, doesn't add any opcodes. */
  y;
  z;
  __asm
  ld    hl,2      /* Hop over return address. */
  add   hl,sp     /* Get pointer to argument x, will put in bc. */
  ld    c,(hl)
  inc   hl
  ld    b,(hl)    /* bc now has pointer to x's fx data, a 32 bit integer. */
  inc   hl
  ld    e,(hl)
  inc   hl
  ld    d,(hl)    /* Temporarily de has pointer to y's fx data. */
  inc   hl
  ld    a,(hl)
  inc   hl
  ld    h,(hl)    /* Yes, you can do that, mangle hl with (hl) data. */
  ld    l,a       /* Temporarily hl has pointer to z's fx data. */
  ex    de,hl     /* de has z, hl has y, needed for sbc a,(hl) instruction. */
  ld    a,(bc)    /* Do the 32 bit subtraction, x - y = z in effect. */
  sub   a,(hl)
  ld    (de),a    /* Save result in z, can overwrite x or y, but that's OK. */
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  sbc   a,(hl)
  ld    (de),a
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  sbc   a,(hl)
  ld    (de),a
  inc   hl
  inc   bc
  inc   de
  ld    a,(bc)
  sbc   a,(hl)
  ld    (de),a
  __endasm;
#else /* Generic C implementation. */
  z->as_int32 = x->as_int32 - y->as_int32;
#endif
}


/* Put fx absolute value of x into x. */
void ABS_FX(pfx x)
{
  if (IS_NEGATIVE_FX(x))
    NEGATE_FX(x);
}


/* Compare two values X & Y, return a small integer (so it can be returned in
   a register rather than on the stack) which is -1 if X < Y, zero if X = Y,
   +1 if X > Y. */
int8_t COMPARE_FX(pfx x, pfx y)
{
#ifdef NABU_H
  x; /* Just avoid warning about unused argument, doesn't add any opcodes. */
  y;
  __asm
  ld    hl,2      /* Hop over return address. */
  add   hl,sp     /* Get pointer to argument x, will put in bc. */
  ld    c,(hl)
  inc   hl
  ld    b,(hl)    /* bc now has pointer to x's fx data, a 32 bit integer. */
  inc   hl
  ld    e,(hl)
  inc   hl
  ld    d,(hl)    /* de now has pointer to y's fx data. */
  ex    de,hl     /* But need y to be in hl for only sbc a,(regpair) opcode. */
  ld    a,(bc)    /* Do the 32 bit comparison, x - y in effect. */
  sub   a,(hl)
  ld    e,a       /* Collect bits to see if whole result is zero. */
  inc   hl
  inc   bc
  ld    a,(bc)
  sbc   a,(hl)
  ld    d,a
  inc   hl
  inc   bc
  ld    a,(bc)
  sbc   a,(hl)
  ld    iyl,a
  inc   hl
  inc   bc
  ld    a,(bc)
  sbc   a,(hl)
  ld    iyh,a
  jp    PO,NoOverflowCompare /* Pity, no jr jump relative for overflow tests. */
  xor   a,0x80    /* Signed value overflowed, sign bit is reversed, fix it. */
NoOverflowCompare:
  jp    P,PositiveCompare
  ld    l,0xFF    /* Negative result, so X < Y. */
  ret
PositiveCompare:
  ld    a,iyh     /* See if the complete result is zero. */
  or    a,iyl
  or    a,d
  or    a,e
  jr    z,ZeroCompare
  ld    l,0x01
  ret
ZeroCompare:
  __endasm;
  return 0;       /* Need at least one return in C, else get warning. */
#else /* Generic C implementation. */
  if (x->as_int32 < y->as_int32)
    return -1;
  else if (x->as_int32 == y->as_int32)
    return 0;
  return 1;
#endif
}


/* Compare value X against zero and return a small integer which is
   -1 if X < 0, zero if X == 0, +1 if X > 0. */
int8_t TEST_FX(pfx x)
{
#ifdef NABU_H
  x; /* Just avoid warning about unused argument, doesn't add any opcodes. */
  __asm
  ld    hl,2      /* Hop over return address. */
  add   hl,sp     /* Get pointer to argument x, will put in bc. */
  ld    a,(hl)
  inc   hl
  ld    h,(hl)
  ld    l,a       /* hl now has pointer to x's fx data, a 32 bit integer. */
  ld    a,(hl)    /* First, least significant, byte of data. */
  inc   hl
  or    a,(hl)
  inc   hl
  or    a,(hl)
  inc   hl
  or    a,(hl)
  jr    z,ZeroTest
  ld    a,(hl)
  or    a,a
  jp    P,PositiveTest
  ld    l,0xFF    /* Negative result, so X < 0. */
  ret
PositiveTest:
  ld    l,0x01
  ret
ZeroTest:
  __endasm;
  return 0;       /* Need at least one return in C, else get warning. */
#else /* Generic C implementation. */
  if (x->as_int32 < 0)
    return -1;
  else if (x->as_int32 == 0)
    return 0;
  return 1;
#endif
}


/* Divide the FX by two.  Same as shifting the given value arithmetic right
   (sign bit extended, so works with negative numbers too) by one bit.  May
   have to do an N bits version later, but 1 bit is extra efficient in that
   we can shift in memory.
*/
void DIV2_FX(pfx x)
{
#ifdef NABU_H
  x; /* Just avoid warning about unused argument, doesn't add any opcodes. */
  __asm
  pop   de        /* Get return address into de. */
  pop   hl        /* Get pointer to argument x, will put in hl. */
  push  hl        /* Put it back so caller can clean up stack as expected. */
  inc   hl        /* Add 3 to hl, to start at most significant byte of x. */
  inc   hl
  inc   hl
  sra   (hl)      /* Shift right, duplicating high sign bit, modifies memory. */
  dec   hl
  rr    (hl)      /* Rotate right, using the carry bit from previous shift. */
  dec   hl
  rr    (hl)
  dec   hl
  rr    (hl)      /* Last rotation and we're done.  Result already in memory. */
  ex    de,hl
  jp    (hl)      /* Return using saved return address. */
  __endasm;
#else
  x->as_int32 >>= 1;
#endif
}

