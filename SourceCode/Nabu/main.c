/* Nth Pong Wars - the multiplayer Pong inspired game.
 * Copyright © 2025 by Alexander G. M. Smith.
 *
 * AGMS20240721 Start a NABU version of the game, just doing the basic bouncing
 * ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * Optionally with --math32 for 32 bit floating point, uses 5K extra memory.
 * Use this command line to compile for CP/M, 20000 max allocs (a compiler code
 * optimization time limit) is good enough:
 *
 * zcc +cpm -v --list --c-code-in-asm -z80-verb -gen-map-file -gen-symbol-file -create-app -compiler=sdcc -O2 --opt-code-speed=all --max-allocs-per-node20000 --fverbose-asm --math32 -lndos main.c z80_delay_ms.asm z80_delay_tstate.asm l_fast_utoa.asm CHIPNSFX.asm Art/NthPongWarsMusic.asm Art/NthPongWarsExtractedEffects.asm -o "NTHPONG.COM" ; cp -v *.COM ~/Documents/NABU\ Internet\ Adapter/Store/D/0/ ; time sync
 *
 * Use this one to compile for HomeBrew (raw NABU segment files downloaded
 * into memory), no CP/M so you have 10K more memory (and 4K of ROM in low
 * memory you can replace), but no files or stdio.  Z88DK simulates the NABU
 * CAB file builder (CABUILD) to make the segment file with a 3 byte header.
 * Note that the Z88DK +nabu target defaults to including their VDP library and
 * other things, so we made a "bare" subtype that has almost nothing plus a
 * relocating loader so we can use the RAM obscured by the ROM.  Get that
 * hacked up Z88DK from https://github.com/agmsmith/z88dk/tree/nabu_bare
 *
 * zcc +nabu -subtype=bare -v --list --c-code-in-asm -z80-verb -gen-map-file -gen-symbol-file -create-app -compiler=sdcc -O2 --opt-code-speed=all --max-allocs-per-node20000 --fverbose-asm main.c z80_delay_ms.asm z80_delay_tstate.asm l_fast_utoa.asm CHIPNSFX.asm Art/NthPongWarsMusic.asm Art/NthPongWarsExtractedEffects.asm -o "NthPongWars" ; rm -v NthPongWars ; cp -v NTHPONGWARS.NABU ~/Documents/NABU\ Internet\ Adapter/Local\ Source/NthPongWars.nabu ; time sync
 * See https://github.com/marinus-lab/z88dk/wiki/WritingOptimalCode for tips on
 * writing code that the compiler likes and optimizer settings.
 *
 * To prepare to run, create the data files in the server store directory,
 * usually somewhere like Documents/NABU Internet Adapter/Store/NTHPONG/
 * The Art/*.PC2 files are copied as is, the *.DAT text files edited by ICVGM
 * are copied and renamed *.ICV (TODO: convert them to binary for speed).
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
#ifdef __NABU_BARE__
  #pragma output nostreams /* No disk based file streams? */
  #pragma output nofileio /* Sets CLIB_OPEN_MAX to zero, also use -lndos? */
#else /* We have stdio and printf, and usually CP/M as the OS doing it. */
  #pragma printf = "%d %X %c %s" /* Need these printf formats. */
  /* #pragma printf = "%f %d %X %c %s" /* Printf formats and float for debug. */
#endif
#pragma output nogfxglobals /* No global variables from Z88DK for graphics. */

#pragma define CRT_STACK_SIZE=1024
/* Reserve space for local variables on the stack, the remainder goes in the
   heap for malloc()/free() to distribute.  The Z88DK C runtime uses a default
   of 512 bytes of stack.  NABU Cloud CP/M has 200 bytes at the very end of
   memory for its stack (just after the interrupt table).  We need a bit more
   for temporary buffer space to load pictures, etc.  General game code doesn't
   make any lasting allocations, to avoid fragmentation of memory. */

/* Various standard libraries included here if we need them. */
#if 0 /* These includes are included by NABU-LIB anyway. */
  #include <stdbool.h>
  #include <stdlib.h>
  #include <stdio.h> /* For printf and sprintf and stdout file handles. */
#endif
/* #include <math.h> /* For 32 bit floating point math routines, for debug. */
#include <string.h> /* For strlen. */
#include <malloc.h> /* For malloc and free, and initialising a heap. */

/* Use DJ Sure's Nabu code library, for VDP video hardware and the Nabu Network
   simulated data network.  Now that it compiles with the latest Z88DK compiler
   (lots of warnings about "function declarator with no prototype" which we
   fixed in our branch), and it has a richer library for our purposes than the
   Z88DK NABU support.  Here are some defines to select options in the library,
   see NABU-LIB.h for docs.  You may need to adjust the paths to fit where you
   downloaded it from https://github.com/DJSures/NABU-LIB.git  There's a fixed
   version at https://github.com/agmsmith/NABU-LIB/tree/NthPongCustomisations */
#ifdef __NABU_BARE__ /* Defined for Z88DK builds with +nabu -subtype=bare */
  #define BIN_TYPE BIN_HOMEBREW /* Compile for stand alone no OS mode. */
#else
  #define BIN_TYPE BIN_CPM /* Have printf() and standard output. */
#endif
/* #define DISABLE_KEYBOARD_INT /* Disable it to use only the CP/M keyboard. */
/* #define DISABLE_HCCA_RX_INT /* Disable if not using networking. */
/* #define DISABLE_VDP /* Disable if not using the Video Display Processor. */
/* #define DEBUG_VDP_INT /* Flash the Alert LED if VDP updates are too slow. */
/* #define DISABLE_CURSOR /* Don't flash on VDP during NABU-LIB keyboard input. */
#include "../../../NABU-LIB/NABULIB/NABU-LIB.h" /* Also includes NABU-LIB.c */
#include "../../../NABU-LIB/NABULIB/RetroNET-FileStore.h" /* For TCP Server. */

#if 0 /* We have our own VDP font now, don't need array using up memory. */
#define FONT_LM80C
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font as a global array. */
#endif

/* Temporary global buffer used for sprinting into and for intermediate storage
   during screen loading, etc.  Avoids using stack space. */
#define TEMPBUFFER_LEN 512
char g_TempBuffer[TEMPBUFFER_LEN];

/* Our own game include files, some are source code!  Comes after NABU-LIB.h
   has been included and g_TempBuffer defined. */

#include "../Common/fixed_point.c" /* Our own fixed point math. */
#include "z80_delay_ms.h" /* Our hacked up version of time delay for NABU. */
#include "l_fast_utoa.h" /* Our hacked up version of utoa() to fix bugs. */
#include "CHIPNSFX.h" /* Music player glue functions. */
#include "Art/NthPong1.h" /* Graphics definitions to go with loaded data. */
#include "Art/NthPongWarsMusic.h" /* List of available Music loaded. */
#include "../Common/debug_print.c"
#include "../Common/tiles.c"
#include "../Common/players.c"
#include "../Common/simulate.c"
#include "../Common/scores.c"
#include "../Common/soundscreen.c"

/* Variables in main() that also need to be accessed from assembler have to be
   globalish.  Make them static but global scope. */

static uint16_t s_StackFramePointer = 0;
static uint16_t s_StackPointer = 0;
static const char *k_CorruptedLowMemText =
  "Corrupted location zero (NULL) Memory!";
static const char *k_WelcomeText =
  "Welcome to the Nth Pong Wars NABU game.  "
  "Copyright 2025 by Alexander G. M. Smith, contact agmsmith@ncf.ca.  "
  "Started in February 2024, see the blog at "
  "https://web.ncf.ca/au829/WeekendReports/20240207/NthPongWarsBlog.html  "
  "Released under the GNU General Public License version 3.\n";
static bool s_KeepRunning;


/*******************************************************************************
 * The usual press any key to continue prompt, in VDP graphics mode.  NULL
 * message pointer to show a default message string.  Keeps the music playing
 * while waiting for a key to be pressed.
 */
static char HitAnyKey(const char *MessageText)
{
  vdp_newLine();
  vdp_print((char *) (MessageText ? MessageText : "Press any key to continue."));

  while (!isKeyPressed())
  {
    vdp_waitVDPReadyInt();
    vdp_waitVDPReadyInt();
    CSFX_play(); /* Update music every 1/30 of a second. */
  }

  return getChar();
}


/*******************************************************************************
 * Process Keyboard inputs.  Mostly making fake joystick data from cursor keys,
 * and checking for "q" to quit.
 */
#define DEBUG_KEYBOARD 0
static void ProcessKeyboard(void)
{
  while (isKeyPressed())
  {
    uint8_t letter;
    letter = getChar(); /* Note, may flash cursor, writing ' ' to VDP. */

    if (letter >= 0xE0 && letter < 0xEB) /* Cursor key pressed. */
    {
      if (letter == 0xE7) /* YES key pressed, F1 in MAME. */
        g_KeyboardFakeJoystickStatus |= Joy_Button;
      else if (letter == 0xE3) /* Down cursor key pressed. */
        g_KeyboardFakeJoystickStatus |= Joy_Down;
      else if (letter == 0xE2) /* Up cursor key pressed. */
        g_KeyboardFakeJoystickStatus |= Joy_Up;
      else if (letter == 0xE1) /* Left cursor key pressed. */
        g_KeyboardFakeJoystickStatus |= Joy_Left;
      else if (letter == 0xE0) /* Right cursor key pressed. */
        g_KeyboardFakeJoystickStatus |= Joy_Right;
    }
    else if (letter >= 0xF0 && letter < 0xFB) /* Cursor key released. */
    {
      if (letter == 0xF7) /* YES key released. */
        g_KeyboardFakeJoystickStatus &= ~Joy_Button;
      else if (letter == 0xF3) /* Down cursor key released. */
        g_KeyboardFakeJoystickStatus &= ~Joy_Down;
      else if (letter == 0xF2) /* Up cursor key released. */
        g_KeyboardFakeJoystickStatus &= ~Joy_Up;
      else if (letter == 0xF1) /* Left cursor key released. */
        g_KeyboardFakeJoystickStatus &= ~Joy_Left;
      else if (letter == 0xF0) /* Right cursor key released. */
        g_KeyboardFakeJoystickStatus &= ~Joy_Right;
    }
    else
    {
#if DEBUG_KEYBOARD
  #ifndef __NABU_BARE__
      printf ("Got letter %d %c.\n", (int) letter, letter);
  #endif
#endif
      if (letter == 'q')
      {
        s_KeepRunning = false;
      }
    }
  }
}


/*******************************************************************************
 * Initialise an about this program message in the temp buffer.
 */
static void SetUpAboutThisProgramText(void)
{
  uint16_t totalMem, largestMem;

  strcpy(g_TempBuffer, "Compiled on " __DATE__ " at " __TIME__ ".  ");
  mallinfo(&totalMem, &largestMem);
  strcat (g_TempBuffer, "Heap has ");
  AppendDecimalUInt16(totalMem);
  strcat (g_TempBuffer, " bytes free, largest ");
  AppendDecimalUInt16(largestMem);
  strcat (g_TempBuffer, ".  Stack pointer is ");
  AppendDecimalUInt16(s_StackPointer);
  strcat(g_TempBuffer, ", frame ");
  AppendDecimalUInt16(s_StackFramePointer);
  strcat(g_TempBuffer, ".  Using Z88DK with the SDCC compiler, and "
    "D. J. Sures NABU-LIB.\n");
}


/*******************************************************************************
 * Main program and main game loop.
 */
void main(void)
{
  /* Save some stack space, variables that persist in main() can be static. */
  static char s_OriginalLocationZeroMemory[16];

  /* Initialise some fixed point number constants. */
  ZERO_FX(gfx_Constant_Zero);
  INT_TO_FX(1, gfx_Constant_One);
  COPY_NEGATE_FX(&gfx_Constant_One, &gfx_Constant_MinusOne);
  INT_FRACTION_TO_FX(0 /* int */, 0x2000 /* fraction */, gfx_Constant_Eighth); 
  COPY_NEGATE_FX(&gfx_Constant_Eighth, &gfx_Constant_MinusEighth);

  /* Detect memory corruption from using a NULL pointer.  Changing CP/M drive
     letter and user may affect this since they're in the CP/M parameter
     area (the first 256 bytes of memory). */
  memcpy(s_OriginalLocationZeroMemory, NULL,
    sizeof(s_OriginalLocationZeroMemory));

  /* Grab the stack pointer and stack frame pointer to see if they're sane.
     If the frame pointer is $FF00 or above (the CP/M really small stack),
     while stack is $Cxxx then things may go badly.  Frame should be nearby
     stack.  Nabu Bare mode has stack at $FF00, interrupt table above that.*/
  __asm
  push af;
  push hl;
  ld hl,0
  add hl, sp; /* No actual move sp to hl instruction, but can add it. */
  ld (_s_StackPointer), hl;
  ld (_s_StackFramePointer), ix;
  pop hl;
  pop af;
  __endasm;

  DebugPrintString(k_WelcomeText); /* So CP/M users can see the welcome text. */
  SetUpAboutThisProgramText();
  DebugPrintString(g_TempBuffer);

  initNABULib(); /* No longer can use CP/M text input or output. */

  vdp_init(VDP_MODE_G2,
    VDP_WHITE /* fgColor not applicable */, VDP_BLACK /* bgColor */,
    true /* bigSprites 16x16 pixels but fewer names, each takes 4 */,
    false /* magnify */, true /* splitThirds */);
  /* Sets up the TMS9918A Video Display Processor with this memory map:
    _vdpPatternGeneratorTableAddr = 0x00; 6K or 0x1800 long, ends 0x1800.
    _vdpPatternNameTableAddr = 0x1800; 768 or 0x300 bytes long, ends 0x1B00.
    _vdpSpriteAttributeTableAddr = 0x1b00; 128 or 0x80 bytes long, ends 0x1b80.
    gap 0x1b80, length 1152 or 0x480, ends 0x2000.
    _vdpColorTableAddr = 0x2000; 6K or 0x1800 long, ends 0x3800.
    _vdpSpriteGeneratorTableAddr = 0x3800; 2048 or 0x800 bytes long, end 0x4000.
  */

  /* Note that printf goes through CP/M and scrambles the video memory (it's
     arranged differently), so don't use printf during graphics mode.  However
     the NABU CP/M has a screen text buffer and that will be restored to the
     screen when the program exits, so you can see printf output after exit.
     Or if you've redirected output to a remote device (telnet server), you can
     see it there and it doesn't mess up the screen.  But since Nabu Bare mode
     doesn't have printf, best to not use it. */

  /* Turn off random sprites, often left over from the last run. */

  vdp_setWriteAddress(_vdpSpriteAttributeTableAddr);
  IO_VDPDATA = 0xD0;

  /* Make sure cursor is at top left, so it will wipe out a known spot on
     screen for full screen graphics. */
  vdp_setCursor2(0, 0);

  /* Initialise the CHIPNSFX music player library, before frame interrupt. */
  CSFX_stop();

  /* Start the frame interrupt, to count frames and do sound tick timing. */
  vdp_enableVDPReadyInt();

#if 1
  /* Start some loading screen music. */
  PlayMusic("ONECOLN1");
  if (!LoadScreenNFUL("COTTAGE"))
    return;
  HitAnyKey(""); /* No font available to print text, so print nothing. */
#endif

#if 1
  /* More loading screen music. */
  PlayMusic("SQUAROOT");
  if (!LoadScreenNFUL("TITLESCREEN")) /* Error message on debug output. */
    return;
  HitAnyKey(""); /* No font available to print text, so print nothing. */
#endif

  /* Load our game screen, with a font and sprites too. */

  PlayMusic("LOADSONG");
  if (!LoadScreenNSCR("NTHPONG1"))
    return;

#if 0
  /* Print some text on screen which we can screen grab and put into the fully
     graphic title screen, and it will still look pixel perfect if we get it on
     a character cell 8 pixel boundary. */

  vdp_clearRows(0, 23);
  vdp_setCursor2(0, 0);
  /* 32 wide 12345678901234567890123456789012 */
  vdp_print("Nth Pong Wars copyright (c) 2025");
  vdp_newLine();
  vdp_print("by Alexander G. M. Smith.");
  vdp_newLine();
  vdp_print("hit any key to continue...");
  vdp_newLine();
  HitAnyKey(NULL);
#endif

  /* Print out some status info about the game.  Leave top row unmodified since
     it has score graphics from the default screen load. */

  vdp_clearRows(1, 23);
  vdp_setCursor2(0, 2);
  vdp_printJustified((char *) k_WelcomeText, 0, 32);
  vdp_newLine();
  CSFX_play(); /* Avoid a pause in the music due to slow printing. */
  SetUpAboutThisProgramText();
  vdp_printJustified(g_TempBuffer, 0, 32);
  vdp_newLine();
  HitAnyKey(NULL);

  if (memcmp(s_OriginalLocationZeroMemory, NULL,
  sizeof(s_OriginalLocationZeroMemory)) != 0)
  {
    HitAnyKey(k_CorruptedLowMemText);
    return;
  }

  /* Set up the tiles.  Directly map play area to screen for now rather than
     using a scrolling window, which is quite slow. */

  g_play_area_height_tiles = 23;
  g_play_area_width_tiles = 32;

  g_screen_height_tiles = 23;
  g_screen_width_tiles = 32;
  g_screen_top_X_tiles = 0;
  g_screen_top_Y_tiles = 1;

  g_play_area_col_for_screen = 0;
  g_play_area_row_for_screen = 0;

  if (!InitTileArray())
  {
    HitAnyKey("Failed to set up play area tiles.\n");
    return;
  }

  InitialisePlayers();

  InitialiseScores(); /* Do after tiles & players set up: calc initial score. */

  CSFX_stop(); /* Stop loading screen background music. */
  CSFX_start(NthEffects_a_z, true /* IsEffects */);
  CSFX_start(NthMusic_a_z, false /* IsEffects */); /* Game background music. */

  /* The main program loop.  Update things (which may take a while), then wait
     for vertical blanking to start, then copy data to the VDP quickly, then
     go back to updating things, etc.  Unfortunately we don't have enough time
     to do everything in one frame, so it's usually 30 frames per second,
     often dropping to 20 fps when there's lots of physics activity. */

  s_KeepRunning = true;
  while (true)
  {
    ProcessKeyboard();
    UpdatePlayerInputs();
    Simulate();
    if ((g_FrameCounter & 0x3F) == 0)
      AddNextPowerUpTile(); /* See coincident power-up before hitting it. */
    UpdateTileAnimations();
    UpdatePlayerAnimations();
    UpdateScores();

    /* Check for game exit here, after the updates have been done, but before
       the dirty flags have been cleared, so we can see what's taking up all
       the time being drawn. */

    if (!s_KeepRunning)
      break;

    /* Wait for the next vertical blank.  Check if our update took longer than
       a frame.  vdpIsReady counts number of vertical blank starts missed. */

    g_ScoreFramesPerUpdate = vdpIsReady + 1;
    vdp_waitVDPReadyInt(); /* Fixed version now sets vdpIsReady to zero. */ 

    /* Do the sprites first, since they're time critical to avoid glitches. */
    CopyPlayersToSprites();
    CopyTilesToScreen();
    CopyScoresToScreen();

    /* Update the audio hardware with the music being played.  Can debug which
       channels are busy, look in scores.c. */
    CSFX_play();

    /* Frame has been completed.  On to the next one.  16 bit wrap-around
       needed so time differences past the wrap still work. */

    g_FrameCounter++;

    if ((g_FrameCounter & 0x1F) == 0)
      g_ScoreGoal--; /* Fake decreasing of the goal about once per second. */

#if 0
    /* Every once in a while move the screen around the play area. */

    if ((g_FrameCounter & 0x1ff) == 0)
    {
      g_play_area_col_for_screen += 3;
      if (g_play_area_col_for_screen >= g_play_area_width_tiles)
      {
        g_play_area_col_for_screen = 0;
        g_play_area_row_for_screen += 3;
        if (g_play_area_row_for_screen >= g_play_area_height_tiles)
          g_play_area_row_for_screen = 0;
      }
      ActivateTileArrayWindow();
    }
#endif

#if 0
  /* Draw some new tiles once in a while, randomly moving around. */

  if ((g_FrameCounter & 31) == 19)
  {
    static uint8_t row, col;
    tile_owner newOwner;
    tile_pointer pTile;

    row++;
    if ((rand() & 255) > 200)
    { /* Rarely put in a power up. */
      newOwner = (rand() & 7) + OWNER_PUP_NORMAL;
    }
    else /* Regular tile change. */
    {
      newOwner = rand() & 0x07;
      if (newOwner >= OWNER_PUP_NORMAL)
        newOwner = OWNER_MAX; /* Skip power ups. */
    }
    if (newOwner < OWNER_MAX)
    {
      col++;
      if (col >= g_play_area_width_tiles)
        col = 0;
      if (row >= g_play_area_height_tiles)
        row = 0;
      pTile = g_tile_array_row_starts[row];
      if (pTile != NULL)
      {
        pTile += col;
        SetTileOwner(pTile, newOwner);
      }
    }
  }
#endif

#if 0 /* Check for corrupted memory every frame. */
    if (memcmp(s_OriginalLocationZeroMemory, NULL,
    sizeof(s_OriginalLocationZeroMemory)) != 0)
    {
      vdp_setCursor2(0, 0);
      vdp_print(k_CorruptedLowMemText);
    }
#endif
  }
  vdp_disableVDPReadyInt();
  CSFX_stop();

  if (memcmp(s_OriginalLocationZeroMemory, NULL,
  sizeof(s_OriginalLocationZeroMemory)) != 0)
    DebugPrintString(k_CorruptedLowMemText);

  DumpTilesToTerminal();
  strcpy(g_TempBuffer, "Frame count: ");
  AppendDecimalUInt16(g_FrameCounter);
  strcat(g_TempBuffer, "\n");
  DebugPrintString(g_TempBuffer);

#if 0
  /* If plain return isn't working.  Happens if main doesn't return void, or if
     you print too much CP/M output to the screen when in graphics mode.  To do
     a real reset, would need to switch ROM in before the jp 0 instruction. */
  DebugPrintString("Done.  Brute force exit to CP/M.\n");
  __asm
  jp 0;
  __endasm;
#endif
  DebugPrintString("Returning to operating system.\n");
}

