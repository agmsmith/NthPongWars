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


signed char FixedPoint16Test(signed short NumA, signed short NumB)
{
  signed short NumC;

  NumC = NumA + NumB;
  return (signed char) (NumC >> 6);
}


signed short int FixedPoint32Test(signed long NumA, signed long NumB)
{
  signed long NumC;

  NumC = NumA + NumB;
  return (signed short int) (NumC >> 16);
}


int main(void)
{
  int i;
  signed short Sum16 = 0;
  signed long Sum32 = 0;

  printf("Hello World!\n");
  printf ("Test for basic compatibility by AGMS20240304.\n");

  for (i = 0; i < 30000; i++)
  {
    Sum16 += FixedPoint16Test(42.1 * 64, 17.5 * 64);
  }
  printf ("Finished fixed point test, sum is %d.\n", Sum16);


  for (i = 0; i < 30000; i++)
  {
    Sum32 += FixedPoint16Test(42.1 * 65536, 17.5 * 65536);
  }
  printf ("Finished fixed point test, sum is %ld.\n", Sum32);
return 0;
}
