/******************************************************************************
 * Nth Pong Wars, levels.h for loading levels, screens and other things.
 *
 * Sets up background screen, music, AI code, screen size, victory conditions
 * all sorts of other things when given a level file.  The meta game loop will
 * then run the level with those conditions, and after it's done, load whatever
 * level the victory conditions determine as the next one.
 *
 * AGMS20251129 - Start this header file.
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
#ifndef _LEVELS_H
#define _LEVELS_H 1

#define MAX_FILE_NAME_LENGTH 64 /* Maximum for the Nabu Internet Adapter. */
#define MAX_LEVEL_NAME_LENGTH 32 /* Short names take less memory. */

extern const char kMagicWordCopyright[]; /* Contains "Copyright". */
extern const char kMagicWordVersion[]; /* Contains "Version". */

/* Various things controlling how the victory condition is achieved.  Can be
   several of them at once. */

extern bool gVictoryModeFireButtonPress;
/* If a fire button press wins the game.  Next level is selected by which
   player pressed their fire button first (see gWinnerNextLevelName). */

extern bool gVictoryModeJoystickPress;
/* The next level is selected by which joystick direction or fire button is
   pressed first.  Mostly useful for trivia contests. */

extern bool gVictoryModeHighestTileCount;
/* The player with the highest tile count wins.  If FALSE then the player
   movement won't be run and AI's should go inactive.  The game runs until the
   highest player tile count is greater or equal to the countdown value
   (g_ScoreGoal).  The countdown ticks down once per second and starts at the
   number of tiles in the game area, unless otherwise specified. */

extern bool gVictoryModeEditScores;
/* High scores of some sort or another are shown on the screen and the players
   can edit their name (left/right moves cursor, up/down changes letter, fire
   toggles between done and editing mode, screen ends when all players done.
   AI players can edit their own name too. */

extern uint8_t gVictoryWinningPlayer;
/* Number of the winning player, or MAX_PLAYERS+2 if no player has won.  Does
   get set to MAX_PLAYERS (not a valid player) when in gVictoryModeJoystickPress
   and the fire button is used, or MAX_PLAYERS+1 for a level timeout. */

extern char gLevelName[MAX_LEVEL_NAME_LENGTH];
/* Base name for the currently running level, or the next level after victory
   happens.  When opening the level file, will have ".level" appended to find
   the file, and on the NABU should be all upper case.  The actual file
   is a text file located locally, or on the server, or on Alex's web site.
   Users can make their own if they wish. */

extern char gWinnerNextLevelName[MAX_PLAYERS+2][MAX_LEVEL_NAME_LENGTH];
/* Whoever wins the level has a custom next level base file name.  Mostly
   useful for doing trivia contests.  Though for ordinary use these are all set
   to the same level name.  0 to MAX_PLAYERS-1 are for players.  In button modes
   0 is left, 1 is down, 2 is right, 3 is up, 4 is fire, 5 is for timeout. */

extern char gBookmarkedLevelName [MAX_LEVEL_NAME_LENGTH];
/* A level name saved for later use.  Possibly many levels later. */


extern bool VictoryConditionTest(void);
/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed. */

extern bool LoadLevelFile(void);
/* Loads the named level file, with the base name in gLevelName.  Will be
   converted to a full file name and searched for locally, on the Nabu server
   and on Alex's web site.  Returns FALSE if it couldn't find the file, or if
   the name is "Quit" or there is a fixable syntax error.  If the name is
   "Bookmark" then it will load the previously saved bookmarked level name.
   Loading sets up related things as it loads, like the game tile area size or
   background music.  Since it is a line by line keyword based file, it can
   successfully load garbage without doing anything (you'll end up playing
   the previous level again). */

extern const char *StockTextMessages(const char *MagicWord);
/* Returns one of several stock text messages when given a keyword.  May use
   g_TempBuffer or maybe not.  Returns your MagicWord if it doesn't know
   that magic word.  Currently recognises "Copyright" and "Version". */

/******************************************************************************
 * Simple File Buffering Routines originally used for Level Loading and later *
 * reused for loading high scores since we can't afford to use stdio.h.       *
 ******************************************************************************/

#define MAX_LEVEL_NUMERIC_ARGUMENTS 6
extern int16_t sNumericArgumentsDecoded[MAX_LEVEL_NUMERIC_ARGUMENTS];
/* To avoid duplicating code, we have a generic function for reading in comma
   separated arguments after a keyword.  That function deposits the values in
   this array. */

extern char LevelReadByte(void);
/* Read the next byte, refill the buffer if needed, returns 0 on end of file. */

void LevelUndoReadByte(void);
/* Undoes a read of one byte of the level file.  Can only call once in a row
   since the buffer filling is only guaranteed to not overwrite just the last
   read byte.  Also, don't call this if you got end of file. */

extern char LevelPeekNextByte(void);
/* Look at the next byte, but put it back in the buffer for later reading.
   This is so you can read ahead a bit to remove leading spaces etc. */

extern bool LevelSkipSpaces(void);
/* Skip spaces and tabs until a non-blank character or end of file.  Returns
   FALSE when it hits end of file, TRUE otherwise. */

extern bool LevelReadLine(char *Buffer, uint8_t BufferSize);
/* Read everything until the next end of line.  Then skip past multiple end of
   line (LF, CR) characters until the start of the next line with something on
   it.  That means it works for both LF and CRLF terminated lines.  The CR/LF
   characters are not added to the buffer.  If it runs out of buffer space, it
   fills as much as it could then skips over the rest of the line data.
   Returns true if it read some data, false if it ran into end of file
   immediately while trying to read data.  Buffer will be NUL terminated.
   BufferSize should be at least 2, max 255, zero will trash memory, 1 will
   have spurious end of file indications. */

extern bool LevelReadToStartOfNextLine(void);
/* Read to the start of the next line.  Useful for skipping over the remainder
   of a line that you don't want to process. */

extern bool LevelReadWord(char *Buffer, uint8_t BufferSize, char Delimiter);
/* Read a word from the level file.  Stops at the delimiter (it gets consumed)
   or end of line (doesn't get consumed) or NUL.  If there is more text than
   will fit in the buffer, read and skip the excess.  Fills the given Buffer
   with the text up to but not including the delimiter and adds a NUL.
   Returns FALSE if end of file was encountered before any data was read.
   BufferSize should be at least 2, max 255, zero will trash memory, 1 will
   have spurious end of file indications. */

extern bool LevelReadAndTrimLine(char *Buffer, uint8_t BufferSize);
/* Read a line and trim off leading and trailing spaces and tabs.  Returns TRUE
  if successful, FALSE at end of file. */ 

extern bool LevelReadNumericArguments(uint8_t NumberOfArguments);
/* Read some the comma separated arguments following a keyword, converting to
   binary and storing in sNumericArgumentsDecoded[].  The number of arguments
   is specified, if there are fewer then unfilled elements of the array will
   be zero.  Returns FALSE if no data at all was read (usually due to end of
   file).  Doesn't read the remainder of the line after the last number and
   comma, so you may need to purge that. */


#endif /* _LEVELS_H */

