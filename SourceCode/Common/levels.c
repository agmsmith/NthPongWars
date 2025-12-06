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

bool gVictoryModeFireButtonPress = true;
bool gVictoryModeJoystickPress = false;
bool gVictoryModeHighestTileCount = false;
uint8_t gVictoryWinningPlayer = MAX_PLAYERS + 1;

char gLevelName[MAX_LEVEL_NAME_LENGTH] = "TITLE";
char gWinnerNextLevelName[MAX_PLAYERS+1][MAX_LEVEL_NAME_LENGTH];
char gBookmarkedLevelName [MAX_LEVEL_NAME_LENGTH] = "TITLE";


/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed.
*/
bool VictoryConditionTest(void)
{
  uint8_t winningPlayer = MAX_PLAYERS + 1;

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

  /* If we have a winner (or MAX_PLAYERS for fire button pressed in joystick
     mode), use their next level as the one we want to load next. */

  if (winningPlayer < MAX_PLAYERS + 1)
  {
    gVictoryWinningPlayer = winningPlayer;
    strcpy(gLevelName, gWinnerNextLevelName[winningPlayer]);
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


/* Read a word from the level file.  Stops at the delimeter or NUL.  If there
   is more text than will fit in the buffer, skips the excess.  Fills the given
   Buffer with the text up to but not including the delimiter and adds a NUL.
   Returns TRUE if data was read, FALSE for end of file.
   BufferSize should be at least 2, max 255, zero will trash memory, 1 will
   have spurrious end of file indications.
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


/* This keyword specifies the base level name for the next level.  It is
   followed by a player identification, either "All" or "P0", "P1", "P2",
   "P3" or a joystick direction to identify which next level gets set.
   The player is followed by a comma and the level name is the rest of the line.
*/
bool KeywordLevelNext(void)
{
  char levelName[MAX_LEVEL_NAME_LENGTH];
  char playerName[8];
  uint8_t iPlayer = MAX_PLAYERS + 2; /* +2 means no player specified. */

  static struct LevelOptionStruct {
    const char *optionWord;
    uint8_t playerNumber;
  } sLevelNextOptions[] = {
    {"All", MAX_PLAYERS + 1},
    {"P0", 0},
    {"P1", 1},
    {"P2", 2},
    {"P3", 3},
    {"Left", 0},
    {"Down", 1},
    {"Right", 2},
    {"Up", 3},
    {"Fire", 4},
    {NULL, MAX_PLAYERS + 2}
  };

  /* Get the player number, 0 to 3 for normal players, or 4 for Fire button,
     5 == MAX_PLAYERS+1 to signify all players.  MAX_PLAYERS+2 for invalid. */

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
  if (iPlayer >= MAX_PLAYERS + 2)
    goto ErrorExit; /* No recognised option found. */

  if (!LevelReadAndTrimLine(levelName, sizeof(levelName)))
    goto ErrorExit;

  /* Copy the name to the appropriate player's NextLevel file name. */

  if (iPlayer < MAX_PLAYERS + 1)
    strcpy(gWinnerNextLevelName[iPlayer], levelName);
  else /* All players picked. */
  {
    uint8_t i;
    for (i = 0; i < MAX_PLAYERS + 1; i++)
      strcpy(gWinnerNextLevelName[i], levelName);
  }
  return true;

ErrorExit:
  DebugPrintString("LevelNext: needs P0 to P3, All, "
    "Up/Down/Left/Right/Fire then a comma & level name.\n");
  return false;
}


/* Save a named level for use later.  Possibly many levels later.
*/
bool KeywordLevelBookmark(void)
{
  LevelReadAndTrimLine(gBookmarkedLevelName, sizeof(gBookmarkedLevelName));
  return true;
}


/* Delay a number of 1/10 of a second.  Keep the music playing. */
bool KeywordDelay(void)
{
  char textOfNumber[10];
  int delayCount;

  LevelReadAndTrimLine(textOfNumber, sizeof(textOfNumber));
  delayCount = atoi(textOfNumber) * 2;

  /* Our delay loop runs 1/20 second per iteration, to keep the music going. */

  while (delayCount > 0)
  {
    if (vdpIsReady >= 3) /* 3 frames, 1/20 of second has gone by. */
    {
      vdpIsReady = 0;
      CSFX_play(); /* Update sound hardware with current tune data. */
      delayCount--;
    }
  }
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


/* Handle all level keywords.  We have a table of the word and the
   corresponding function to call. */

typedef bool (*KeywordFunctionPointer)(void);

typedef struct KeyWordCallStruct {
  const char *keyword;
  KeywordFunctionPointer function;
} *KeyWordCallPointer;

static struct KeyWordCallStruct kKeywordFunctionTable[] = {
  {"ScreenFull", KeywordFullScreen},
  {"ScreenFont", KeywordCharFontSpriteScreen},
  {"ScreenChar", KeywordCharImageScreen},
  {"Music", KeywordBackgroundMusic},
  {"LevelNext", KeywordLevelNext},
  {"LevelBookmark", KeywordLevelBookmark},
  {"Delay", KeywordDelay},
  {"GameMode", KeywordGameMode},
  {NULL, NULL}
};


/* Read the level file keyword by keyword.  Returns FALSE on abort, usually
   something wrong about the level file, like bad tile map syntax.
*/
static bool ProcessLevelKeywords(void)
{
  #define MAX_LEVEL_KEYWORD_LENGTH 32
  char keyWord[MAX_LEVEL_KEYWORD_LENGTH];

  while (true)
  {
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
  char levelBuffer[LEVEL_READ_BUFFER_SIZE];
  sLevelBuffer = levelBuffer; /* Save a pointer to the start of the buffer. */

  gVictoryWinningPlayer = MAX_PLAYERS + 1;

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

  sLevelFileHandle = OpenDataFile(gLevelName, "LEVEL");
  if (sLevelFileHandle == BAD_FILE_HANDLE)
    return false; /* OpenDataFile will have printed an error message. */
  DebugPrintString("Now loading level file \"");
  DebugPrintString(g_TempBuffer);
  DebugPrintString("\".\n");

  /* Reinitialise our local file buffering system to be empty. */
  sLevelReadPosition = 0;
  sLevelWritePosition = 0;

  /* Load the level! */
  bool returnCode = ProcessLevelKeywords();

  CloseDataFile(sLevelFileHandle);
  sLevelFileHandle = BAD_FILE_HANDLE;
  sLevelBuffer = NULL;

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

