/* Test for the FX floating point routines.
 * Copyright © 2026 by Alexander G. M. Smith.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * with --math32 for 32 bit floating point.  Use this command line:
 *
 * zcc +cpm -v --list --c-code-in-asm -z80-verb -gen-map-file -gen-symbol-file -create-app -compiler=sdcc -O2 --opt-code-speed=all --max-allocs-per-node20000 --fverbose-asm --math32 -lndos -DNABU_H=1 fixed_point_test.c -o "FXTEST.COM" ; cp -v *.COM ~/Documents/NABU\ Internet\ Adapter/Store/CPM/D/0/ ; time sync
 *
 * See https://github.com/marinus-lab/z88dk/wiki/WritingOptimalCode for tips on
 * writing code that the compiler likes and optimizer settings.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Various options to tell the Z88DK compiler system what to include. */

#pragma output noprotectmsdos /* No need for MS-DOS test and warning. */
#pragma output noredir /* No command line file redirection. */
#pragma output CLIB_EXIT_STACK_SIZE = 0 /* Not using atexit() functions. */
#pragma printf = "%f %d %X %c %s" /* Printf formats and float for debug. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */

#pragma define CRT_STACK_SIZE = 1024
/* Reserve space for local variables on the stack, the remainder goes in the
   heap for malloc()/free() to distribute. */

/* Various standard libraries included here if we need them. */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h> /* For printf and sprintf and stdout file handles. */
#include <math.h> /* For 32 bit floating point math routines, for debug. */
#include <string.h> /* For strlen. */
#include <ctype.h> /* For toupper. */
#include <malloc.h> /* For malloc and free, and initialising a heap. */

#include "fixed_point.c" /* Our own fixed point math. */


/*******************************************************************************
 * Main program, just runs the tests.
 */
int main(int argc, char *argv[])
{
  /* Avoid warning by "using" unused arguments. */
  argc; argv;

  /* Initialise some fixed point number constants. */
  ZERO_FX(gfx_Constant_Zero);
  INT_TO_FX(1, gfx_Constant_One);
  COPY_NEGATE_FX(gfx_Constant_One, gfx_Constant_MinusOne);
  INT_FRACTION_TO_FX(0 /* int */, MAX_FX_FRACTION / 8 + 1 /* fraction */,
    gfx_Constant_Eighth);
  COPY_NEGATE_FX(gfx_Constant_Eighth, gfx_Constant_MinusEighth);

  /* Do the tests. */
  FX_Tests();
  return 0;
}


fx TestX;
fx TestY;
fx TestZ;
float TestFloatA;
float TestFloatB;
int16_t TestInt16A;

void FX_Tests(void)
{
  fx LocalA;
  fx LocalB;
  fx LocalC;
  printf("\nRunning the FX Fixed Point math code tests.  AGMS20260523\n");
  printf("Compiled on " __DATE__ " at " __TIME__ ".\n");

  COPY_FX(gfx_Constant_One, TestX);
  COPY_FX(gfx_Constant_One, LocalA);
  COPY_FX(LocalA, LocalB);

  FLOAT_TO_FX(-32109.876, TestX);
  FLOAT_TO_FX(12345.6789, LocalA);
  printf("SWAP_FX %d.%d (%f)\n  with %d.%d (%f)\n",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX),
    GET_FX_INTEGER(LocalA), GET_FX_FRACTION(LocalA), GET_FX_FLOAT(LocalA));
  SWAP_FX(TestX, LocalA);
  printf("  yields %d.%d (%f) and\n  %d.%d (%f).\n",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX),
    GET_FX_INTEGER(LocalA), GET_FX_FRACTION(LocalA), GET_FX_FLOAT(LocalA));

  /* Look at assembler code to see how the macro gets compiled for
     local vs global variables. */
  SWAP_FX(LocalA, LocalB);

  TestFloatA += 12345.6789;
  FLOAT_TO_FX(TestFloatA, LocalC);
  printf("FLOAT_TO_FX float %f to FX %d.%d (fraction out of %d+1).\n", TestFloatA,
    GET_FX_INTEGER(LocalC), GET_FX_FRACTION(LocalC), MAX_FX_FRACTION);

  TestFloatB = GET_FX_FLOAT(LocalC);
  printf("GET_FX_FLOAT back to float %f with error %f.\n",
    TestFloatB, TestFloatB - TestFloatA);

  TestFloatA = -24000.1234;
  FLOAT_TO_FX(TestFloatA, LocalC);
  printf("FLOAT_TO_FX float %f to FX %d.%d (fraction out of %d+1).\n", TestFloatA,
    GET_FX_INTEGER(LocalC), GET_FX_FRACTION(LocalC), MAX_FX_FRACTION);

  TestFloatB = GET_FX_FLOAT(LocalC);
  printf("GET_FX_FLOAT back to float %f with error %f.\n",
    TestFloatB, TestFloatB - TestFloatA);

  TestInt16A = GET_FX_INTEGER(LocalC);
  TestInt16A++;
  INT_TO_FX(TestInt16A, TestX);
  printf("INT_TO_FX integer %d to FX %d.%d.\n", TestInt16A,
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX));

  INT_FRACTION_TO_FX(-10000, 100, LocalB);
  printf("INT_FRACTION_TO_FX (-10000, 100) to FX %d.%d (%f).\n",
    GET_FX_INTEGER(LocalB), GET_FX_FRACTION(LocalB), GET_FX_FLOAT(LocalB));

  printf("NEGATE_FX %d.%d (%f) to ",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX));
  NEGATE_FX(TestX);
  printf("%d.%d (%f).\n",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX));

  FLOAT_TO_FX(5432.625, LocalB);
  printf("NEGATE_FX %d.%d (%f) to ",
    GET_FX_INTEGER(LocalB), GET_FX_FRACTION(LocalB), GET_FX_FLOAT(LocalB));
  NEGATE_FX(LocalB);
  printf("%d.%d (%f).\n",
    GET_FX_INTEGER(LocalB), GET_FX_FRACTION(LocalB), GET_FX_FLOAT(LocalB));

  COPY_NEGATE_FX(LocalB, LocalC);
  printf("COPY_NEGATE_FX to %d.%d (%f).\n",
    GET_FX_INTEGER(LocalC), GET_FX_FRACTION(LocalC), GET_FX_FLOAT(LocalC));

  FLOAT_TO_FX(-6543.556, TestX);
  FLOAT_TO_FX(8765.778, LocalA);
  ADD_FX(TestX, LocalA, TestZ);
  printf("ADD_FX %d.%d (%f)\n  plus  %d.%d (%f)\n  has sum %d.%d (%f).\n",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX),
    GET_FX_INTEGER(LocalA), GET_FX_FRACTION(LocalA), GET_FX_FLOAT(LocalA),
    GET_FX_INTEGER(TestZ), GET_FX_FRACTION(TestZ), GET_FX_FLOAT(TestZ));

  COPY_ABS_FX(TestX, LocalC);
  printf("COPY_ABS_FX %d.%d (%f)\n  yields %d.%d (%f).\n",
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX),
    GET_FX_INTEGER(LocalC), GET_FX_FRACTION(LocalC), GET_FX_FLOAT(LocalC));

  COPY_ABS_FX(LocalA, LocalC);
  printf("COPY_ABS_FX %d.%d (%f)\n  yields %d.%d (%f).\n",
    GET_FX_INTEGER(LocalA), GET_FX_FRACTION(LocalA), GET_FX_FLOAT(LocalA),
    GET_FX_INTEGER(LocalC), GET_FX_FRACTION(LocalC), GET_FX_FLOAT(LocalC));

  printf("COMPARE_FX(%f, %f) is %d.\n",
    GET_FX_FLOAT(LocalA), GET_FX_FLOAT(TestX), COMPARE_FX(LocalA, TestX));

  printf("COMPARE_FX(%f, %f) is %d.\n",
    GET_FX_FLOAT(TestX), GET_FX_FLOAT(LocalA), COMPARE_FX(TestX, LocalA));

  printf("COMPARE_FX(%f, %f) is %d.\n",
    GET_FX_FLOAT(TestX), GET_FX_FLOAT(TestX), COMPARE_FX(TestX, TestX));

  printf("TEST_FX(%f) is %d.\n",
    GET_FX_FLOAT(LocalA), TEST_FX(LocalA));

  printf("TEST_FX(%f) is %d.\n",
    GET_FX_FLOAT(TestX), TEST_FX(TestX));

  printf("TEST_FX(%f) is %d.\n",
    GET_FX_FLOAT(gfx_Constant_Zero), TEST_FX(gfx_Constant_Zero));

  TestFloatA = -6543.556;
  FLOAT_TO_FX(TestFloatA, TestX);
  DIV2_FX(TestX);
  printf("DIV2_FX of %f is %d.%d (%f).\n",
    TestFloatA,
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX));

  FLOAT_TO_FX(TestFloatA, TestX);
  DIV2Nth_FX(TestX, 9);
  printf("DIV2Nth_FX of %f by 9 is %d.%d (%f).\n",
    TestFloatA,
    GET_FX_INTEGER(TestX), GET_FX_FRACTION(TestX), GET_FX_FLOAT(TestX));
}

