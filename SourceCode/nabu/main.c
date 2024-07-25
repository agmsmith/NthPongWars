/* Nth Pong Wars - the multiplayer Pong inspired game.
 *
 * AGMS20240721 Start a NABU version of the game, just doing the basic bouncing
 * ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * with --math32 for 32 bit floating point.  Use the command line:
 *
 * zcc +cpm --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "WALLBNCE.COM"
*/

/* Various options to tell the Z88DK compiler system what to include. */
#pragma output noprotectmsdos /* No need for don't run on MS-DOS warning. */
/* #pragma output noredir /* No command line file redirection. */
/* #pragma output nostreams /* Remove disk IO, can still use stdout and stdin. */
/* #pragma output nofileio /* Remove stdout, stdin also.  See "-lndos" too. */
#pragma printf = "%f %d %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */

#if 0 /* These includes are included by NABU-LIB anyway. */
  #include <stdbool.h>
  #include <stdlib.h>
  #include <stdio.h> /* For printf and sprintf and stdout file handles. */
#endif
#include <math.h> /* For floating point math routines. */
#include "../common/fixed_point.h" /* Our own fixed point 16.16 math. */
#include <arch/z80.h> /* for z80_delay_ms(), runs slow 3.579545 vs 4.0 Mhz. */

/* Use DJ Sure's Nabu code library, for VDP video hardware and the Nabu Network
   simulated data network.  Now that it compiles with the latest Z88DK compiler
   (lots of warnings about "function declarator with no prototype"),
   and it has a richer library for our purposes than the Z88DK NABU support.
   Here are some defines specify options to build the library, see NABU-LIB.h
   for docs.  You may need to adjust the paths to fit where you downloaded it
   from https://github.com/DJSures/NABU-LIB.git */
#define BIN_TYPE BIN_CPM
/* #define DISABLE_KEYBOARD_INT /* Disable it to use only the CP/M keyboard. */
/* #define DISABLE_HCCA_RX_INT /* Disable if not using networking. */
/* #define DISABLE_VDP /* Disable if not using the Video Display Processor. */
/* #define DEBUG_VDP_INT /* Flash the Alert LED if VDP updates are too slow. */
/* #define DISABLE_CURSOR /* Don't flash during NABU-LIB keyboard input. */
#include "../../../NABU-LIB/NABULIB/NABU-LIB.h" /* Also includes NABU-LIB.c */
#include "../../../NABU-LIB/NABULIB/RetroNET-FileStore.h"
#define FONT_LM80C
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font data array in memory. */

static char TempString[81]; /* For sprintfing into.  Avoids using stack. */

int main(void)
{
  fx FX_ONE_HALF;
  fx fx_test;
  fx fx_quarter;

  initNABULib();

  vdp_clearVRAM();
  vdp_init(VDP_MODE_G2, VDP_WHITE /* fgColor not applicable */,
    VDP_BLACK /* bgColor */, true /* bigSprites 16x16 pixels but fewer names */,
    false /* magnify */, true /* autoScroll */, true /* splitThirds */);
  /* Sets up the TMS9918A Video Display Processor with this memory map:
    _vdpPatternGeneratorTableAddr = 0x00; 6K or 0x1800 long, ends 0x1800.
    _vdpPatternNameTableAddr = 0x1800; 768 or 0x300 bytes long, ends 0x1B00.
    _vdpSpriteAttributeTableAddr = 0x1b00; 128 or 0x80 bytes long, ends 0x1b80.
    gap 0x1b80, length 1152 or 0x480, ends 0x2000.
    _vdpColorTableAddr = 0x2000; 6K or 0x1800 long, ends 0x3800.
    _vdpSpriteGeneratorTableAddr = 0x3800; 1024 or 0x400 bytes long, end 0x3C00.
    gap 0x3c00, length 1024 or 0x400, ends 0x4000.
  */

  /* Write a test pattern to the name table, so you can see the font loading.
     Use of Do-While generates better assembler code, combines a test if zero
     with a decrement instruction, so fewer instructions.  Comparing against
     zero is also faster.  The resulting assembler code:
        ld	c,0x03
      l_main_00118:
        xor	a, a
      l_main_00101:
        out	(_IO_VDPDATA), a
        dec	a
        jr	NZ,l_main_00101
        dec	c
        jr	NZ,l_main_00118
     */
  {
    vdp_setWriteAddress(_vdpPatternNameTableAddr);
    for (uint8_t i = 0; i < 3; i++) {
      for (uint16_t j = 0; j < 256; j++) {
        IO_VDPDATA = j;
      }
    }
  }

  {
    vdp_setWriteAddress(_vdpPatternNameTableAddr);
    uint8_t i = 3;
    do {
      uint8_t j = 0;
      do {
        IO_VDPDATA = j;
      } while (--j != 0);
    } while (--i != 0);
  }

  /* Set colours to white and black for every tile / letter. */

  vdp_setWriteAddress(_vdpColorTableAddr);
  for (uint16_t i = 0x1800; i != 0; i--) {
    IO_VDPDATA = (uint8_t) ((VDP_WHITE << 4) | VDP_BLACK);
  }

  /* Note that font doesn't have first 32 control characters, so load them with
     whatever garbage is in memory.  Also this call triplicates it into each of
     the 3 split screen slices of graphics mode 2 pattern table.*/
  vdp_loadPatternTable(ASCII - 256, 1024);

  /* Try printing some text.  Don't go off screen, else it will write past the
     end of memory.  Test going off one line, autoscroll working? */
  vdp_setCursor2(0, 0);
  for (uint8_t i = 0; i <= _vdpCursorMaxYFull; i++) {
    sprintf(TempString, "Hello, world #%d!\n", (int) i);
    vdp_print(TempString);
    vdp_newLine();
    z80_delay_ms(1000);
  }
  vdp_clearScreen(); /* Just clears the text area = name table. */

  /* Rather than having to compute it each time it is used, save a constant. */
  FLOAT_TO_FX(0.5, FX_ONE_HALF);

  /* Do some sanity tests. */

  vdp_setCursor2(0, 0);
  FLOAT_TO_FX(123.456, fx_test);
  sprintf (TempString, "123.456 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempString); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  INT_TO_FX(-41, fx_test);
  sprintf (TempString, "-41 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempString); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  DIV4_FX(fx_test, fx_quarter);
  sprintf (TempString, "-41/4 is %ld or %lX.\n", fx_quarter.as_int, fx_quarter.as_int);
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Int part is %d.\n", GET_FX_INTEGER(fx_quarter));
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_quarter),
    GET_FX_FRACTION(fx_quarter) / 65536.0);
    vdp_print(TempString); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  INT_TO_FX(0, fx_test);
  SUBTRACT_FX(fx_test, fx_quarter, fx_test);
  sprintf (TempString, "-(-41/4) is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempString); vdp_newLine();
  sprintf (TempString, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempString); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  vdp_print("Hit any key to end.");
  printf ("You ended with %c.\n", getChar());
  return 0;
}
