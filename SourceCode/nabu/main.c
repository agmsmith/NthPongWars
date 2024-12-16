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

#pragma define CRT_STACK_SIZE=2048
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
#define DEBUG_VDP_INT /* Flash the Alert LED if VDP updates are too slow. */
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

static char OriginalLocationZeroMemory[20];

/* The usual press any key to continue prompt, in VDP graphics mode.  NULL for
   a default message string. */
static char HitAnyKey(const char *MessageText)
{
  char keyPressed;
  vdp_newLine();
  vdp_print((char *) (MessageText ? MessageText : "Press any key to continue."));
  keyPressed = getChar();
  return keyPressed;
}


int main(void)
{
  /* Note that printf goes through CP/M and scrambles the video memory (it's
     arranged differently), so don't use printf during graphics mode.  However
     the NABU CP/M has a screen text buffer and that will be restored to the
     screen when the program exits, so you can see printf output after exit. */

  printf ("Welcome to the Nth Pong Wars NABU game.\n");
  printf ("By Alexander G. M. Smith, contact me at\n");
  printf ("agmsmith@ncf.ca.  Project started\n");
  printf ("February 2024, see the blog at\n");
  printf ("https://web.ncf.ca/au829/WeekendReports/20240207/NthPongWarsBlog.html\n");
  printf ("Compiled on " __DATE__ " at " __TIME__ ".\n");
  #if __SDCC
    unsigned int totalMem, largestMem;
    mallinfo(&totalMem, &largestMem);
    printf ("Using the SDCC compiler, unknown version.\n");
    printf ("Heap has %u bytes free.\n", totalMem);
  #elif __GNUC__
    printf ("Using the GNU gcc compiler version " __VERSION__ ".\n");
  #endif
  printf ("Hit any key... ");
  printf ("  Got %c.\n", getchar()); /* CP/M compatible keyboard input. */

  /* Detect memory corruption from using a NULL pointer. */
  memcpy (OriginalLocationZeroMemory, NULL, sizeof(OriginalLocationZeroMemory));

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

#if 0
  if (!LoadScreenPC2("NTHPONG\\COTTAGE.PC2"))
  {
    printf("Failed to load NTHPONG\\COTTAGE.PC2.\n");
    return 10;
  }
  z80_delay_ms(1000); /* No font loaded, just graphics, so no hit any key. */
#endif

  /* Load our game screen, with a font and sprites defined. */

  if (!LoadScreenICVGM("NTHPONG\\NTHPONG1.ICV"))
  {
    printf("Failed to load NTHPONG\\NTHPONG1.ICV.\n");
    return 10;
  }
  HitAnyKey(NULL);

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
    return 10;
  }
  for (uint8_t i = 0; i < OWNER_MAX && i < g_play_area_width_tiles; i++)
  {
    tile_pointer pTile;
    pTile = g_tile_array_row_starts[i];
    if (pTile == NULL)
      break;
    pTile->owner = i;
    pTile += i;
    pTile->owner = i;
  }

  int frameCounter = 0;
  vdp_enableVDPReadyInt();
  while (isKeyPressed())
    ; /* Wait for the key to be released. */
  while (!isKeyPressed())
  {
    UpdateTileAnimations();
    vdp_waitVDPReadyInt();
    CopyTilesToScreen();
    vdp_setCursor2(27, 0);
    strcpy(TempBuffer, "000"); /* Leading zeroes. */
    itoa(frameCounter, TempBuffer + 3, 10 /* base */);
    vdp_print(TempBuffer + strlen(TempBuffer) - 3 /* Last three chars. */);
    frameCounter++;

    /* Every once in a while move the screen around the play area. */

    if ((frameCounter & 0x3f) == 0)
    {
      if (++g_play_area_col_for_screen >= g_play_area_width_tiles)
      {
        g_play_area_col_for_screen = 0;
        if (++g_play_area_row_for_screen >= g_play_area_height_tiles)
          g_play_area_row_for_screen = 0;
      }
      ActivateTileArrayWindow();
    }

    if (memcmp(OriginalLocationZeroMemory, NULL,
    sizeof(OriginalLocationZeroMemory)) != 0)
    {
      vdp_setCursor2(1, 1);
      vdp_print("Corrupted NULL");
    }
  }
  vdp_disableVDPReadyInt();

/*  UpdateTileAnimations(); /* So we can see some dirty flags. */
/*   DumpTilesToTerminal(); */
  printf ("Frame count: %d\n", (int) frameCounter);
  return 0;
}

