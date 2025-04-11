/* Nth Pong Wars - the multiplayer Pong inspired game.
 * Copyright © 2025 by Alexander G. M. Smith.
 *
 * AGMS20240721 Start a NABU version of the game, just doing the basic bouncing
 * ball using VDP video memory direct access, and our fixed point math.
 *
 * Compile for NABU + RetroNet Cloud CP/M (generates a usable .COM file),
 * Optionally with --math32 for 32 bit floating point, uses 5K extra memory.
 * Use this command line to compile the final version (remove
 * max-allocs-per-node for usable speed during development):
 *
 * zcc +cpm -v --list --c-code-in-asm -z80-verb -gen-map-file -gen-symbol-file -create-app -compiler=sdcc -O2 --opt-code-speed=all --max-allocs-per-node200000 --fverbose-asm --math32 -lndos main.c z80_delay_ms.asm z80_delay_tstate.asm l_fast_utoa.asm CHIPNSFX.asm Art/NthPongWarsMusic.asm Art/NthPongWarsExtractedEffects.asm -o "NTHPONG.COM" ; cp -v *.COM ~/Documents/NABU\ Internet\ Adapter/Store/D/0/
 *
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
/* #pragma output nostreams /* No disk based file streams? */
/* #pragma output nofileio /* Sets CLIB_OPEN_MAX to zero, also use -lndos? */
#pragma printf = "%f %d %ld %X %c %s" /* Need these printf formats. */
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
/* #include <math.h> /* For 32 bit floating point math routines. */
#include <string.h> /* For strlen. */
#include <malloc.h> /* For malloc and free, and initialising a heap. */

/* Use DJ Sure's Nabu code library, for VDP video hardware and the Nabu Network
   simulated data network.  Now that it compiles with the latest Z88DK compiler
   (lots of warnings about "function declarator with no prototype"),
   and it has a richer library for our purposes than the Z88DK NABU support.
   Here are some defines to select options in the library, see NABU-LIB.h
   for docs.  You may need to adjust the paths to fit where you downloaded it
   from https://github.com/DJSures/NABU-LIB.git   There's also a fixed up fork
   at https://github.com/agmsmith/NABU-LIB/tree/NthPongCustomisations */
#define BIN_TYPE BIN_CPM /* We're compiling for CP/M OS, not stand alone. */
/* #define DISABLE_KEYBOARD_INT /* Disable it to use only the CP/M keyboard. */
/* #define DISABLE_HCCA_RX_INT /* Disable if not using networking. */
/* #define DISABLE_VDP /* Disable if not using the Video Display Processor. */
/* #define DEBUG_VDP_INT /* Flash the Alert LED if VDP updates are too slow. */
#define DISABLE_CURSOR /* Don't flash during NABU-LIB keyboard input. */
#include "../../../NABU-LIB/NABULIB/NABU-LIB.h" /* Also includes NABU-LIB.c */
#include "../../../NABU-LIB/NABULIB/RetroNET-FileStore.h"

#if 0 /* We have our own VDP font now, don't need array using up memory. */
#define FONT_LM80C
#include "../../../NABU-LIB/NABULIB/patterns.h" /* Font as a global array. */
#endif

/* Temporary buffer used for sprinting into and for intermediate storage during
   screen loading, etc.  Avoids using stack space. */
#define TEMPBUFFER_LEN 512
char TempBuffer[TEMPBUFFER_LEN];

/* Our own game include files, some are source code!  Comes after NABU-LIB.h
   has been included and TempBuffer defined. */

#include "../common/fixed_point.c" /* Our own fixed point math. */
#include "LoadScreenICVGM.c"
#include "LoadScreenPC2.c"
#include "z80_delay_ms.h" /* Our hacked up version of time delay for NABU. */
#include "l_fast_utoa.h" /* Our hacked up version of utoa() to fix bugs. */
#include "CHIPNSFX.h" /* Music player glue functions. */
#include "Art/NthPong1.h" /* Graphics definitions to go with loaded data. */
#include "Art/NthPongWarsMusic.h" /* List of available Music loaded. */
#include "../common/tiles.c"
#include "../common/players.c"
#include "../common/simulate.c"
#include "../common/scores.c"
#include "../common/sounds.c"

/* Our globals and semi-global statics. */

static bool s_KeepRunning;


/*******************************************************************************
 * The usual press any key to continue prompt, in VDP graphics mode.  NULL for
 * a default message string.
 */
static char HitAnyKey(const char *MessageText)
{
  vdp_newLine();
  vdp_print((char *) (MessageText ? MessageText : "Press any key to continue."));
  return getChar();
}


/*******************************************************************************
 * Process Keyboard and Joystick inputs.
 */
#define DEBUG_KEYBOARD 0
static void ProcessKeyboard(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;

  static uint8_t iControlledPlayer = 3;
  /* Current player for cursor keys. */

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
    else if (letter >= 0x80) /* High bit set for joystick and NABU special keys. */
    {
      /* Joystick or special keys.  Ignore them, there are lots of them.
         NABU-LIB will parse the joystick keys for us. */
#if DEBUG_KEYBOARD
      /* printf("(%X)", letter); */
#endif
    }
    else
    {
#if DEBUG_KEYBOARD
      printf ("Got letter %d %c.\n", (int) letter, letter);
#endif
      if (letter == 'q')
      {
        s_KeepRunning = false;
      }
      else if (letter == 'a' || letter == 's' || letter == 'w' ||
      letter == 'z' || letter == '0')
      {
        pPlayer = g_player_array + iControlledPlayer;
        if (letter == 'a') /* Speed up leftwards. */
        {
          ADD_FX(&pPlayer->velocity_x, &gfx_Constant_MinusOne,
            &pPlayer->velocity_x);
        }
        else if (letter == 's') /* Speed up rightwards. */
        {
          ADD_FX(&pPlayer->velocity_x, &gfx_Constant_One,
            &pPlayer->velocity_x);
        }
        else if (letter == 'w') /* Speed up upwards. */
        {
          ADD_FX(&pPlayer->velocity_y, &gfx_Constant_MinusOne,
            &pPlayer->velocity_y);
        }
        else if (letter == 'z') /* Speed up downwards. */
        {
          ADD_FX(&pPlayer->velocity_y, &gfx_Constant_One,
            &pPlayer->velocity_y);
        }
        else if (letter == '0') /* Stop moving. */
        {
          ZERO_FX(pPlayer->velocity_x);
          ZERO_FX(pPlayer->velocity_y);
        }
#if DEBUG_KEYBOARD
        printf("Player %d changed speed to %f, %f.\n", iControlledPlayer,
          GET_FX_FLOAT(pPlayer->velocity_x),
          GET_FX_FLOAT(pPlayer->velocity_y));
#endif
      }
      else if (letter >= '1' && letter <= '4')
      {
        iControlledPlayer = letter - '1';
#if DEBUG_KEYBOARD
        printf("Now controlling player %d.\n", (int) iControlledPlayer);
#endif
      }
      else
      {
#if DEBUG_KEYBOARD
        printf ("Unhandled letter %d.\n", (int) letter);
#endif
      }
    }
  }

  /* Update the player's control inputs with joystick or keyboard input from
     the Human players. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE || pPlayer->brain == BRAIN_JOYSTICK)
    {
      /* These kinds of brains use Human input devices.  So read the devices. */
      if (iPlayer == iControlledPlayer)
        pPlayer->joystick_inputs = g_KeyboardFakeJoystickStatus;
      else
        pPlayer->joystick_inputs = (getJoyStatus(iPlayer) & 0x1F);
    }
  }
}


/*******************************************************************************
 * Main program.  Variables in main() that also need to be accessed from
 * assembler have to be globalish.  Make them static but global scope.
 */
static uint16_t s_StackFramePointer = 0;
static uint16_t s_StackPointer = 0;

void main(void)
{
  /* Save some stack space, variables that persist in main() can be static. */
  static char s_OriginalLocationZeroMemory[16];
  static char s_OriginalStackMemory[16];

  /* A few local variables to force stack frame creation, sets IX register. */
  unsigned int sTotalMem, sLargestMem;

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
     stack. */
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

  /* Detect memory corruption in our stack. */
  memcpy(s_OriginalStackMemory, (char *) (s_StackFramePointer + 0),
    sizeof(s_OriginalStackMemory));

  /* Note that printf goes through CP/M and scrambles the video memory (it's
     arranged differently), so don't use printf during graphics mode.  However
     the NABU CP/M has a screen text buffer and that will be restored to the
     screen when the program exits, so you can see printf output after exit.
     Or if you've redirected output to a remote device (telnet server), you can
     see it there and it doesn't mess up the screen. */

  printf("Welcome to the Nth Pong Wars NABU game.\n"
    "Copyright 2025 by Alexander G. M. Smith,\n"
    "contact me at agmsmith@ncf.ca.  Started\n"
    "February 2024, see the blog at\n"
    "https://web.ncf.ca/au829/WeekendReports/20240207/NthPongWarsBlog.html\n"
    "Released under the GNU General Public\n"
    "License version 3.\n");

  printf("Compiled on " __DATE__ " at " __TIME__ ".\n");
  mallinfo(&sTotalMem, &sLargestMem);
  printf("Heap has %d bytes free, largest %d.\n", sTotalMem, sLargestMem);
  printf("Using the SDCC compiler, unknown version.\n");

  /* Weird bug in Z88DK where printf("$%X $$$$$%X", a, b); loses any
     number of dollar signs before the second %X. */
#if 0
  printf("Stack pointer is $%X, frame $%X.\n",
    (int) s_StackPointer, (int) s_StackFramePointer);
#else
  printf("Stack pointer is $%X, frame %c%X.\n",
    (int) s_StackPointer, '$', (int) s_StackFramePointer);
#endif
/* printf ("Hit any key... ");
  printf ("  Got %c.\n", getchar()); /* CP/M compatible keyboard input. */

  if (memcmp(s_OriginalStackMemory, (char *) (s_StackFramePointer + 0),
  sizeof(s_OriginalStackMemory)) != 0)
  {
    printf("Corrupted Stack Memory before anything done!\n");
    return;
  }

  if (memcmp(s_OriginalLocationZeroMemory, NULL,
  sizeof(s_OriginalLocationZeroMemory)) != 0)
  {
    printf("Corrupted zero page Memory before anything done!\n");
    return;
  }

  initNABULib(); /* No longer can use CP/M text input or output. */

  vdp_init(VDP_MODE_G2, VDP_WHITE /* fgColor not applicable */,
    VDP_BLACK /* bgColor */, true /* bigSprites 16x16 pixels but fewer names */,
    false /* magnify */, true /* splitThirds */);
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
    return;
  }
  z80_delay_ms(100); /* No font loaded, just graphics, so no hit any key. */
#endif

  /* Load our game screen, with a font and sprites defined. */

  if (!LoadScreenICVGM("NTHPONG\\NTHPONG1.ICV"))
  {
    printf("Failed to load NTHPONG\\NTHPONG1.ICV.\n");
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
    printf("Failed to set up play area tiles.\n");
    return;
  }

  InitialisePlayers();

  InitialiseScores(); /* Do after tiles & players set up: calc initial score. */

  CSFX_stop(); /* Initialise the CHIPNSFX music player library. */
  CSFX_start(NthMusic_a_z, false /* IsEffects */);

  /* The main program loop.  Update things (which may take a while), then wait
     for vertical blanking to start, then copy data to the VDP quickly, then
     go back to updating things, etc.  Unfortunately we don't have enough time
     to do everything in one frame, so it's usually 30 frames per second. */

  s_KeepRunning = true;
  vdp_enableVDPReadyInt();
  while (true)
  {
    ProcessKeyboard();
    UpdatePlayerInputs();
    Simulate();
    UpdateTileAnimations();
    UpdatePlayerAnimations();
    UpdateScores();

    /* Check if our update took longer than a frame.  vdpIsReady counts number
       of vertical blank starts missed. */

    g_ScoreFramesPerUpdate = vdpIsReady + 1;
#if 1
    if (g_ScoreFramesPerUpdate >= 4) /* Non-zero means we missed frames. */
      playNoteDelay(2 /* Channel 0 to 2 */,
        10 + g_ScoreFramesPerUpdate /* Higher pitch if more missed */,
        40 /* Time delay to hold note. */);
#endif

    /* Check for game exit here, after the updates have been done, but before
       the dirty flags have been cleared, so we can see what's taking up all
       the time being drawn. */

    if (!s_KeepRunning)
      break;

    vdp_waitVDPReadyInt(); /* Fixed version now sets vdpIsReady to zero. */ 

    /* Do the sprites first, since they're time critical to avoid glitches. */
    CopyPlayersToSprites();
    CopyTilesToScreen();
    CopyScoresToScreen();

    /* Update the audio hardware with the music being played. */
    CSFX_play();

    /* Frame has been completed.  On to the next one.  Wrap after 10000 since
       we only have a four decimal digit display. */

    if (++g_FrameCounter >= 10000)
      g_FrameCounter = 0;

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

#if 0 /* Check for corrupted memory. */
    if (memcmp(s_OriginalLocationZeroMemory, NULL,
    sizeof(s_OriginalLocationZeroMemory)) != 0)
    {
      vdp_setCursor2(0, 0);
      vdp_print("Corrupted NULL Memory!");
    }
    if (memcmp(s_OriginalStackMemory, (char *) (s_StackFramePointer + 0),
    sizeof(s_OriginalStackMemory)) != 0)
    {
      vdp_setCursor2(0, 1);
      vdp_print("Corrupted Stack Memory!");
    }
#endif
  }
  vdp_disableVDPReadyInt();
  CSFX_stop();

  DumpTilesToTerminal();
  printf ("Frame count: %d\n", g_FrameCounter);

  if (memcmp(s_OriginalLocationZeroMemory, NULL,
  sizeof(s_OriginalLocationZeroMemory)) != 0)
  {
    printf("Corrupted NULL Memory!");
  }

  if (memcmp(s_OriginalStackMemory, (char *) (s_StackFramePointer + 0),
  sizeof(s_OriginalStackMemory)) != 0)
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

