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
  ld    hl,4
  add   hl,sp     ; Get pointer to x.
  xor   a,a       ; Zeroes register A, also annoyingly clears carry flag.
  ld    b,a       ; Save the zero for later.
  sub   a,(hl)      ; Less bytes using "sub" than starting with 2 byte "neg".
  ld    (hl),a    ; Overwrite one byte with the result.
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
  x; /* Avoid warning about unused argument, doesn't add any opcodes. */
  y;
  __asm
  ld    hl,2      ; Hop over return address.
  add   hl,sp     ; Get pointer to x in de.
  ld    e,(hl)
  inc   hl
  ld    d,(hl)    ; de now has x, pointer to fx data.
  inc   hl
  ld    c,(hl)
  inc   hl
  ld    b,(hl)    ; bc now has y, pointer to fx data.
  ld    a,(de)    ; Do the comparison, x - y in effect.
  sub   a,(bc)
  inc   de
  inc   bc
  ld    a,(de)
  sbc   a,(bc)
  inc   de
  inc   bc
  ld    a,(de)
  sbc   a,(bc)
  inc   de
  inc   bc
  ld    a,(de)
  sbc   a,(bc)
  ld    l,a     ; 8 bit results returned in l register.
  __endasm;
#else /* Generic C implementation. */
  if (x->as_int32 < y->as_int32)
    return -1;
  if (x->as_int32 > y->as_int32)
    return 1;
  return 0;
#endif
}

