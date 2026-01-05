/******************************************************************************
 * Nth Pong Wars, levels.c for loading levels, screens and other things.
 *
 * AGMS20251129 - Started this code file.
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

#include "levels.h"

#ifndef NUL  /* Our end of string marker. */
  #define NUL ((char) 0)
#endif

const char kMagicWordCopyright[] = "Copyright";
const char kMagicWordVersion[] = "Version";

bool gVictoryModeFireButtonPress = true;
bool gVictoryModeJoystickPress = false;
bool gVictoryModeHighestTileCount = false;
uint8_t gVictoryWinningPlayer = MAX_PLAYERS + 2;

char gLevelName[MAX_LEVEL_NAME_LENGTH] = "TITLE";
char gWinnerNextLevelName[MAX_PLAYERS+2][MAX_LEVEL_NAME_LENGTH];
char gBookmarkedLevelName [MAX_LEVEL_NAME_LENGTH] = "TITLE";

static uint16_t sVictoryTimeoutFrame = 0;
/* Frame counter when the current level times out and player 4 (the Fire
   button) is declared the winner.  Zero if the timeout isn't active. */

static bool sFirstTileQuotaKeywordClears = true;
/* A flag that's set for each level load so we know that all tile quotas
   need to be cleared the first time we see the keyword. */

#define MAX_NUMERIC_ARGUMENTS 6
static int16_t sNumericArgumentsDecoded[MAX_NUMERIC_ARGUMENTS];
/* To avoid duplicating code, we have a generic function for reading in comma
   separated arguments after a keyword.  That function deposits the values in
   this array. */


/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed.
*/
bool VictoryConditionTest(void)
{
  uint8_t winningPlayer = MAX_PLAYERS + 2;
  /* 0 to 3 are players, 4 is Fire button, 5 is Timeout, 6 is invalid. */

  uint8_t i;
  player_pointer pPlayer = g_player_array;
  for (i = 0; i < MAX_PLAYERS; i++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    /* First player to press their fire button wins. */

    if (gVictoryModeFireButtonPress)
    {
      if (pPlayer->joystick_inputs & Joy_Button)
      {
        winningPlayer = i;
        break;
      }
    }

    /* First player to move their joystick selects the winner by what direction
       they moved it, or if they pressed the fire button. */

    if (gVictoryModeJoystickPress)
    {
      /* Make sure the joystick bits are contiguous, since we shift them around
         assuming that they are.  And that shifting the fire button gets us to
         MAX_PLAYERS.  Otherwise this joystick to player number code needs to
         be redone. */
      COMPILER_VERIFY(JOYSTICK_DIRECTION_MASK == 0b00001111);
      COMPILER_VERIFY(Joy_Button == 0b00010000);
      COMPILER_VERIFY(MAX_PLAYERS == 4);

      uint8_t buttonBits = pPlayer->joystick_inputs; /* Unused bits already 0 */
      if (buttonBits != 0)
      {
        uint8_t bitCount = 0;
        while ((buttonBits & 1) == 0)
        {
          buttonBits >>= 1;
          bitCount++;
        }
        winningPlayer = bitCount; /* Will be from 0 to 4 (MAX_PLAYER == 4). */
        break;
      }
    }

    /* First player whose tile count reaches the countdown timer wins. */

    if (gVictoryModeHighestTileCount)
    {
      if (g_TileOwnerCounts[OWNER_PLAYER_1 + i] >= g_ScoreGoal)
      {
        winningPlayer = i;
        break;
      }
    }
  }

  /* Check for a level timeout if we don't have a winner yet. */

  if (sVictoryTimeoutFrame && (sVictoryTimeoutFrame == g_FrameCounter) &&
  (winningPlayer >= MAX_PLAYERS + 2))
    winningPlayer = MAX_PLAYERS + 1; /* Timeout "player" is the winner. */

  /* If we have a winner (or MAX_PLAYERS = 4 for fire button pressed in
     joystick mode or MAX_PLAYERS+1 == 5 for timeout), use their next level as
     the one we want to load next. */

  if (winningPlayer < MAX_PLAYERS + 2)
  {
    gVictoryWinningPlayer = winningPlayer;
    strcpy(gLevelName, gWinnerNextLevelName[winningPlayer]);

    strcpy(g_TempBuffer, "Winner is player #");
    AppendDecimalUInt16(winningPlayer);
    strcat(g_TempBuffer, ", next level \"");
    strcat(g_TempBuffer, gLevelName);
    strcat(g_TempBuffer, "\".\n");
    DebugPrintString(g_TempBuffer);

    return true;
  }

  return false;
}


/* Internal utilities to read some text from the level file.  Uses a temporary
   buffer to cache data so that it reads in larger more efficient chunks.  If
   the buffer is empty, refill it almost but not quite full (for undo reasons)
   from the already open level file.
*/
#define LEVEL_READ_BUFFER_SIZE 256 /* For easier wrap-around when reading. */
static FileHandleType sLevelFileHandle;
static uint8_t sLevelReadPosition;
static uint8_t sLevelWritePosition;
static char *sLevelBuffer; /* Points to a temporary on-stack buffer. */

/* Read the next byte, refill the buffer if needed, returns 0 on end of file.
*/
static char LevelReadByte(void)
{
  if (sLevelReadPosition != sLevelWritePosition)
    return sLevelBuffer[sLevelReadPosition++];

  /* Buffer is empty, refill all but one byte, so we can read ahead and then
     put back that byte without the buffer suddenly seeming to be empty. */

  uint16_t amountToRead =
    (LEVEL_READ_BUFFER_SIZE - 1) - (uint16_t) sLevelWritePosition;
  if (amountToRead <= 0)
  {
    /* Read at least one byte.  Happens when on the last byte in the buffer. */
    amountToRead = 1;
  }
  uint16_t amountRead = rn_fileHandleReadSeq(sLevelFileHandle,
    sLevelBuffer + sLevelWritePosition, 0 /* buffer offset */, amountToRead);
  if (amountRead == 0)
    return NUL; /* End of file or error, return zero character. */
  sLevelWritePosition += amountRead;
  return sLevelBuffer[sLevelReadPosition++];
}


/* Undoes a read of one byte of the level file.  Can only call once in a row
   since the buffer filling is only guaranteed to not overwrite just the last
   read byte.  Also, don't call this if you got end of file.
*/
static void LevelUndoReadByte(void)
{
  sLevelReadPosition--;
}


/* Look at the next byte, but put it back in the buffer for later reading.
   This is so you can read ahead a bit to remove leading spaces etc.
*/
static char LevelPeekNextByte(void)
{
  char letter = LevelReadByte();
  if (letter != 0)
    LevelUndoReadByte();
  return letter;
}


/* Skip spaces and tabs until a non-blank character or end of file.  Returns
   FALSE when it hits end of file, TRUE otherwise.
*/
static bool LevelSkipSpaces(void)
{
  while (true)
  {
    char letter = LevelReadByte();

    if (letter == 0) /* End of file. */
      return false;

    if (!isblank(letter)) /* Undo the read, no more spaces, done. */
    {
      LevelUndoReadByte();
      break;
    }
  }
  return true;
}


/* Read everything until the next end of line.  Then skip past multiple end of
   line (LF, CR) characters until the start of the next line with something on
   it.  That means it works for both LF and CRLF terminated lines.  The CR/LF
   characters are not added to the buffer.  If it runs out of buffer space, it
   fills as much as it could then skips over the rest of the line data.
   Returns true if it read some data, false if it ran into end of file
   immediately while trying to read data.  Buffer will be NUL terminated.
   BufferSize should be at least 2, max 255, zero will trash memory, 1 will
   have spurrious end of file indications.
*/

/* Make sure our large temp buffer is safe to use in LevelReadLine() with a
   hard coded 255 buffer length, since the length here is byte sized. */
COMPILER_VERIFY(sizeof(g_TempBuffer) >= 255);

static bool LevelReadLine(char *Buffer, uint8_t BufferSize)
{
  char *pDest = Buffer;
  uint8_t dataSize = BufferSize - 1; /* Save space for final NUL character. */
  uint8_t sizeSoFar = 0;
  char letter = NUL;
  while (true)
  {
    letter = LevelReadByte();
    if (letter == NUL || letter == 10 /* LF */ || letter == 13 /* return */)
      break;
    if (sizeSoFar < dataSize)
    {
      *pDest++ = letter;
      sizeSoFar++;
    }
  }
  *pDest = NUL; /* Terminate the buffer string. */

  if (letter == NUL)
    return (sizeSoFar != 0); /* FALSE if end of file before any data read. */

  /* Now skip over further LF and CR characters. */

  while (true)
  {
    char letter = LevelReadByte();

    if (letter == NUL) /* End of file. */
      break;

    if (letter != 10 /* line feed */ && letter != 13 /* carriage return */)
    { /* Start of the next line, unread it and we're done. */
      LevelUndoReadByte();
      break;
    }
  }
  return true;
}


/* Read to the start of the next line.  Useful for skipping over the remainder
   of a line that you don't want to process. */
static bool LevelReadToStartOfNextLine(void)
{
  char Buffer[10];
  return LevelReadLine(Buffer, sizeof(Buffer));
}


/* Read a word from the level file.  Stops at the delimiter (it gets consumed)
   or end of line (doesn't get consumed) or NUL.  If there is more text than
   will fit in the buffer, read and skip the excess.  Fills the given Buffer
   with the text up to but not including the delimiter and adds a NUL.
   Returns FALSE if end of file was encountered before any data was read.
   BufferSize should be at least 2, max 255, zero will trash memory, 1 will
   have spurious end of file indications.
*/
static bool LevelReadWord(
  char *Buffer, uint8_t BufferSize, char Delimiter)
{
  char *pDest = Buffer;
  uint8_t dataSize = BufferSize - 1; /* Save space for final NUL character. */
  uint8_t sizeSoFar = 0;
  char letter = NUL;

  while (true)
  {
    letter = LevelReadByte();
    if (letter == NUL || letter == Delimiter)
      break;
    if (letter == 13 || letter == 10) /* Carriage return or line feed. */
    { /* Leave the end of line intact, so it can be read after this word.
         Also means that if you do several read words in a row, they will all
         return empty strings when you are up against the end of the line. */
      LevelUndoReadByte();
      break;
    }
    if (sizeSoFar < dataSize)
    {
      *pDest++ = letter;
      sizeSoFar++;
    }
  }
  *pDest = NUL;

  /* Returns FALSE when end of file was hit before reading any data. */
  return !(sizeSoFar == 0 && letter == NUL);
}


/* Read a line and trim off leading and trailing spaces and tabs.
*/
bool LevelReadAndTrimLine(char *Buffer, uint8_t BufferSize)
{
  if (!LevelSkipSpaces())
    return false;
  if (!LevelReadLine(Buffer, BufferSize))
    return false;

  uint8_t length = strlen(Buffer);
  if (length <= 0)
    return true;

  char *pChar = Buffer - 1 + length; /* Point at the last non-NUL character. */
  while (pChar >= Buffer)
  {
    if (!isblank(*pChar))
      break; /* Terminate the string just after the last non-blank character. */
    pChar--;
  }
  pChar[1] = NUL;
  return true;
}


/* Read some the comma separated arguments following a keyword, converting to
   binary and storing in sNumericArgumentsDecoded[].  The number of arguments
   is specified, if there are fewer then unfilled elements of the array will
   be zero.  Returns FALSE if no data at all was read (usually due to end of
   file).  Doesn't read the remainder of the line after the last number and
   comma, so you may need to purge that.
*/
bool LevelReadNumericArguments(uint8_t NumberOfArguments)
{
  uint8_t i, maxCount;
  char numberText[10];
  bzero(sNumericArgumentsDecoded, sizeof(sNumericArgumentsDecoded));

  maxCount = MAX_NUMERIC_ARGUMENTS;
  if (NumberOfArguments < maxCount)
    maxCount = NumberOfArguments;
  for (i = 0; i < maxCount; i++)
  {
    if (!LevelReadWord(numberText, sizeof(numberText), ','))
      break; /* End of file. */
    sNumericArgumentsDecoded[i] = atoi(numberText);
  }

#if 0
    strcpy(g_TempBuffer, "Read ");
    AppendDecimalUInt16(i);
    strcat(g_TempBuffer, " numeric arguments: ");
    uint8_t j;
    for (j = 0; j < i; j++)
    {
      AppendDecimalUInt16(sNumericArgumentsDecoded[j]);
      if (j + 1 != i)
        strcat(g_TempBuffer, ", ");
    }
    strcat(g_TempBuffer, ".\n");
    DebugPrintString(g_TempBuffer);
#endif

  return i > 0; /* TRUE if at least one number was successfully read. */
}


/* This keyword loads a full screen (*.NFUL) graphic.  Returns true if you can
   continue processing, false to abort the level file load.  Should usually
   print a debug message if that happens.
*/
bool KeywordFullScreen(void)
{
  char screenFileName[MAX_FILE_NAME_LENGTH];
  if (!LevelReadAndTrimLine(screenFileName, sizeof(screenFileName)))
    return true;
  return LoadScreenNFUL(screenFileName);
}


/* This keyword loads a font based screen (*.NSCR) which includes sprites.
   Returns false to abort the level file load.
*/
bool KeywordCharFontSpriteScreen(void)
{
  char screenFileName[MAX_FILE_NAME_LENGTH];
  if (!LevelReadAndTrimLine(screenFileName, sizeof(screenFileName)))
    return true;
  return LoadScreenNSCR(screenFileName);
}


/* This draws some text on the screen, using whatever font is currently loaded.
   You specify a line to start drawing the text, left margin for the first line
   and then left margin for subsequent lines (in case you want to indent a
   paragraph) and right margin.  The text will be wrapped to fill that spot,
   possibly taking up several lines if needed.  Magic words are recognised and
   replace the text with something else.
*/
bool KeywordTextOnScreen(void)
{
  /* Read the line number, first left margin, subsequent left, right margin. */

  if (!LevelReadNumericArguments(4))
    return false;

  /* Yes, vdp_setCursor2() has input sanity checking. */
  vdp_setCursor2(sNumericArgumentsDecoded[1] /* column */,
    sNumericArgumentsDecoded[0] /* row */);

  /* Read the line of text from the level file and print it. */

  SoundUpdateIfNeeded();
  if (!LevelReadAndTrimLine(g_TempBuffer, 255))
    return false;

  SoundUpdateIfNeeded();
  vdp_printJustified((char *) StockTextMessages(g_TempBuffer),
    sNumericArgumentsDecoded[2], sNumericArgumentsDecoded[3]);

  return true;
}


/* This keyword loads just the name table, the characters of a screen (*.NCHR).
   Uses the previously loaded font.  Returns false to abort the level file load.
*/
bool KeywordCharImageScreen(void)
{
  char screenFileName[MAX_FILE_NAME_LENGTH];
  if (!LevelReadAndTrimLine(screenFileName, sizeof(screenFileName)))
    return true;
  return LoadScreenNCHR(screenFileName);
}


/* This keyword loads the background music (*.CHIPNSFX).
*/
bool KeywordBackgroundMusic(void)
{
  char musicFileName[MAX_FILE_NAME_LENGTH];
  if (!LevelReadAndTrimLine(musicFileName, sizeof(musicFileName)))
    return true;
  PlayMusic(musicFileName);
  return true;
}


/* Make the current level end after a given number of seconds. */
bool KeywordPlayTimeout(void)
{
  if (!LevelReadNumericArguments(1))
    return false;
  sVictoryTimeoutFrame = sNumericArgumentsDecoded[0] * 20 /* Assume 20hz */;

  if (sVictoryTimeoutFrame != 0)
  { /* User wants a timeout, we need a global frame number to trigger it. */
    sVictoryTimeoutFrame += g_FrameCounter;
    if (sVictoryTimeoutFrame == 0)
      sVictoryTimeoutFrame = 1; /* Handle rare case of hitting zero. */
  }

  return true;
}


/* This keyword specifies the base level name for the next level.  It is
   followed by a player identification, either "All" or "P0", "P1", "P2",
   "P3" or a joystick direction + Fire + Timeout to identify which next level
   gets set.  The player is followed by a comma and the level name is the rest
   of the line.
*/
bool KeywordLevelNext(void)
{
  char levelName[MAX_LEVEL_NAME_LENGTH];
  char playerName[8]; /* Longest is "Timeout" */

  uint8_t iPlayer = MAX_PLAYERS + 3;
  /* 0 to 3 are players, 4 is Fire button, 5 is Timeout, 6 is All, 7 is none. */

  static struct LevelOptionStruct {
    const char *optionWord;
    uint8_t playerNumber;
  } sLevelNextOptions[] = {
    {"All", MAX_PLAYERS + 2},
    {"P0", 0},
    {"P1", 1},
    {"P2", 2},
    {"P3", 3},
    {"Left", 0},
    {"Down", 1},
    {"Right", 2},
    {"Up", 3},
    {"Fire", 4},
    {"Timeout", 5},
    {NULL, MAX_PLAYERS + 3}
  };

  /* Get the player number, 0 to 3 for normal players, or 4 for Fire button,
     or 5 for Timeout, or 6 to signify all players.  MAX_PLAYERS+3 == 7 for
     invalid. */

  if (!LevelReadWord(playerName, sizeof(playerName), ','))
    goto ErrorExit;

  struct LevelOptionStruct *pOption;
  pOption = sLevelNextOptions;
  const char *optionWord;
  while ((optionWord = pOption->optionWord) != NULL)
  {
    if (strcasecmp(playerName, optionWord) == 0)
    {
      iPlayer = pOption->playerNumber;
      break;
    }
    pOption++;
  }
  if (iPlayer >= MAX_PLAYERS + 3)
    goto ErrorExit; /* No recognised option found. */

  if (!LevelReadAndTrimLine(levelName, sizeof(levelName)))
    goto ErrorExit;

  /* Copy the name to the appropriate player's NextLevel file name. */

  if (iPlayer < MAX_PLAYERS + 2)
    strcpy(gWinnerNextLevelName[iPlayer], levelName);
  else /* All players picked.  iPlayer == MAX_PLAYERS + 2 */
  {
    uint8_t i;
    for (i = 0; i < MAX_PLAYERS + 2; i++)
      strcpy(gWinnerNextLevelName[i], levelName);
  }
  return true;

ErrorExit:
  DebugPrintString("LevelNext: needs P0 to P3, All, "
    "Up/Down/Left/Right/Fire/Timeout then a comma & level name.\n");
  return false;
}


/* Save a named level for use later.  Possibly many levels later.
*/
bool KeywordLevelBookmark(void)
{
  LevelReadAndTrimLine(gBookmarkedLevelName, sizeof(gBookmarkedLevelName));
  return true;
}


/* Remove all the players from the game.  Lets you have an AI only game.
*/
bool KeywordRemovePlayers(void)
{
  /* Throw away the rest of this line, it's not used. */
  LevelReadToStartOfNextLine();

  DeassignPlayersFromDevices();
  return true;
}


/* Set the desired number of AI players.  More AI players get added every few
  seconds to meet this quota if there are empty player spots.
*/
bool KeywordMaxAIPlayers(void)
{
  if (!LevelReadNumericArguments(1))
    return false;
  gLevelMaxAIPlayers = sNumericArgumentsDecoded[0];

  return true;
}


/* Set the starting point of each AI player's code.  That will set the program
   they are using and thus their behaviour.
*/
bool KeywordAIPlayerCodeStart(void)
{
  if (!LevelReadNumericArguments(MAX_PLAYERS))
    return false;

  uint8_t iPlayer;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
    g_target_start_indices[iPlayer] = sNumericArgumentsDecoded[iPlayer];

  return true;
}


/* When playing in countdown mode, the first player to reach this many tiles in
   their colour wins.  The count starts at this value and counts down about once
   per second.  If you don't specify it, it gets set to the number of tiles in
  the level.
*/
bool KeywordCountdownStart(void)
{
  if (!LevelReadNumericArguments(1))
    return false;
  gVictoryStartingTileCount = sNumericArgumentsDecoded[0];

  return true;
}


/* How should this level be run?  Should the game engine be turned on?  Or is
   it just a static screen waiting for a button press?  Set up the appropriate
   things for the selected mode.
*/
bool KeywordGameMode(void)
{
  char modeName[MAX_FILE_NAME_LENGTH];
  if (!LevelReadAndTrimLine(modeName, sizeof(modeName)))
    return true; /* Early end of file.  Leave mode unchanged. */

  if (strcasecmp(modeName, "Slide") == 0)
  {
    gVictoryModeFireButtonPress = true;
    gVictoryModeJoystickPress = false;
    gVictoryModeHighestTileCount = false;
  }
  else if (strcasecmp(modeName, "Trivia") == 0)
  {
    gVictoryModeFireButtonPress = false;
    gVictoryModeJoystickPress = true;
    gVictoryModeHighestTileCount = false;
  }
  else if (strcasecmp(modeName, "PongWar") == 0)
  {
    gVictoryModeFireButtonPress = false;
    gVictoryModeJoystickPress = false;
    gVictoryModeHighestTileCount = true;
  }
  else
  {
    DebugPrintString("Unrecognised GameMode: \"");
    DebugPrintString(modeName);
    DebugPrintString("\", try Slide, Trivia, PongWar.\n");
  }

  return true;
}


/* Set the size of the game board, width and height in tiles.
*/
bool KeywordBoardSize(void)
{
  if (!LevelReadNumericArguments(2))
    return false;

  uint8_t width = sNumericArgumentsDecoded[0];
  uint8_t height = sNumericArgumentsDecoded[1];

  /* Sanity check and use the numbers. */

  if (width <= 0)
    width = 1;
  g_play_area_width_tiles = width;

  if (height <= 0)
    height = 1;
  g_play_area_height_tiles = height;

  if (!InitTileArray())
  {
    strcpy(g_TempBuffer, "BoardSize of ");
    AppendDecimalUInt16(width);
    strcat(g_TempBuffer, "x");
    AppendDecimalUInt16(height);
    strcat(g_TempBuffer, " is too big to fit in memory.\n");
    DebugPrintString(g_TempBuffer);

    /* Go back to a safe size so we don't crash with no board. */

    g_play_area_width_tiles = 20;
    g_play_area_height_tiles = 15;
    return false;
  }
  return true;
}


/* Set the rectangle on the screen where the game board will be drawn.  Can be
   smaller or larger than the actual board size.  Tiles won't get drawn outside
   the rectangle, and non-existent tiles won't get drawn at all.  The four
   arguments after the keyword are X character position of top left corner
   (0 to 31), Y of top left corner (0 to 23), width in tiles, height in tiles.
*/
bool KeywordBoardScreen(void)
{
  if (!LevelReadNumericArguments(6))
    return false;

  g_screen_top_X_tiles = sNumericArgumentsDecoded[0];
  g_screen_top_Y_tiles = sNumericArgumentsDecoded[1];
  g_screen_width_tiles = sNumericArgumentsDecoded[2];
  g_screen_height_tiles = sNumericArgumentsDecoded[3];
  g_play_area_col_for_screen = sNumericArgumentsDecoded[4];
  g_play_area_row_for_screen = sNumericArgumentsDecoded[5];

  ActivateTileArrayWindow();
  return true;
}


/* Read tile data from the level file and set tiles and initial player
   positions.  Stops on a line with "END".  Ignores spaces and comment lines.
*/
bool KeywordBoardTileData(void)
{
  uint8_t row = 0;
  while (LevelReadAndTrimLine(g_TempBuffer, 255))
  {
    SoundUpdateIfNeeded();

    if (strcasecmp(g_TempBuffer, "END") == 0)
      break; /* End of the tile data. */
    if (g_TempBuffer[0] == '#')
      continue; /* Skip over comment lines. */

    /* Decode the tiles in the line. */

    uint8_t column = 0;
    const char *pChar;
    char letter;
    for (pChar = g_TempBuffer; (letter = *pChar) != 0; pChar++)
    {
      if (isblank(letter))
        continue; /* Skip blanks between tile codes. */

      /* Convert the level file letter to a tile type. */
      tile_owner tileType;
      for (tileType = 0; tileType < OWNER_MAX; tileType++)
      {
        if (g_TileOwnerLetters[tileType] == letter)
          break;
      }

      /* Default unknown tile types to empty tiles. */
      if (tileType >= OWNER_MAX)
        tileType = OWNER_EMPTY;

      tile_pointer pTile = TileForColumnAndRow(column, row);
      if (pTile == NULL)
        break; /* Ignore the rest of this line, no tiles here. */
      SetTileOwner(pTile, tileType);

      /* If it is a player coloured tile, move the player there. */

      uint8_t iPlayer = tileType - OWNER_PLAYER_1;
      if (iPlayer < MAX_PLAYERS)
      {
        player_pointer pPlayer = g_player_array + iPlayer;
        pPlayer->starting_level_pixel_x = pTile->pixel_center_x;
        pPlayer->starting_level_pixel_y = pTile->pixel_center_y;
        pPlayer->starting_level_pixel_flying_height = FLYING_ABOVE_TILES_HEIGHT;
      }

      /* Successfully set one tile, advance to the next column. */
      column++;
    }

    if (column > 0) /* If we read something, count this as a row. */
      row++;
  }

  return row > 0; /* TRUE if we actually read anything. */
}


/* Read a tile type and quota number from the level file.  Set the corresponding
   quota for that kind of power-up.  If it's the first time, clear all the
   power-ups.
*/
bool KeywordTileQuota(void)
{
  if (sFirstTileQuotaKeywordClears)
  {
    bzero(g_TileOwnerQuotas, sizeof(g_TileOwnerQuotas));
    sFirstTileQuotaKeywordClears = false;
  }

  /* Read the tile type code. */
  if (!LevelReadWord(g_TempBuffer, 255, ','))
    return false;

  /* Convert the level file letter to a tile type.  Do nothing fatal for
     unknown possibly future level codes. */
  tile_owner tileType;
  for (tileType = 0; tileType < OWNER_MAX; tileType++)
  {
    if (g_TileOwnerLetters[tileType] == g_TempBuffer[0])
      break;
  }

  /* Read the quota number. */

  if (!LevelReadNumericArguments(1))
    return false;
  uint8_t quotaAmount = sNumericArgumentsDecoded[0];

  /* Default unknown tile types ignored. */
  if (tileType < OWNER_MAX)
    g_TileOwnerQuotas[tileType] = quotaAmount;

  return true;
}


/******************************************************************************
 * Handle all level keywords.  We have a table of the word and the
 * corresponding function to call.
 */

typedef bool (*KeywordFunctionPointer)(void);
/* Generic function to read and process a keyword.  Returns FALSE to abort. */

typedef struct KeyWordCallStruct {
  const char *keyword;
  KeywordFunctionPointer function;
} *KeyWordCallPointer;

static struct KeyWordCallStruct kKeywordFunctionTable[] = {
  {"ScreenFull", KeywordFullScreen},
  {"ScreenFont", KeywordCharFontSpriteScreen},
  {"ScreenChar", KeywordCharImageScreen},
  {"ScreenText", KeywordTextOnScreen},
  {"Music", KeywordBackgroundMusic},
  {"PlayTimeout", KeywordPlayTimeout},
  {"LevelNext", KeywordLevelNext},
  {"LevelBookmark", KeywordLevelBookmark},
  {"RemovePlayers", KeywordRemovePlayers},
  {"MaxAIPlayers", KeywordMaxAIPlayers},
  {"AIPlayerCodeStart", KeywordAIPlayerCodeStart},
  {"InitialCount", KeywordCountdownStart},
  {"GameMode", KeywordGameMode},
  {"BoardSize", KeywordBoardSize},
  {"BoardScreen", KeywordBoardScreen},
  {"BoardTileData", KeywordBoardTileData},
  {"TileQuota", KeywordTileQuota},
  {NULL, NULL}
};

/* Read the level file keyword by keyword.  Returns FALSE on abort, usually
   something wrong about the level file, like bad tile map syntax.
*/
static bool ProcessLevelKeywords(void)
{
  #define MAX_LEVEL_KEYWORD_LENGTH 32
  char keyWord[MAX_LEVEL_KEYWORD_LENGTH];

  sFirstTileQuotaKeywordClears = true;

  while (true)
  {
    SoundUpdateIfNeeded();

    /* Read the next keyword.  Skip leading spaces.  Skip comment lines. */

    if (!LevelSkipSpaces())
      break; /* Hit end of file. */

    if (LevelPeekNextByte() == '#')
    {
      /* Just read the rest of the line and discard it. */
      if (!LevelReadLine(keyWord, sizeof(keyWord)))
        break; /* End of file encountered. */
      continue; /* Continue on to the next line after the comment. */
    }

    if (!LevelReadWord(keyWord, sizeof(keyWord), ':'))
      break; /* End of file encountered.  Job done. */

    if (keyWord[0] == 0) /* Empty line keyword, discard rest of line. */
    {
      if (!LevelReadAndTrimLine(keyWord, sizeof(keyWord)))
        break; /* End of file while discarding. */
      continue;
    }

    strcpy(g_TempBuffer, "Now doing level keyword \"");
    strcat(g_TempBuffer, keyWord);
    strcat(g_TempBuffer, "\".\n");
    DebugPrintString(g_TempBuffer);

    if (!LevelSkipSpaces()) /* For convenience, skip spaces after the colon. */
      break; /* Hit end of file, always need a value after the keyword. */

    /* Got the keyword, do special processing for each one. */

    KeyWordCallPointer pKeywordCall = kKeywordFunctionTable;
    while (pKeywordCall->keyword != NULL)
    {
      if (strcasecmp(pKeywordCall->keyword, keyWord) == 0)
      {
        if (!pKeywordCall->function())
          return false; /* Abort, abort! */
        break; /* Processed this keyword, don't bother searching further. */
      }
      pKeywordCall++;
    }
    if (pKeywordCall->keyword == NULL)
    { /* Unknown keyword, print a debug message and discard the rest. */
      strcpy(g_TempBuffer, "Unknown keyword \"");
      strcat(g_TempBuffer, keyWord);
      strcat(g_TempBuffer, "\".\n");
      DebugPrintString(g_TempBuffer);
      if (!LevelReadAndTrimLine(keyWord, sizeof(keyWord)))
        break; /* End of file while discarding the rest of the line. */
    }
  }
  return true;
}


/* Loads the named level file, with the base name in gLevelName.  Will be
   converted to a full file name and searched for locally, on the Nabu server
   and on Alex's web site.  Returns FALSE if it couldn't find the file, or if
   the name is "Quit" or there is a fixable syntax error.  If the name is
   "Bookmark" then it will load the previously saved bookmarked level name.
   Loading sets up related things as it loads, like the game tile area size or
   background music.  Since it is a line by line keyword based file, it can
   successfully load garbage without doing anything (you'll end up playing
   the previous level again).
*/
bool LoadLevelFile(void)
{
  /* Do Quit before resetting level things (like the frame counter), so we
     have better debug info.  And a bookmark could be quit too. */

  if (strcasecmp(gLevelName, "Bookmark") == 0)
  {
    DebugPrintString("Using bookmarked level.\n");
    strcpy(gLevelName, gBookmarkedLevelName);
  }

  if (strcasecmp(gLevelName, "Quit") == 0)
  {
    DebugPrintString("Quit level.\n");
    return false;
  }

  /* Some things that need to be reset at the start of a level. */
  gVictoryWinningPlayer = MAX_PLAYERS + 2;
  sVictoryTimeoutFrame = 0;
  gLevelMaxAIPlayers = MAX_PLAYERS;

  sLevelFileHandle = OpenDataFile(gLevelName, "LEVEL");
  if (sLevelFileHandle == BAD_FILE_HANDLE)
    return false; /* OpenDataFile will have printed an error message. */
  DebugPrintString("Now loading level file \"");
  DebugPrintString(g_TempBuffer);
  DebugPrintString("\".\n");

  char levelBuffer[LEVEL_READ_BUFFER_SIZE];
  sLevelBuffer = levelBuffer; /* Save a pointer to the start of the buffer. */

  /* Reinitialise our local file buffering system to be empty. */
  sLevelReadPosition = 0;
  sLevelWritePosition = 0;

  /* Load the level! */
  bool returnCode = ProcessLevelKeywords();

  CloseDataFile(sLevelFileHandle);
  sLevelFileHandle = BAD_FILE_HANDLE;
  sLevelBuffer = NULL;

  /* Force redraw of all tiles, and in the possibly moved on-screen window.
     Lets you see the tiles, rather than whatever leftover graphics are on
     screen.  But only when in a game mode that displays the board. */
  if (gVictoryModeHighestTileCount)
  {
    ActivateTileArrayWindow();
    MakeAllTilesDirty();
  }

  /* Reset player things (like scores, position, velocity) and a few other
     things too that are level specific, now that the new level has loaded. */
  InitialisePlayersForNewLevel();

  /* Do after tiles & players set up: calc initial score. */
  InitialiseScores();

  /* Remove extra keystrokes and joystick actions from the input queue, so a
     stale joystick action doesn't take over an AI player or skip to the next
     level too early. */
#ifdef NABU_H
  while (isKeyPressed())
    getChar();
  g_KeyboardFakeJoystickStatus = 0;
  bzero(g_JoystickStatus, sizeof(g_JoystickStatus));
#endif /* NABU_H */

  return returnCode;
}


/* Returns one of several stock text messages when given a keyword.  May use
   g_TempBuffer or maybe not.  Returns your MagicWord if it doesn't know
   that magic word.  Currently recognises "Copyright" and "Version".
*/
const char *StockTextMessages(const char *MagicWord)
{
  if (strcasecmp(MagicWord, kMagicWordCopyright) == 0)
  {
    return
      "Welcome to the Nth Pong Wars NABU game.  "
      "Copyright 2025 by Alexander G. M. Smith, contact agmsmith@ncf.ca.  "
      "Started in February 2024, see the blog at "
      "https://web.ncf.ca/au829/WeekendReports/20240207/NthPongWarsBlog.html  "
      "Released under the GNU General Public License version 3.\n";
  }
  else if (strcasecmp(MagicWord, kMagicWordVersion) == 0)
  {
    uint16_t totalMem, largestMem;

    strcpy(g_TempBuffer, "Using D. J. Sures NABU-LIB, compiled with "
      "the Z88DK build environment (using the SDCC compiler).  "
      "Compiled on " __DATE__ " at " __TIME__ ".  Heap has ");
    mallinfo(&totalMem, &largestMem);
    AppendDecimalUInt16(totalMem);
    strcat (g_TempBuffer, " bytes free, largest ");
    AppendDecimalUInt16(largestMem);
    strcat(g_TempBuffer, ".\n");
    return g_TempBuffer;
  }

  return MagicWord;
}

