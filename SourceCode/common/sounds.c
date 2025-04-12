/******************************************************************************
 * Nth Pong Wars, sounds.c for doing portable sound effects and music.
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

#include "sounds.h"

#ifdef NABU_H
#define MAX_SOUND_CHANNELS 3 /* Just the sound effects ones, 0 to 2. */

static sound_type s_NowPlaying[MAX_SOUND_CHANNELS];
/* Keeps track of which channels are playing which sounds.  Set to SOUND_NULL
   when a channel has finished playing a sound effect. */

static const bool k_WhiteNoiseSound[SOUND_MAX] = {
  false, /* SOUND_NULL */
  true,  /* SOUND_HARVEST */
  false, /* SOUND_TILE_HIT */
  false, /* SOUND_WALL_HIT */
  true,  /* SOUND_BALL_HIT */
};
/* Identifies which sound effects use the white noise generator, which will be
   limited to one particular channel for all white noise sounds and music.  The
   choice is up to the person composing the CHIPNSFX music files. */

static void *k_SoundTrackPointers[SOUND_MAX] = {
  NthEffectsSilence,
  NthEffectsHarvest,
  NthEffectsTileHit,
  NthEffectsWallHit,
  NthEffectsBallOnBall,
};
#endif /* NABU_H */


/* Play a sound effect.  Given a player so we can customise sounds per player.
   Plays with priority if the system can't play multiple sounds at once.
*/
void PlaySound(sound_type sound_id, player_pointer pPlayer)
{
#ifdef NABU_H
/* First see which channel should play the sound.  If it has white noise, it
   always goes in channel 2 (music is written with that rule too).  Else it
   goes in the next unused (quiet) or lower priority sound channel.  Then if
   the channel is playing an equal or lower priority sound, start the new sound
   in that channel. */

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
     equal or lower priority.  Equal makes for more reuse of a channel for
     rapidly happening sounds, which sounds better. */

  if (s_NowPlaying[channel] <= sound_id)
  {
    CSFX_chan(channel, k_SoundTrackPointers[sound_id]);
    s_NowPlaying[channel] = sound_id;
  }
#endif /* NABU_H */
}

