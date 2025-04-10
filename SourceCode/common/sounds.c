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

/* Play a sound effect.  Given a player so we can customise sounds per player.
   Plays with priority if the system can't play multiple sounds at once. */
void PlaySound(sound_type type, player_pointer pPlayer)
{
#ifdef NABU_H
/* First see which channel should play the sound.  If it has white noise, it
   always goes in channel 0.  Else it goes in the next unused (quiet) or lower
   priority sound channel.  Then if the channel is playing an equal or lower
   priority sound, start the new sound in that channel. */
#endif /* NABU_H */
}

