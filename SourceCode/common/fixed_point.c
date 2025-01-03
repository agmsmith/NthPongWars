/******************************************************************************
 * Nth Pong Wars, fixed_point.c for fixed point math operations.
 */

#include "fixed_point.h"

/* Negate, or subtract from 0, the fx value. */
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
  jp    po,NoOverflow
  xor   a,0x80    /* Signed value overflowed, sign bit is reversed, fix it. */
NoOverflow:
  jp    p,PositiveResult
  ld    l,0xFF    /* Negative result, so X < Y. */
  ret
PositiveResult:
  ld    a,iyh     /* See if the complete result is zero. */
  or    a,iyl
  or    a,d
  or    a,e
  jr    z,ZeroExit
  ld    l,0x01
  ret
ZeroExit:
  __endasm;
  return 0;
#else /* Generic C implementation. */
  if (x->as_int32 < y->as_int32)
    return -1;
  if (x->as_int32 > y->as_int32)
    return 1;
  return 0;
#endif
}

