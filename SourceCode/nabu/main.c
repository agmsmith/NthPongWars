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
#pragma output noprotectmsdos /* No need for MS-DOS warning code. */
#pragma printf = "%f %d %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output noredir /* No command line file redirection. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */

#include <stdio.h> /* For printf and sprintf and stdout file handles. */
#include <math.h> /* For floating point math routines. */
#include "../common/fixed_point.h"
#include <arch/z80.h> // for z80_delay_ms()

/* Use DJ Sure's Nabu code library, for VDP video hardware and the Nabu Network
   simulated data network.  Some defines specify options to build the library,
   see NABU-LIB.h for docs.  You may need to adjust the paths to fit where you
   downloaded it from https://github.com/DJSures/NABU-LIB.git */
#define BIN_TYPE BIN_CPM
#define DISABLE_KEYBOARD_INT /* Use stdin for reading keyboard. */
#define FONT_AMIGA
#include "../../../NABU-LIB/NABULIB/NABU-LIB.h" /* Also includes NABU-LIB.c */
#include "../../../NABU-LIB/NABULIB/RetroNET-FileStore.h"
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font data. */

int main(void)
{
  fx FX_ONE_HALF;
  fx fx_test;
  fx fx_quarter;

  initNABULib();
  vdp_initTextMode(15 /* white */, 0 /* blackish */, true /* autoscroll */);
  vdp_loadASCIIFont(ASCII);
  vdp_setCursor2(0, 0);
  vdp_print("Hello World!");
  vdp_newLine();
  z80_delay_ms(1000);
return 0;

/* Rather than having to compute it each time it is used, save a constant. */
  FLOAT_TO_FX(0.5, FX_ONE_HALF);

  FLOAT_TO_FX(123.456, fx_test);
  printf ("123.456 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  INT_TO_FX(-41, fx_test);
  printf ("-41 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  DIV4_FX(fx_test, fx_quarter);
  printf ("-41/4 is %ld or %lX.\n", fx_quarter.as_int, fx_quarter.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_quarter));
  printf ("Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_quarter),
    GET_FX_FRACTION(fx_quarter) / 65536.0);

  INT_TO_FX(0, fx_test);
  SUBTRACT_FX(fx_test, fx_quarter, fx_test);
  printf ("-(-41/4) is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
  printf ("Int part is %d.\n", GET_FX_INTEGER(fx_test));
  printf ("Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);

  return 0;
}
