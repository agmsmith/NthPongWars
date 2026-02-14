/******************************************************************************
 * Nth Pong Wars, soundscreen.h for doing portable sounds, music and screens.
 *
 * They're kind of related, with the need to play music while loading a full
 * screen image (which takes a few seconds on the NABU).
 *
 * AGMS20250410 - Start this header file.
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
#ifndef _SOUND_SCREEN_H
#define _SOUND_SCREEN_H 1

typedef enum sounds_enum { /* List of all sounds, in increasing priority. */
  SOUND_NULL = 0, /* The sound of silence? */
  SOUND_HARVEST, /* Sound of harvest mode collecting some speed, has noise. */
  SOUND_WALL_HIT, /* Ball colliding with a wall. */
  SOUND_TILE_HIT, /* Ball colliding with a tile. */
  SOUND_TILE_HIT_COPY, /* Internal copy for simultaneous sound, don't use. */
  SOUND_PUP_WIDER, /* Power-up that makes you wider. */
  SOUND_PUP_BASH, /* Power-up that lets you bash through tiles. */
  SOUND_PUP_SOLID, /* Power-up that makes you trail solid tiles. */
  SOUND_PUP_STOP, /* Power-up that stops motion. */
  SOUND_PUP_FLY, /* Power-up that starts you flying. */
  SOUND_PUP_NORMAL, /* Power-up that resets you to normal, losing everything. */
  SOUND_BALL_HIT, /* Ball colliding with another ball, has noise. */
  SOUND_MAX
  };
typedef uint8_t sound_type; /* Want it to be 8 bits, not 16. */

extern const uint8_t g_TileOwnerToSoundID[OWNER_MAX];
/* Given a tile owner for a power-up as the index, returns the corresponding
   sound_type (SOUND_NULL for unimplemented ones). */


/* We can open a data file to load music or screen data, using a platform
   specific file handle.  It has a special value for errors. */
#ifdef NABU_H
typedef uint8_t FileHandleType;
#define BAD_FILE_HANDLE ((FileHandleType) 0xFF)
#else
typedef int FileHandleType;
#define BAD_FILE_HANDLE ((FileHandleType) -1)
#endif /* NABU_H */


extern uint8_t g_harvest_sound_threshold;
/* Keeps track of the amount of harvesting going on in the most recent harvest
   sound, and doesn't play other harvests this big or smaller.  Decremented
   once per frame until it hits zero, to make it adaptive. */


extern void PlaySound(sound_type sound_id, player_pointer pPlayer);
/* Play a sound effect.  Given a player so we can customise sounds per player
   (usually with a different frequency / tone for each player).  Plays with
   priority based on the sound_id if the system can't play multiple sounds
   at once. */

extern FileHandleType OpenDataFile(const char *fileNameBase,
  const char *extension, int32_t *pFileSize);
/* Open a file for sequential reading, using the given file name.  If extension
   is specified (not NULL or empty), will append a period and the extension
   string to the file name.  Will look in various directories and online,
   returning the first one found.  Returns BAD_FILE_HANDLE if it wasn't found
   anywhere and prints a debug message.  If it was found, it sets the optional
   file size argument if it isn't NULL.  You should close the file when you've
   finished using it.  Uses g_TempBuffer.  For NABU, use upper case names. */

extern void CloseDataFile(FileHandleType fileHandle);
/* Undoes OpenDataFile.  Does nothing when given BAD_FILE_HANDLE. */

extern bool PlayMusic(const char *FileName);
/* Starts the given piece of external music playing.  Assumes the sound library
   is initialised and a game loop (or screen loader) will update sound ticks.
   Will look for that file in several places, and may try a platform specific
   extension if you don't specify one.  "Silence" turns off background music
   and "Default" plays the built-in music designed for when the game is running
   (quieter, not too complex).  Returns true if successful.  If it returns
   false, it leaves whatever music was playing still playing. */

extern void SoundUpdateIfNeeded(void);
/* This is best called after every lengthy operation, like opening a file.
   We could do music updates in an interrupt, but that can be a bit tricky. */

extern bool LoadScreen(const char *FileName);
/* Read a character mapped screen picture from a file with that name, and stores
   it in video memory (which means the user sees it loading).  Preferably the
   file name includes an extension, though if it doesn't we will use the file
   size to determine what kind of file it is.  We won't try possible extensions
   if the file doesn't exist.  The various kinds of file are:

   *.NSCR has text, fonts and sprites.  The original file is *.DAT created by
   ICVGM (a Windows tool), which gets assembled into a binary file.  Since
   we're running in graphics mode 2, the font needs to be triplicated for each
   third of the screen.

   *.NCHR Has just the characters making up the screen, assumes fonts are
   already loaded by a previous .NSCR load.  Thus it loads really quickly.
   Can be generated by only using the first part of a binary file created by
   assembling output from ICVGM.

   .*NFUL Read a full screen bitmap (no usable fonts so printing text doesn't
   work) into video RAM.  Usually a *.NFUL file created by the Dithertron
   online tool.  https://8bitworkshop.com/dithertron/#sys=msx

   Uses g_TempBuffer.  Returns TRUE if successful, FALSE (and prints a debug
   message) if it couldn't open the file or there isn't enough data in the
   file. */

#endif /* _SOUND_SCREEN_H */

