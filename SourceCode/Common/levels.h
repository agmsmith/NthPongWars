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

/* Various things controlling how the victory condition is achieved.  Can be
   several of them at once. */

extern bool gVictoryModeFireButtonPress;
/* If a fire button press wins the game.  Next level is selected by which
   player pressed their fire button first (see gWinnerNextLevelName). */

extern bool gVictoryModeJoystickPress;
/* The next level is selected by which joystick direction or fire button is
   pressed first.  Mostly useful for trivia contests. */

extern bool gVictoryModeHighestTileCount;
/* The player with the highest tile count wins.  The game runs until the
   highest player tile count is greater or equal to the countdown value
   (g_ScoreGoal).  The countdown ticks down once per second and starts at the
   number of tiles in the game area. */

extern uint8_t gVictoryWinningPlayer;
/* Number of the winning player, or MAX_PLAYERS+1 if no player has won.  Does
   get set to MAX_PLAYERS (not a valid player) when in gVictoryModeJoystickPress
   and the fire button is used. */

extern char gLevelName[MAX_LEVEL_NAME_LENGTH];
/* Base name for the currently running level, or the next level after victory
   happens.  When opening the level file, will have ".level" appended to find
   the file, and on the NABU should be all upper case.  The actual file
   is a text file located locally, or on the server, or on Alex's web site.
   Users can make their own if they wish. */

extern char gWinnerNextLevelName[MAX_PLAYERS+1][MAX_LEVEL_NAME_LENGTH];
/* Whoever wins the level has a custom next level base file name.  Mostly
   useful for doing trivia contests.  Though for ordinary use these are all set
   to the same level name.  0 to MAX_PLAYERS-1 are for players.  In button modes
   0 is left, 1 is down, 2 is right, 3 is up, 4 is fire. */

extern char gBookmarkedLevelName [MAX_LEVEL_NAME_LENGTH];
/* A level name saved for later use.  Possibly many levels later. */


extern bool VictoryConditionTest(void);
/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed. */

extern bool LoadLevelFile(void);
/* Loads the named level file, with the base name in gLevelName.  Will be
   converted to a full file name and searched for locally, on the Nabu server
   and on Alex's web site.  Returns FALSE if it couldn't find the file, or if
   the name is "Quit".  If the name is "Bookmark" then it will load the
   previously saved bookmarked level name.  Loading sets up related things as
   it loads, like the game tile area size or background music.  Since it is a
   line by line keyword based file, it can successfully load garbage without
   doing anything (you'll end up playing the previous level again). */

#endif /* _LEVELS_H */

