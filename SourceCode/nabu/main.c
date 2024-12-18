/* Nth Pong Wars - the multiplayer Pong inspired game.
 *
 * AGMS20240721 Start a NABU version of the game, just doing the basic bouncing
 * ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * with --math32 for 32 bit floating point.  Use the command line:
 *
 * zcc +cpm --list -gen-map-file -gen-symbol-file -create-app -compiler=sdcc --opt-code-speed --math32 main.c z80_delay_ms.asm z80_delay_tstate.asm -o "NTHPONG.COM"
 *
 * To prepare to run, create the data files in the server store directory,
 * usually somewhere like Documents/NABU Internet Adapter/Store/NTHPONG/
 * The Art/*.PC2 files are copied as is, the *.DAT text files edited by ICVGM
 * are copied and renamed *.ICV.
*/

/* Various options to tell the Z88DK compiler system what to include. */

#pragma output noprotectmsdos /* No need for MS-DOS test and warning. */
/* #pragma output noredir /* No command line file redirection. */
/* #pragma output nostreams /* Remove disk IO, can still use stdout and stdin. */
/* #pragma output nofileio /* Remove stdout, stdin also.  See "-lndos" too. */
#pragma printf = "%f %d %u %ld %c %s %X %lX" /* Need these printf formats. */
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */

#pragma define CRT_STACK_SIZE=1024
/* Reserve space for local variables on the stack, the remainder goes in the
   heap for malloc()/free() to distribute.  The Z88DK C runtime uses a default
   of 512 bytes of stack.  NABU Cloud CP/M has 200 bytes at the very end of
   memory for its stack (just after the interrupt table). */

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

#if 0 /* We have our own VDP font now, don't need ASCII array in memory. */
#define FONT_LM80C
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font as a global array. */
#endif

/* Temporary buffer used for sprinting into and for intermediate storage during
   screen loading, etc.  Avoids using stack space. */
#define TEMPBUFFER_LEN 512
char TempBuffer[TEMPBUFFER_LEN];

/* Our own game include files.  Comes after NABU-LIB.h has been included and
   TempBuffer defined. */

#include "../common/fixed_point.h" /* Our own fixed point math. */
#include "LoadScreenICVGM.c"
#include "LoadScreenPC2.c"
#include "z80_delay_ms.h" /* Our hacked up version of time delay for NABU. */
#include "Art/NthPong1.h" /* Graphics definitions to go with loaded data. */
#include "../common/tiles.c"


/* The usual press any key to continue prompt, in VDP graphics mode.  NULL for
   a default message string. */
static char HitAnyKey(const char *MessageText)
{
  vdp_newLine();
  vdp_print((char *) (MessageText ? MessageText : "Press any key to continue."));
  return getChar();
}

/* Variables in main() that also need to be accessed from assembler have to be
   globalish. */
static uint16_t sStackFramePointer = 0;
static uint16_t sStackPointer = 0;

void main(void)
{
  /* Save some stack space, variables that persist in main() can be static. */
  static char sOriginalLocationZeroMemory[16];
  static char sOriginalStackMemory[16];
  static uint16_t sFrameCounter;

  /* A few local variables to force stack frame creation, sets IX register. */
  uint8_t i;
  unsigned int sTotalMem, sLargestMem;

  /* Detect memory corruption from using a NULL pointer.  Changing CP/M drive
     letter and user may affect this since they're in the CP/M parameter
     area (the first 256 bytes of memory). */
  memcpy(sOriginalLocationZeroMemory, NULL, sizeof(sOriginalLocationZeroMemory));

  /* Grab the stack pointer and stack frame pointer to see if they're sane.
     If the frame pointer is $FF00 or above (the CP/M really small stack),
     while stack is $Cxxx then things may go badly.  Frame should be inside
     stack. */
  __asm
  push af;
  push hl;
  ld hl,0
  add hl, sp; /* No actual move sp to hl instruction, but can add it. */
  ld (_sStackPointer), hl;
  ld (_sStackFramePointer), ix;
  pop hl;
  pop af;
  __endasm;

  /* Detect memory corruption in our stack. */
  memcpy(sOriginalStackMemory, (char *) (sStackFramePointer + 1),
    sizeof(sOriginalStackMemory));

  /* Note that printf goes through CP/M and scrambles the video memory (it's
     arranged differently), so don't use printf during graphics mode.  However
     the NABU CP/M has a screen text buffer and that will be restored to the
     screen when the program exits, so you can see printf output after exit.
     Or if you've redirected output to a remote device (telnet server), you can
     see it there. */

  printf ("Welcome to the Nth Pong Wars NABU game.\n");
  printf ("By Alexander G. M. Smith, contact me at\n");
  printf ("agmsmith@ncf.ca.  Project started\n");
  printf ("February 2024, see the blog at\n");
  printf ("https://web.ncf.ca/au829/WeekendReports/20240207/NthPongWarsBlog.html\n");
  printf ("Compiled on " __DATE__ " at " __TIME__ ".\n");
  mallinfo(&sTotalMem, &sLargestMem);
  printf ("Heap has %u bytes free, largest %u.\n", sTotalMem, sLargestMem);
  printf ("Using the SDCC compiler, unknown version.\n");

  /* Weird bug in Z88DK where printf("$%X $$$$$%X", a, b); loses any
     number of dollar signs before the second %X. */
#if 0
  printf("Stack pointer is $%X, frame $%X.\n",
    (int) sStackPointer, (int) sStackFramePointer);
#else
  printf("Stack pointer is $%X, frame %c%X.\n",
    (int) sStackPointer, '$', (int) sStackFramePointer);
#endif
  printf ("Hit any key... ");
  printf ("  Got %c.\n", getchar()); /* CP/M compatible keyboard input. */

  if (memcmp(sOriginalStackMemory, (char *) (sStackFramePointer + 1),
  sizeof(sOriginalStackMemory)) != 0)
  {
    printf("Corrupted Stack Memory before anything done!\n");
    return;
  }

  if (memcmp(sOriginalLocationZeroMemory, NULL,
  sizeof(sOriginalLocationZeroMemory)) != 0)
  {
    printf("Corrupted zero page Memory before anything done!\n");
    return;
  }

  initNABULib(); /* No longer can use CP/M text input or output. */

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
    printf("Failed to load NTHPONG\\COTTAGE.PC2.\n");
    return;
  }
  z80_delay_ms(100); /* No font loaded, just graphics, so no hit any key. */

  /* Load our game screen, with a font and sprites defined. */

  if (!LoadScreenICVGM("NTHPONG\\NTHPONG1.ICV"))
  {
    printf("Failed to load NTHPONG\\NTHPONG1.ICV.\n");
    return;
  }

  /* Set up the tiles.  Directly map play area to screen for now. */

  g_play_area_height_tiles = 8;
  g_play_area_width_tiles = 8;

  g_screen_height_tiles = 20;
  g_screen_width_tiles = 28;
  g_screen_top_X_tiles = 2;
  g_screen_top_Y_tiles = 2;

  g_play_area_col_for_screen = 0;
  g_play_area_row_for_screen = 0;

  if (!InitTileArray())
  {
    printf("Failed to set up play area tiles.\n");
    return;
  }

  /* Set up a test pattern in the tile array. */

  for (i = 0; i < OWNER_MAX && i < g_play_area_width_tiles; i++)
  {
    tile_pointer pTile;
    pTile = g_tile_array_row_starts[i];
    if (pTile == NULL)
      break;
    pTile->owner = i;
    pTile += i;
    pTile->owner = i;
  }

  sFrameCounter = 0;
  vdp_enableVDPReadyInt();
  while (!isKeyPressed())
  {
    UpdateTileAnimations();
#if 1
    /* Check for updates that took longer than a frame.  The DEBUG_VDP_INT
       doesn't seem to work in MAME, leaving the Alert light on all the time. */
    if (vdpIsReady)
      playNoteDelay(2, 71, 40); /* High, short note. */
#endif
    vdp_waitVDPReadyInt();
    vdpIsReady = false;
    CopyTilesToScreen();
    vdp_setCursor2(27, 0);
    strcpy(TempBuffer, "000"); /* Leading zeroes. */
    utoa(sFrameCounter, TempBuffer + 3, 10 /* base */);
    vdp_print(TempBuffer + (strlen(TempBuffer) - 3) /* Last three chars. */);
    sFrameCounter++;

#if 1
    /* Every once in a while move the screen around the play area. */

    if ((sFrameCounter & 0x3f) == 0)
    {
      if (++g_play_area_col_for_screen >= g_play_area_width_tiles)
      {
        g_play_area_col_for_screen = 0;
        if (++g_play_area_row_for_screen >= g_play_area_height_tiles)
          g_play_area_row_for_screen = 0;
      }
      ActivateTileArrayWindow();
    }
#endif

#if 1
    if (memcmp(sOriginalLocationZeroMemory, NULL,
    sizeof(sOriginalLocationZeroMemory)) != 0)
    {
      vdp_setCursor2(0, 0);
      vdp_print("Corrupted NULL Memory!");
    }
    if (memcmp(sOriginalStackMemory, (char *) (sStackFramePointer + 1),
    sizeof(sOriginalStackMemory)) != 0)
    {
      vdp_setCursor2(0, 1);
      vdp_print("Corrupted Stack Memory!");
    }
#endif
  }
  vdp_disableVDPReadyInt();

  UpdateTileAnimations(); /* So we can see some dirty flags. */
  DumpTilesToTerminal();
  printf ("Frame count: %d\n", sFrameCounter);

  if (memcmp(sOriginalLocationZeroMemory, NULL,
  sizeof(sOriginalLocationZeroMemory)) != 0)
  {
    printf("Corrupted NULL Memory!");
  }

  if (memcmp(sOriginalStackMemory, (char *) (sStackFramePointer + 1),
  sizeof(sOriginalStackMemory)) != 0)
  {
    printf("Corrupted Stack Memory!");
    /* Restart CP/M even when the stack is corrupted. */
    __asm
    rst 0;
    __endasm;
  }

#if 1
  /* If plain return isn't working.  Happens if main doesn't return void, or if
     you print too much CP/M output to the screen when in graphics mode. */
  printf("Done.  Brute force exit to CP/M.\n");
  __asm
  rst 0;
  __endasm;
#endif
  printf("Done.  Returning to CP/M.\n");
}

