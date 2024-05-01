/* AGMS20240304 Hello World basic test of C compilers and runtime.
 * AGMS20240416 Add some code to see how the compiler handles 16 bit fixed
 * point fractions.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image and a
 * usable .COM file, --math32 for floating point) with:
 * zcc +cpm -subtype=naburn --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "TestNumb.exe"
*/

#pragma output noprotectmsdos /* No need for MS-DOS warning code. */
/* Don't #pragma printf %f /* Want to print floating point numbers. */

#include <stdio.h>
#include <math.h>


signed short FixedPoint16Test(signed short NumA, signed short NumB)
{
  signed short NumC;

  NumC = NumA + NumB;
  return NumC;
}


signed long FixedPoint32Test(signed long NumA, signed long NumB)
{
  signed long NumC;

  NumC = NumA + NumB;
  return NumC;
}


float Float32Test(float NumA, float NumB)
{
  float NumC;

  NumC = NumA + NumB;
  return NumC;
}


int main(void)
{
  short unsigned int i;

  printf("Hello World!\n");
  printf ("Test for basic compatibility by AGMS20240304.\n");

  for (i = 0; i < 50000; i++)
  {
    FixedPoint16Test(2.1 * 64, 1.7 * 64);
  }
  printf ("Fixed 16 done.\n");

  for (i = 0; i < 50000; i++)
  {
    FixedPoint32Test(2.1 * 1024, 1.7 * 1024);
  }
  printf ("Fixed 32 done.\n");

  for (i = 0; i < 50000; i++)
  {
    Float32Test(2.1, 1.7);
  }
  printf ("Float 32 done.\n");

  return 0;
}
