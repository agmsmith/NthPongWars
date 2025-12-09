/******************************************************************************
 * Nth Pong Wars, soundscreen.h for doing portable sounds, music and screens.
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

#include "soundscreen.h"

#ifdef NABU_H
#define MAX_SOUND_CHANNELS 3 /* Just the sound effects ones, 0 to 2. */

static sound_type s_NowPlaying[MAX_SOUND_CHANNELS];
/* Keeps track of which channels are playing which sounds.  Set to SOUND_NULL
   when a channel has finished playing a sound effect. */

static const bool k_WhiteNoiseSound[SOUND_MAX] = {
  false, /* SOUND_NULL */
  true,  /* SOUND_HARVEST */
  false, /* SOUND_WALL_HIT */
  false, /* SOUND_TILE_HIT */
  false, /* SOUND_TILE_HIT_COPY */
  true,  /* SOUND_BALL_HIT */
};
/* Identifies which sound effects use the white noise generator, which will be
   limited to one particular channel for all white noise sounds and music.  The
   choice is up to the person composing the CHIPNSFX music files. */

static void *k_SoundTrackPointers[SOUND_MAX] = {
  NthEffectsSilence,
  NthEffectsHarvest,
  NthEffectsWallHit,
  NthEffectsTileHit,
  NthEffectsTileHitCopy, /* A copy so we can customise sound for dual plays. */
  NthEffectsBallOnBall,
};
#endif /* NABU_H */

uint8_t g_harvest_sound_threshold = 0;
/* Cut down on the number of times the harvest sound is played, using an
   adaptive scheme that keeps a running threshold going continuously.  If the
   player's harvest isn't big enough compared to other recent harvests, don't
   play a sound. */


/* Play a sound effect.  Given a player so we can customise sounds per player.
   Plays with priority if the system can't play multiple sounds at once.
*/
void PlaySound(sound_type sound_id, player_pointer pPlayer)
{
  /* Skip harvest sound if it isn't as big as recent harvests. */

  if (sound_id == SOUND_HARVEST)
  {
    if (pPlayer->thrust_harvested <= g_harvest_sound_threshold)
      return; /* Not a big enough harvest to make a noise. */

    g_harvest_sound_threshold = pPlayer->thrust_harvested;
  }

#ifdef NABU_H
/* First see which channel should play the sound.  If it has white noise, it
   always goes in channel 2 (music is written with that rule too, due to a bug
   in the music player with white noise in channel 0 not restarting).  Else it
   goes in the next unused (quiet) or lower priority sound channel.  Then if
   the channel is playing a lower priority sound, start the new sound in that
   channel. */

  uint8_t channel;

  if (k_WhiteNoiseSound[sound_id])
  {
    if (s_NowPlaying[2] != SOUND_NULL && !CSFX_busy(2))
      s_NowPlaying[2] = SOUND_NULL;
    channel = 2;
  }
  else /* Tonal sound, can go in other channels. */
  {
    if (s_NowPlaying[0] != SOUND_NULL && !CSFX_busy(0))
      s_NowPlaying[0] = SOUND_NULL;
    if (s_NowPlaying[1] != SOUND_NULL && !CSFX_busy(1))
      s_NowPlaying[1] = SOUND_NULL;

    /* Use lowest priority channel. */

    if (s_NowPlaying[0] < s_NowPlaying[1])
      channel = 0;
    else
      channel = 1;
  }

  /* Can we play the sound?  Need the channel to be playing something with
     lower priority.  Equal makes for annoying repeated sounds, use a really
     short sound if you want that instead of bumping equal priority sounds. */

  if (s_NowPlaying[channel] < sound_id)
  {
    /* Some sounds have variations made by editing the sound data, so each
       player gets a slightly different sound.  Note that we don't change the
       sound data until the channel has finished playing.  Though if it is
       playing in two channels, this scheme may fail unless a copy of the data
       is used. */

    if (sound_id == SOUND_HARVEST)
    {
      /* Change noise frequency from $10 (high enough) to $FF (low). */
      NthEffectsHarvestPlayer[0] = pPlayer->player_array_index * 48 + 50;
    }
    else if (sound_id == SOUND_TILE_HIT)
    {
      /* Use a unique version of sound data for each channel, so we can play a
         customised version in each channel.  Then hack the pitch of the
         note to reflect the player hitting the tile.  Pitch is from
         0 (low, C-0) to 107=$6B (high, B#8), 12 notes per octave. */

      uint8_t pitch = pPlayer->player_array_index * 12 + 36;
      if (channel == 0)
      {
        sound_id = SOUND_TILE_HIT_COPY;
        NthEffectsTileHitCopyNote[0] = pitch;
      }
      else
      {
        NthEffectsTileHitNote[0] = pitch;
      }
    }

    CSFX_chan(channel, k_SoundTrackPointers[sound_id]);
    s_NowPlaying[channel] = sound_id;
  }
#endif /* NABU_H */
}


/* To avoid passing arguments on the stack, which is bulky on Z80.  Set by
   OpenDataFile() and persists until the next open.  Though the provided name
   strings may have vanished by then.  Also useful for generating error
   messages about higher level errors while loading. */
static const char *sSetUpFileNameBase;
static const char *sSetUpExtension;

/* Utility function to set up the path name, returns length of the string. */
static uint8_t SetUpPathInTempBuffer(const char *prefix)
{
  strcpy(g_TempBuffer, prefix);
  strcat(g_TempBuffer, sSetUpFileNameBase);
  strcat(g_TempBuffer, ".");
  strcat(g_TempBuffer, sSetUpExtension);

#if 0
  /* Convert the whole pathname string to upper case. */

  char *pChar, letter;
  pChar = g_TempBuffer;
  while ((letter = *pChar) != 0)
  {
    *pChar++ = toupper(letter);
  }
#endif

  return strlen(g_TempBuffer);
}

/* Open a file for sequential reading, using the given file name and extension
   (the extension will usually be platform specific).  Will look in various
   directories and online, returning the first one found.  Returns
   BAD_FILE_HANDLE if it wasn't found anywhere and prints a debug message.
   If it was found, you should close it when you've finished using it.  Uses
   g_TempBuffer (on success the path name used is in the buffer).  For NABU,
   use upper case names.
*/
FileHandleType OpenDataFile(const char *fileNameBase, const char *extension)
{
  FileHandleType fileID;
#ifdef NABU_H
  uint8_t nameLen;
#endif /* NABU_H */

  sSetUpFileNameBase = fileNameBase;
  sSetUpExtension = extension;

#ifdef NABU_H
  /* First try the NTHPONG directory on the NABU Internet Adapter server.  Note
     that directory separator is a backslash since the adapter was written
     originally in Windows.  Also it needs uppercase. */

  nameLen = SetUpPathInTempBuffer("NTHPONG\\");
  if (rn_fileSize(nameLen, g_TempBuffer) > 0) /* See if the file exists. */
  {
    fileID = rn_fileOpen(nameLen, g_TempBuffer, OPEN_FILE_FLAG_READONLY,
      0xff /* Use a new file handle */);
    if (fileID != BAD_FILE_HANDLE)
      return fileID;
  }

  /* Try Alex's web server.  Hopefully the Internet Adapter will read it once
     and then cache it. */

  nameLen = SetUpPathInTempBuffer("http://www.agmsmith.ca/NTHPONG/");
  if (rn_fileSize(nameLen, g_TempBuffer) > 0) /* See if the file exists. */
  {
    fileID = rn_fileOpen(nameLen, g_TempBuffer, OPEN_FILE_FLAG_READONLY,
      0xff /* Use a new file handle */);
    if (fileID != BAD_FILE_HANDLE)
      return fileID;
  }
#endif /* NABU_H */

  SetUpPathInTempBuffer(
    "Unable to load file from store directory or Alex's web site, named \"");
  strcat(g_TempBuffer, "\".\n");
  DebugPrintString(g_TempBuffer);

  return BAD_FILE_HANDLE;
}


/* Undoes OpenDataFile.  Does nothing when given BAD_FILE_HANDLE.
*/
void CloseDataFile(FileHandleType fileHandle)
{
#ifdef NABU_H
  if (fileHandle != BAD_FILE_HANDLE)
    rn_fileHandleClose(fileHandle);
#endif /* NABU_H */
}


/* Starts the given piece of external music playing.  Assumes the sound library
   is initialised and a game loop (or screen loader) will update sound ticks.
   Will look for that file in several places, using an extension specific to
   the platform (so don't specify a file name extension).  "Silence" turns off
   background music.  Returns true if successful.  If it returns false, it
   starts playing some default built-in music.
*/
#ifdef NABU_H
#define MAX_MUSIC_BUFFER_SIZE 1500 /* Bigest ChipsNSfx song we can play, +1. */
uint8_t gLoadedMusic[MAX_MUSIC_BUFFER_SIZE];
#endif /* NABU_H */

bool PlayMusic(const char *FileName)
{
  if (strcasecmp(FileName, "Silence") == 0)
  {
#ifdef NABU_H
    CSFX_stop();
#endif /* NABU_H */
    return true;
  }

  bool returnCode = false;

#ifdef NABU_H
  uint16_t amountRead;
  uint8_t fileID;

  fileID = OpenDataFile(FileName, "CHIPNSFX");
  if (fileID == BAD_FILE_HANDLE)
    goto ErrorExit;

  bzero(gLoadedMusic, MAX_MUSIC_BUFFER_SIZE); /* For easier debugging. */
  amountRead = rn_fileHandleReadSeq(fileID, gLoadedMusic,
    0 /* buffer offset */, MAX_MUSIC_BUFFER_SIZE);
  CloseDataFile(fileID);
  if (amountRead == 0 || amountRead >= MAX_MUSIC_BUFFER_SIZE)
    goto ErrorExit;

  CSFX_start(gLoadedMusic, false /* IsEffects */); /* Background music. */
  returnCode = true;

ErrorExit:
  if (!returnCode) /* Play some default music. */
    CSFX_start(NthMusic_a_z, false /* IsEffects */); /* Background music. */
#endif /* NABU_H */

  return returnCode;
}


#ifdef NABU_H
/* This is best called after every lengthy operation, like opening a file.
   We could do music updates in an interrupt, but that can be a bit tricky.
*/
static void SoundUpdateIfNeeded(void)
{
  if (vdpIsReady) /* A frame time has gone by. */
  {
    vdpIsReady = 0;
    CSFX_play(); /* Update sound hardware with current tune data. */
  }
}
#endif /* NABU_H */


#ifdef NABU_H
/* Utility function to read AmountToCopy bytes from a file and write to the
   given VRAM address.  If Triplicate is TRUE then data is also written to
   VRAM address + 2K and VRAM address + 4K.  Uses g_TempBuffer, which can be
   smaller than the AmountToCopy.  Updates sound tick as needed so you can have
   music playing during loading (assuming the frame interrupt is working).
   Returns FALSE if there wasn't enough data in the file.  TRUE if it succeeded.
*/
static bool CopyFileToVRAM(uint16_t AmountToCopy, FileHandleType FileID,
  uint16_t VRAMAddress, bool Triplicate)
{
  uint16_t amountRemaining = AmountToCopy;
  uint16_t currentVRAM = VRAMAddress;

  while (amountRemaining != 0)
  {
    uint16_t amountToRead = amountRemaining;
    uint16_t amountRead;
    if (amountToRead > TEMPBUFFER_LEN)
      amountToRead = TEMPBUFFER_LEN;
    amountRead = rn_fileHandleReadSeq(FileID, g_TempBuffer,
      0 /* buffer offset */, amountToRead);
    SoundUpdateIfNeeded();
    if (amountRead == 0) /* End of file or a read error. */
      break;

    const char *bufferPntr = g_TempBuffer;
    uint16_t amountToWrite = amountRead;
    vdp_setWriteAddress(currentVRAM);
    while (amountToWrite-- != 0)
      IO_VDPDATA = *bufferPntr++;
    SoundUpdateIfNeeded();

    if (Triplicate)
    {
      bufferPntr = g_TempBuffer;
      amountToWrite = amountRead;
      vdp_setWriteAddress(currentVRAM + 2048);
      while (amountToWrite-- != 0)
        IO_VDPDATA = *bufferPntr++;
      SoundUpdateIfNeeded();

      bufferPntr = g_TempBuffer;
      amountToWrite = amountRead;
      vdp_setWriteAddress(currentVRAM + 4096);
      while (amountToWrite-- != 0)
        IO_VDPDATA = *bufferPntr++;
      SoundUpdateIfNeeded();
    }
    currentVRAM += amountRead;
    amountRemaining -= amountRead;
  }
  return (amountRemaining == 0); /* Success only if we read it all. */
}
#endif /* NABU_H */


#define RESET_SCREEN_TO_BLACK_AND_WHITE 1
#if RESET_SCREEN_TO_BLACK_AND_WHITE && defined(NABU_H)
/* Reset the colour table to just black and white, so that you can more clearly
   see the pattern data of the next screen while it is loading.  Though this
   does make the loading slower, so comment it out if needed.
*/
static void SetColoursToBlackAndWhite(void)
{
  vdp_setWriteAddress(_vdpColorTableAddr);
  {
    uint8_t i = 24; /* Want a total of 24 * 256 bytes = 6K. */
    do {
      uint8_t j = 0;
      do {
        IO_VDPDATA = 0xF1; /* White on pixels, black off pixels. */
      } while (++j != 0);
    } while (--i != 0);
  }
  SoundUpdateIfNeeded();
}
#endif


static void PrintMissingDataError(void)
{
  SetUpPathInTempBuffer("File is too small, named \"");
  strcat(g_TempBuffer, "\".\n");
  DebugPrintString(g_TempBuffer);
}


#ifdef NABU_H
/* Read a character mapped screen picture, with fonts and sprites, into video
   RAM.  The original file is *.DAT created by ICVGM (a Windows tool), which
   gets assembled into a binary file.  Since we're running in graphics mode 2,
   the font needs to be triplicated for each third of the screen.  Uses
   g_TempBuffer.  Returns true if successful, false (and prints a debug message)
   if it couldn't open the file or there isn't enough data in the file.
*/
bool LoadScreenNSCR(const char *FileName)
{
  uint8_t fileID = BAD_FILE_HANDLE;
  bool returnCode = false;

  fileID = OpenDataFile(FileName, "NSCR");
  SoundUpdateIfNeeded();
  if (fileID == BAD_FILE_HANDLE)
    goto ErrorExit;

#if RESET_SCREEN_TO_BLACK_AND_WHITE
  SetColoursToBlackAndWhite();
#endif

  /* Load name table. */
  if (!CopyFileToVRAM(768, fileID, _vdpPatternNameTableAddr, false))
    goto ErrorMissingData;

  /* Load tile patterns. */
  if (!CopyFileToVRAM(2048, fileID, _vdpPatternGeneratorTableAddr, true))
    goto ErrorMissingData;

  /* Load tile colours. */
  if (!CopyFileToVRAM(2048, fileID, _vdpColorTableAddr, true))
    goto ErrorMissingData;

  /* Load sprite patterns. */
  if (!CopyFileToVRAM(2048, fileID, _vdpSpriteGeneratorTableAddr, false))
    goto ErrorMissingData;

  returnCode = true;
  goto ErrorExit;

ErrorMissingData:
  PrintMissingDataError();

ErrorExit:
  CloseDataFile(fileID);
  return returnCode;
}
#endif /* NABU_H */


#ifdef NABU_H
/* Read just the characters making up the screen, assumes fonts are already
   loaded.  Can be generated by only using the first part of a *.DAT created
   by ICVGM (a Windows tool), assembled into a binary file.  Uses g_TempBuffer.
   Returns true if successful, false (and prints a debug message) if it
   couldn't open the file or there isn't enough data in the file.
*/
bool LoadScreenNCHR(const char *FileName)
{
  uint8_t fileID = BAD_FILE_HANDLE;
  bool returnCode = false;

  fileID = OpenDataFile(FileName, "NCHR");
  SoundUpdateIfNeeded();
  if (fileID == BAD_FILE_HANDLE)
    goto ErrorExit;

  /* Load name table. */
  if (!CopyFileToVRAM(768, fileID, _vdpPatternNameTableAddr, false))
    goto ErrorMissingData;

  returnCode = true;
  goto ErrorExit;

ErrorMissingData:
  PrintMissingDataError();

ErrorExit:
  CloseDataFile(fileID);
  return returnCode;
}
#endif /* NABU_H */


#ifdef NABU_H
/* Read a full screen bitmap (no usable fonts so printing text doesn't work)
   into video RAM.  Usually a *.NFUL file created by Dithertron.  Uses
   g_TempBuffer.  Returns true if successful, false (and prints a debug message)
   if it couldn't open the file or there isn't enough data in the file.
*/
bool LoadScreenNFUL(const char *FileName)
{
  uint8_t fileID = BAD_FILE_HANDLE;
  bool returnCode = false;

  fileID = OpenDataFile(FileName, "NFUL");
  SoundUpdateIfNeeded();
  if (fileID == BAD_FILE_HANDLE)
    goto ErrorExit;

#if RESET_SCREEN_TO_BLACK_AND_WHITE
  SetColoursToBlackAndWhite();
#endif

  /* Reset the name table to the identity function. */

  vdp_setWriteAddress(_vdpPatternNameTableAddr);
  { /* Put in a block so i and j are temporary variables. */
    uint8_t i = 3;
    do {
      uint8_t j = 0;
      do {
        IO_VDPDATA = j++;
      } while (j != 0);
    } while (--i != 0);
  }

  /* Load tile patterns. */
  if (!CopyFileToVRAM(6144, fileID, _vdpPatternGeneratorTableAddr, false))
    goto ErrorMissingData;

  /* Load tile colours. */
  if (!CopyFileToVRAM(6144, fileID, _vdpColorTableAddr, false))
    goto ErrorMissingData;

  returnCode = true;
  goto ErrorExit;

ErrorMissingData:
  PrintMissingDataError();

ErrorExit:
  CloseDataFile(fileID);
  return returnCode;
}
#endif /* NABU_H */

