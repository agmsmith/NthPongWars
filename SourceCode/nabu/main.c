/* AGMS20240721 Start a NABU version of the game, just doing the basic bouncing ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image and a
 * usable .COM file, --math32 for floating point) with:
 *
 * zcc +cpm -subtype=naburn --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "WALLBNCE.COM"
*/

#pragma output noprotectmsdos /* No need for MS-DOS warning code. */

#pragma printf = "%f %d %ld %c %s %X %lX" /* Choose print format converters. */

#include <stdio.h>
#include <math.h>
#include "../common/fixed_point.h"

int main(void)
{
  printf("Hello World!\n");

  fx FX_ONE_HALF;
  fx fx_test;
  fx fx_quarter;

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
