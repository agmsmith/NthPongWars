/* AGMS20240304 Hello World basic test of C compilers and runtime.
 * AGMS20240416 Add some code to see how the compiler handles 16 bit fixed
 * point fractions.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image and a
 * usable .COM file, --math32 for floating point) with:
 * zcc +cpm -subtype=naburn --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "TestNumb"
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
  int i;
  signed long Sum16 = 0;
  signed long Sum32 = 0;
  float SumFloat = 0.0;
  static char FtoABuf[30];

  printf("Hello World!\n");
  printf ("Test for basic compatibility by AGMS20240304.\n");

  for (i = 0; i < 10000; i++)
  {
    Sum16 += FixedPoint16Test(2.1 * 64, 1.7 * 64);
  }
  printf ("Fixed 16 sum is %ld (%ld).\n", Sum16 >> 6, Sum16);

  for (i = 0; i < 10000; i++)
  {
    Sum32 += FixedPoint32Test(2.1 * 1024, 1.7 * 1024);
  }
  printf ("Fixed 32 sum is %ld (%d).\n", Sum32 >> 10, Sum32);

  for (i = 0; i < 10000; i++)
  {
    SumFloat += Float32Test(2.1, 1.7);
  }
  ftoa(SumFloat, 2 /* fraction digits */, FtoABuf);
  printf ("Float 32 sum is %s.\n", FtoABuf);

  return 0;
}
