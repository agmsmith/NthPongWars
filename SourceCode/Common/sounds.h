/******************************************************************************
 * Nth Pong Wars, sounds.h for doing portable sound effects and music.
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
#ifndef _SOUNDS_H
#define _SOUNDS_H 1

typedef enum sounds_enum { /* List of all sounds, in increasing priority. */
  SOUND_NULL = 0, /* The sound of silence? */
  SOUND_HARVEST, /* Sound of harvest mode collecting some speed, has noise. */
  SOUND_WALL_HIT, /* Ball colliding with a wall. */
  SOUND_TILE_HIT, /* Ball colliding with a tile. */
  SOUND_TILE_HIT_COPY, /* Internal copy for simultaneous sound, don't use. */
  SOUND_BALL_HIT, /* Ball colliding with another ball, has noise. */
  SOUND_MAX
  };
typedef uint8_t sound_type; /* Want it to be 8 bits, not 16. */

extern uint8_t g_harvest_sound_threshold;
/* Keeps track of the amount of harvesting going on in the most recent harvest
   sound, and doesn't play other harvests this big or smaller.  Decremented
   once per frame until it hits zero, to make it adaptive. */

extern void PlaySound(sound_type sound_id, player_pointer pPlayer);
/* Play a sound effect.  Given a player so we can customise sounds per player.
   Plays with priority if the system can't play multiple sounds at once. */

#endif /* _SOUNDS_H */

