/* Nth Pong Wars - Data file 1 - tiles and sprites.
 *
 * AGMS20241107 Add a header file to point out important pre-loaded values in
 * the video memory from an artwork file.
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
#ifndef _NTHPONG1_H
#define _NTHPONG1_H 1

/* Sprites for the ball animation and special effects surrounding it. */

typedef enum SpriteAnimationsEnum {
  SPRITE_ANIM_NONE = 0, /* Nothing gets drawn, for invisible objects. */
  SPRITE_ANIM_BALL_ROLLING,
  SPRITE_ANIM_BALL_EFFECT_THRUST,
  SPRITE_ANIM_BALL_EFFECT_WIDER,
  SPRITE_ANIM_BALL_EFFECT_BASH,
  SPRITE_ANIM_MAX
};
typedef uint8_t SpriteAnimationType; /* Want it to be 8 bits, not 16. */

typedef struct SpriteAnimStruct {
  SpriteAnimationType type; /* Identifies the animation overall. */
  uint8_t first_name; /* Identifies the first frame of the animation. */
  uint8_t past_last_name; /* Here or past and we need to restart anim. */
  uint8_t delay; /* Number of video frames per animation frame. */
  uint8_t current_name; /* Name for the currently displaying frame. */
  uint8_t current_delay; /* Counts down from delay, to slow the frame rate. */
} SpriteAnimRecord, *SpriteAnimPointer;

/* The various animations we know of.  Data is copied to a player and used
   there, so some fields will be changing in the copy, like the current frame
   name.  Note that it uses 4 characters per sprite, so multiples of 4.
   Force it to start at the beginning with current_name 250 (update adds 4,
   result 254 will be past end, animation restarts) and delay 0.
*/
static const SpriteAnimRecord g_SpriteAnimData[SPRITE_ANIM_MAX] = {
  {SPRITE_ANIM_NONE, 0, 4, 60, 250, 0}, /* Nothing, box test pattern shown. */
  {SPRITE_ANIM_BALL_ROLLING, 4, 20, 3, 250, 0}, /* A ball rolling. */
  {SPRITE_ANIM_BALL_EFFECT_THRUST, 20, 32, 2, 250, 0}, /* Halo circling ball. */
  {SPRITE_ANIM_BALL_EFFECT_WIDER, 32, 48, 3, 250, 0}, /* Expanding rings. */
  {SPRITE_ANIM_BALL_EFFECT_BASH, 48, 68, 2, 250, 0}, /* Expanding bars. */
};

/* One more sprite frame after SPRITE_ANIM_BALL_EFFECT_THRUST for extra
   bold thrust indication.  Can hack the run-time animation data to play it
   then go back to the usual frames (which is why the frame is after all other
   animations, so that they detect that they have hit their end and restart).
   It's animated to have the next frame be the starting frame.  Note that the
   frame at 252 isn't useable, since adding 4 to it wraps around to zero and
   the end of animation detection breaks. */
#define SPRITE_ANIM_BALL_EFFECT_THRUST_BOLD_FRAME 248

#endif /* _NTHPONG1_H */

