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

#define MAX_FILE_NAME_LENGTH 64 /* Max for the Nabu Internet Adapter. */
extern char *gLevelFileName[MAX_FILE_NAME_LENGTH+1];
/* File base name for the currently running level.  Will have ".level" appended
   to find the real file, and on the NABU will be all upper case.  The actual
   file is a text file located locally, or on the server, or on Alex's web
   site.  Users can make their own if they wish. */

extern bool VictoryConditionTest(void);
/* Checks the victory conditions and sets things up for loading the next level
   (depending on which player won).  Returns TRUE if the level was completed. */

extern bool LoadLevelFile(const char *FileNameBase);
/* Loads the named level file, given just the base name.  Will be converted to
   upper case, with ".LEVEL" appended.  Will look for that file locally, on the
   Nabu server and on Alex's web site.  Returns FALSE if it couldn't find the
   file, or if the name is "QUIT".  Since it is a line by line keyword based
   file, it can successfully load garbage without doing anything (you'll end
   up playing the previous level again). */

#endif /* _LEVELS_H */

