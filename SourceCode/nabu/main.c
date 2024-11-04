/* Nth Pong Wars - the multiplayer Pong inspired game.
 *
 * AGMS20240721 Start a NABU version of the game, just doing the basic bouncing
 * ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * with --math32 for 32 bit floating point.  Use the command line:
 *
 * zcc +cpm --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c z80_delay_ms.asm z80_delay_tstate.asm -o "NTHPONG.COM"
*/

/* Various options to tell the Z88DK compiler system what to include. */
#pragma output noprotectmsdos /* No need for MS-DOS test and warning. */
/* #pragma output noredir /* No command line file redirection. */
/* #pragma output nostreams /* Remove disk IO, can still use stdout and stdin. */
/* #pragma output nofileio /* Remove stdout, stdin also.  See "-lndos" too. */
#pragma printf = "%f %d %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */
#pragma define CRT_STACK_SIZE=1024 /* Extra memory gets put into the heap. */

/* Various standard libraries included here if we need them. */
#if 0 /* These includes are included by NABU-LIB anyway. */
  #include <stdbool.h>
  #include <stdlib.h>
  #include <stdio.h> /* For printf and sprintf and stdout file handles. */
#endif
#include <math.h> /* For 32 bit floating point math routines. */
#include <string.h> /* For strlen. */
#include <malloc.h> /* For malloc and free, and initialising a heap. */

/* Use DJ Sure's Nabu code library, for VDP video hardware and the Nabu Network
   simulated data network.  Now that it compiles with the latest Z88DK compiler
   (lots of warnings about "function declarator with no prototype"),
   and it has a richer library for our purposes than the Z88DK NABU support.
   Here are some defines to select options in the library, see NABU-LIB.h
   for docs.  You may need to adjust the paths to fit where you downloaded it
   from https://github.com/DJSures/NABU-LIB.git */
#define BIN_TYPE BIN_CPM /* We're compiling for CP/M OS, not stand alone. */
/* #define DISABLE_KEYBOARD_INT /* Disable it to use only the CP/M keyboard. */
/* #define DISABLE_HCCA_RX_INT /* Disable if not using networking. */
/* #define DISABLE_VDP /* Disable if not using the Video Display Processor. */
/* #define DEBUG_VDP_INT /* Flash the Alert LED if VDP updates are too slow. */
/* #define DISABLE_CURSOR /* Don't flash during NABU-LIB keyboard input. */
#include "../../../NABU-LIB/NABULIB/NABU-LIB.h" /* Also includes NABU-LIB.c */
#include "../../../NABU-LIB/NABULIB/RetroNET-FileStore.h"
#define FONT_LM80C
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font as a global array. */

/* Temporary buffer used for sprinting into and for intermediate storage during
   screen loading, etc.  Avoids using stack space. */
#define TEMPBUFFER_LEN 512
static char TempBuffer[TEMPBUFFER_LEN];

/* Our own game include files.  Comes after NABU-LIB.h has been included and
   TempBuffer defined. */

#include "../common/fixed_point.h" /* Our own fixed point math. */
#include "LoadScreenICVGM.c"
#include "LoadScreenPC2.c"
#include "z80_delay_ms.h" /* Our hacked up version of time delay for NABU. */

int main(void)
{
  fx FX_ONE_HALF;
  fx fx_test;
  fx fx_quarter;

  /* Note that printf goes through CP/M and scrambles the video memory (it's
     arranged differently), so don't use printf during graphics mode.  However
     the NABU CP/M has a screen text buffer and that will be restored to the
     screen when the program exits, so you can see printf output after exit. */

  printf ("Starting the Nth Pong Wars experiment,\n");
  printf ("by Alexander G. M. Smith, started 2024.\n");
  printf ("Compiled on " __DATE__ " at " __TIME__ ".\n");
  #if __SDCC
    printf ("Using the SDCC compiler.\n");
  #elif __GNUC__
    printf ("Using the GNU gcc compiler version " __VERSION__ ".\n");
  #endif
  z80_delay_ms(1000);

  initNABULib();

  vdp_init(VDP_MODE_G2, VDP_WHITE /* fgColor not applicable */,
    VDP_BLACK /* bgColor */, true /* bigSprites 16x16 pixels but fewer names */,
    false /* magnify */, true /* autoScroll */, true /* splitThirds */);
  /* Sets up the TMS9918A Video Display Processor with this memory map:
    _vdpPatternGeneratorTableAddr = 0x00; 6K or 0x1800 long, ends 0x1800.
    _vdpPatternNameTableAddr = 0x1800; 768 or 0x300 bytes long, ends 0x1B00.
    _vdpSpriteAttributeTableAddr = 0x1b00; 128 or 0x80 bytes long, ends 0x1b80.
    gap 0x1b80, length 1152 or 0x480, ends 0x2000.
    _vdpColorTableAddr = 0x2000; 6K or 0x1800 long, ends 0x3800.
    _vdpSpriteGeneratorTableAddr = 0x3800; 2048 or 0x800 bytes long, end 0x4000.
  */

  if (!LoadScreenPC2("NTHPONG\\COTTAGE.PC2"))
  {
    printf("Failed to load cottage.\n");
    return 10;
  }
  z80_delay_ms(2000);

  if (!LoadScreenICVGM("NTHPONG\\NTHPONG1.ICV"))
  {
    printf("Failed to load NthPong1.\n");
    return 10;
  }
  z80_delay_ms(5000);

  vdp_clearVRAM();

  /* Write a test pattern to the name table, so you can see the font loading.
     Use of Do-While generates better assembler code (half the size), combines
     a test if zero with a decrement instruction, so fewer instructions.
     Comparing against zero is also faster.  The resulting assembler code:
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
  { /* Put in a block so i and j are temporary variables. */
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
     the 3 split screen slices of graphics mode 2 pattern table.  Plain
     vdp_loadASCIIFont() doesn't do the triplication, and skips first 32. */
  vdp_loadPatternTable(ASCII - 256, 1024);
  vdp_loadPatternToId(0, ASCII); /* Blank at name #0, for screen clear. */

  /* Solid blue block at pattern #1, used for border around the screen. */
  vdp_loadPatternToId(1 /* ID */, ASCII);
  for (uint8_t i = 0; i < 8; i++)
    TempBuffer[i] = (char) (VDP_WHITE << 4 | VDP_DARK_BLUE);
  vdp_loadColorToId(1, TempBuffer);

  /* Try printing some text.  Don't go off screen, else it will write past the
     end of memory.  Test going off one line, autoscroll working?  Yup. */
  vdp_setCursor2(0, 0);
  for (uint8_t i = 0; i < _vdpCursorMaxYFull; i++) {
    sprintf(TempBuffer, "Hello, world #%d!\n", (int) i);
    vdp_print(TempBuffer);
    vdp_newLine();
    z80_delay_ms(100);
  }
  vdp_clearScreen(); /* Just clears the text area = name table to zero. */

  /* Rather than having to compute it each time it is used, save a constant. */
  FLOAT_TO_FX(0.5, FX_ONE_HALF);

  /* Do some sanity tests. */

  vdp_setCursor2(0, 0);
  FLOAT_TO_FX(123.456, fx_test);
  sprintf (TempBuffer, "123.456 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempBuffer); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  INT_TO_FX(-41, fx_test);
  sprintf (TempBuffer, "-41 is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempBuffer); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  DIV4_FX(fx_test, fx_quarter);
  sprintf (TempBuffer, "-41/4 is %ld or %lX.\n", fx_quarter.as_int, fx_quarter.as_int);
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Int part is %d.\n", GET_FX_INTEGER(fx_quarter));
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_quarter),
    GET_FX_FRACTION(fx_quarter) / 65536.0);
    vdp_print(TempBuffer); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  INT_TO_FX(0, fx_test);
  SUBTRACT_FX(fx_test, fx_quarter, fx_test);
  sprintf (TempBuffer, "-(-41/4) is %ld or %lX.\n", fx_test.as_int, fx_test.as_int);
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Int part is %d.\n", GET_FX_INTEGER(fx_test));
    vdp_print(TempBuffer); vdp_newLine();
  sprintf (TempBuffer, "Fract part is %d / 65536 or %f.\n",
    GET_FX_FRACTION(fx_test),
    GET_FX_FRACTION(fx_test) / 65536.0);
    vdp_print(TempBuffer); vdp_newLine(); vdp_newLine();
  z80_delay_ms(1000);

  vdp_print("Hit any key to continue."); getChar();

  /* Set up a simple bouncing ball, using character graphics. */

  fx ball_velocity_x; INT_TO_FX(0, ball_velocity_x);
  fx ball_velocity_y; INT_TO_FX(0, ball_velocity_y);
  fx ball_position_x; INT_TO_FX(1, ball_position_x);
  fx ball_position_y; INT_TO_FX(1, ball_position_y);
  fx wall_left_x, wall_right_x, wall_top_y, wall_bottom_y;

  /* Ball position X can be from 1.0 to almost width-1, for an 80 column screen
     that would be 1.0 to 78.99.  It's drawn at the integer portion character
     cell, so 1 to 78.  Wall is cells at X = 0 and 79.  Y is similar. */
  INT_TO_FX(1, wall_left_x);
  INT_FRACTION_TO_FX(_vdpCursorMaxXFull - 2, MAX_FX_FRACTION, wall_right_x);
  INT_TO_FX(1, wall_top_y);
  INT_FRACTION_TO_FX(_vdpCursorMaxYFull - 2, MAX_FX_FRACTION, wall_bottom_y);

  /* Draw the walls around the screen.  Blue solid block #1 for frame. */

  for (uint8_t row = 0; row < _vdpCursorMaxYFull; row++)
  {
    vdp_writeCharAtLocation(0, row, 1);
    vdp_writeCharAtLocation(_vdpCursorMaxXFull - 1, row, 1);
  }

  for (uint8_t col = 0; col < _vdpCursorMaxXFull; col++)
  {
    vdp_writeCharAtLocation(col, 0, 1);
    vdp_writeCharAtLocation(col, _vdpCursorMaxYFull - 1, 1);
  }

  vdp_setCursor2(1, 0);
  vdp_print("Frame:");

#if 0
  while (!g_stop_running)
  {
    int letter;

    letter = getch(); /* Read a character and do a time delay if none. */
    if (letter != ERR) /* Returns ERR if no character ready. */
    {
      /* Decode user input.  Q to quit, WASD or cursor keys to move,
         space to fire. */
      char letter_upcase = toupper(letter);

      if (letter_upcase == 'Q')
        g_stop_running = true;
      else if (letter == KEY_DOWN || letter_upcase == 'S')
        ADD_FX(ball_velocity_y, FX_ONE_HALF, ball_velocity_y);
      else if (letter == KEY_UP || letter_upcase == 'W')
        SUBTRACT_FX(ball_velocity_y, FX_ONE_HALF, ball_velocity_y);
      else if (letter == KEY_LEFT || letter_upcase == 'A')
        SUBTRACT_FX(ball_velocity_x, FX_ONE_HALF, ball_velocity_x);
      else if (letter == KEY_RIGHT || letter_upcase == 'D')
        ADD_FX(ball_velocity_x, FX_ONE_HALF, ball_velocity_x);
    }

    /* Erase old ball position with a space character. */

    attr_set(A_NORMAL, 0 /* colour pair # */, NULL /* options */);
    col = GET_FX_INTEGER(ball_position_x);
    row = GET_FX_INTEGER(ball_position_y);
    mvaddch(row, col, ' ');

    /* Update ball position and velocities - bounce off walls. */

    ADD_FX(ball_position_x, ball_velocity_x, ball_position_x);
    if (COMPARE_FX(ball_position_x, wall_left_x) < 0)
    {
      ball_position_x = wall_left_x; /* Don't go through wall. */
      /* velocity = -(vel - vel/4), reverse direction and reduce speed. */
      DIV4_FX(ball_velocity_x, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_x, ball_velocity_x);
    }
    else if (COMPARE_FX(ball_position_x, wall_right_x) > 0)
    {
      ball_position_x = wall_right_x;
      DIV4_FX(ball_velocity_x, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_x, ball_velocity_x);
    }

    ADD_FX(ball_position_y, ball_velocity_y, ball_position_y);
    if (COMPARE_FX(ball_position_y, wall_top_y) < 0)
    {
      ball_position_y = wall_top_y;
      DIV4_FX(ball_velocity_y, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_y, ball_velocity_y);
    }
    else if (COMPARE_FX(ball_position_y, wall_bottom_y) > 0)
    {
      ball_position_y = wall_bottom_y;
      DIV4_FX(ball_velocity_y, fx_quarter);
      SUBTRACT_FX(fx_quarter, ball_velocity_y, ball_velocity_y);
    }

    /* Draw the ball. */

    attr_set(A_NORMAL, 1 /* colour pair # */, NULL /* options */);
    col = GET_FX_INTEGER(ball_position_x);
    row = GET_FX_INTEGER(ball_position_y);
    mvaddch(row, col, 'O');

    /* Print the score. */

    snprintf(score_string, sizeof(score_string) - 1,
      "%d, (%9.5f,%9.5f), (%9.5f,%9.5f)", frame_count,
      GET_FX_FLOAT(ball_position_x),
      GET_FX_FLOAT(ball_position_y),
      GET_FX_FLOAT(ball_velocity_x),
      GET_FX_FLOAT(ball_velocity_y));
    attr_set(A_REVERSE, 0 /* colour pair # */, NULL /* options */);
    mvaddstr(0 /* row */, 10 /* col */, score_string);

    refresh;
    frame_count++;
  }
#endif

  vdp_newLine();
  vdp_print("Hit any key to end.");
  printf ("You ended with %c.\n", getChar());
  return 0;
}
