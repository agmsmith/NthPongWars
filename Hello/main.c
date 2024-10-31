/* AGMS20240304 Hello World basic test of C compilers and runtime.
 * AGMS20240416 Add some code to see how the compiler handles 16 bit fixed
 * point fractions.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a hard disk image and a
 * usable .COM file, --math32 for floating point) with:
 * zcc +cpm --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c -o "TESTNUMB.COM"
*/

#pragma printf = "%f %d %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */
#pragma output noprotectmsdos /* No need for MS-DOS warning code. */

#include <stdio.h>
/* #include <fcntl.h> /* For lower level file open, read, close and FDs. */
#include <math.h>


static char Buffer[129]; /* CP/M reads in 128 byte chunks. */


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

void ReadTest(void)
{
  int DataFile = -1;
  const char *FileName = "TESTDATA.BIN";
  ssize_t AmountRead;
  ssize_t TotalRead;

  printf ("Test for reading binary files from CP/M into memory by AGMS20241030.\n");

  /* Can't use fopen/fread since it converts line ends to CRLF etc, even
     if you specify the "rb" binary option in fopen, which messes up file
     contents and positions. */
  DataFile = open(FileName, O_RDONLY, _IOREAD | _IOUSE);
  if (DataFile < 0)
  {
    printf ("Failed to open file %s.\n", FileName);
    return;
  }

  printf ("Successfully opened %s, fd %d.\n", FileName, DataFile);
  AmountRead = 12345;
  TotalRead = 0;
  Buffer[sizeof(Buffer) - 1] = 0;
  while (AmountRead > 0)
  {
    AmountRead = read(DataFile, Buffer, 80);
    if (AmountRead > 0 && AmountRead < sizeof(Buffer))
    {
      Buffer[AmountRead] = 0;
      printf("%s", Buffer);
      TotalRead += AmountRead;
    }
    else
    {
      printf("\nRead %d bytes, ending?\n", AmountRead);
    }
    printf(".");
  }
  printf("\nRead total %d bytes.\n", TotalRead);
  close (DataFile);
  printf("File closed.\n");
}


int main(void)
{
  short unsigned int i = 12345;

  printf("Hello World %d!\n", i);

#if 0
  printf ("Test for computation speed by AGMS20240304.\n");

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
#endif

  ReadTest();

  return 0;
}
