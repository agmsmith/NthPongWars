/* AGMS20240304 Hello World basic test of C compilers and runtime.
 * AGMS20240416 Add some code to see how the compiler handles 16 bit fixed
 * point fractions.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image) with:
 * zcc +cpm -subtype=naburn --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "TestNumb"
*/

#pragma output noprotectmsdos /* No need for MS-DOS warning code. */
/* Don't #pragma printf %f /* Want to print floating point numbers. */

#include <stdio.h>


void FixedPointTest(void)
{
  signed short NumA = 42.1 * 64;
  signed short NumB = 17.5 * 64;
  signed short NumC = 0;
  signed short NumD = 0;
  signed short NumE = 0;

  printf ("And now a test of fixed point math speed by AGMS20240416.\n");
  NumC = NumA + NumB;
  printf ("Adding %f plus %f yields %f.\n",
    NumA / 64.0, NumB / 64.0, NumC / 64.0);
  NumD = NumC / 64;
  NumE = NumC % 64;
  printf ("Or converting using integer math, result is %d and %d/64.\n",
    NumD, NumE);
}


int main(int argc, char **argv)
{
  printf("Hello World!\n");
  printf ("Test for basic compatibility by AGMS20240304.\n");
  FixedPointTest();
  return 0;
}
