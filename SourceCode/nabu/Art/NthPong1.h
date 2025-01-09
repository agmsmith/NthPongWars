/* Nth Pong Wars - Data file 1 - tiles and sprites.
 *
 * AGMS20241107 Add a header file to point out important pre-loaded values in
 * the video memory from an artwork file.
 */
#ifndef _NTHPONG1_H
#define _NTHPONG1_H 1

/* Sprites for the ball animation and special effects surrounding it. */

typedef enum SpriteAnimationsEnum {
  SPRITE_ANIM_NONE = 0, /* Nothing gets drawn, for invisible objects. */
  SPRITE_ANIM_BALL_ROLLING,
  SPRITE_ANIM_BALL_EFFECT_FAST,
  SPRITE_ANIM_MAX
} SpriteAnimationType;

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
  {SPRITE_ANIM_BALL_ROLLING, 4, 16, 3, 250, 0}, /* A ball rolling. */
  {SPRITE_ANIM_BALL_EFFECT_FAST, 16, 28, 5, 250, 0}, /* Halo circling ball. */
};
#endif /* _NTHPONG1_H */

