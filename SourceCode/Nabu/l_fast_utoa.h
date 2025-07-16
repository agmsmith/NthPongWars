#ifndef _Z80_FAST_UTOA
#define _Z80_FAST_UTOA 1

extern char * fast_utoa(uint16_t number, char *buffer);
/* Write unsigned decimal integer to ascii buffer.  Returns one past the
   last digit written.  Also writes a NUL at the end of the string. */

#endif /* _Z80_FAST_UTOA */

