#ifndef _Z80_DELAY_MS
#define _Z80_DELAY_MS 1

extern void z80_delay_ms(uint16_t ms);
/* Busy wait exactly the number of milliseconds, which includes the
  time needed for an unconditional call and the ret, but not C glue code. */

#endif /* _Z80_DELAY_MS */

