/* Nth Pong Wars - Data file 1 - tiles and sprites.
 *
 * AGMS20241107 Add a header file to point out important values in the video
 * memory.
*/

/* Sprites for the ball animation and special effects surrounding it. */

typedef struct SpriteAnimStruct {
  uint8_t first_name;
  uint8_t past_last_name;
} SpriteAnimRecord;

enum SpriteAnimationsEnum {
  SPRITE_ANIM_BALL_ROLLING = 0,
  SPRITE_ANIM_BALL_EFFECT_FAST,
  SPRITE_ANIM_MAX
};

static SpriteAnimRecord g_SpriteAnimData[SPRITE_ANIM_MAX] = {
  {0, 3}, /* SPRITE_ANIM_BALL_ROLLING - a ball rolling. */
  {3, 6}, /* SPRITE_ANIM_BALL_EFFECT_FAST - halo circling around ball. */
};

