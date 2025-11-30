/******************************************************************************
 * Nth Pong Wars, levels.c for loading levels, screens and other things.
 *
 * AGMS20241220 - Started this code file.
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

char *gLevelFileName[MAX_FILE_NAME_LENGTH+1];

/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed.
*/
bool VictoryConditionTest(void)
{
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
  char screenFileName[MAX_FILE_NAME_LENGTH+1];
  if (!LevelReadAndTrimLine(screenFileName, MAX_FILE_NAME_LENGTH+1))
    return true;
  LoadScreenNFUL(screenFileName);
  return true;
}


/* This keyword loads the background music (*.CHIPNSFX).
*/
bool KeywordBackgroundMusic(void)
{
  char musicFileName[MAX_FILE_NAME_LENGTH+1];
  if (!LevelReadAndTrimLine(musicFileName, MAX_FILE_NAME_LENGTH+1))
    return true;
  PlayMusic(musicFileName);
  return true;
}


/* This keyword specifies the base file name for the next level.
*/
bool KeywordNextLevel(void)
{
  LevelReadAndTrimLine(gLevelFileName, MAX_FILE_NAME_LENGTH+1);
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
  {"FullScreen", KeywordFullScreen},
  {"BackgroundMusic", KeywordBackgroundMusic},
  {"NextLevel", KeywordNextLevel},
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
      if (!LevelReadLine(keyWord, MAX_LEVEL_KEYWORD_LENGTH))
        break; /* End of file encountered. */
      continue; /* Continue on to the next line after the comment. */
    }

    if (!LevelReadWord(keyWord, MAX_LEVEL_KEYWORD_LENGTH, ':'))
      break; /* End of file encountered.  Job done. */

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
  }
  return true;
}


/* Loads the named level file, given just the base name.  Will be converted to
   upper case, with ".LEVEL" appended.  Will look for that file locally, on the
   Nabu server and on Alex's web site.  Returns FALSE if it couldn't find the
   file, or if the name is "QUIT".  Since it is a line by line keyword based
   file, it can successfully load garbage without doing anything (you'll end
   up playing the previous level again).
*/
bool LoadLevelFile(const char *FileNameBase)
{
  char levelBuffer[LEVEL_READ_BUFFER_SIZE];
  sLevelBuffer = levelBuffer; /* Save a pointer to the start of the buffer. */

  if (strcasecmp(FileNameBase, "quit") == 0)
    return false;

  sLevelFileHandle = OpenDataFile(FileNameBase, "LEVEL");
  if (sLevelFileHandle == BAD_FILE_HANDLE)
    return false;

  ProcessLevelKeywords();

  CloseDataFile(sLevelFileHandle);
  sLevelFileHandle = BAD_FILE_HANDLE;
  sLevelBuffer = NULL;
  return true;
}

