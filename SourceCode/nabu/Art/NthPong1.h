/* Nth Pong Wars - Data file 1 - tiles and sprites.
 *
 * AGMS20241107 Add a header file to point out important values in the video
 * memory.
*/

/* Tile patterns and colours for the tiles that vary by owner, showing the
   ones the player has taken control of.  TN is Tile Name.  Empty is [0],
   player 1 to 4 are [1] to [4], thus TN_OWNER_SET_SIZE is 5.  There are sets
   for 3 levels of staleness, which essentially have larger borders when
   fresher.
*/
#define TN_OWNER_SET_SIZE 5
#define TN_OWNER_STALE_SET 2
#define TN_OWNER_MIDDLING_SET 7
#define TN_OWNER_FRESH_SET 12

/* Sprites for the ball animation and special effects surrounding it. */

typedef struct AnimStruct {
  uint8_t first_name;
  uint8_t past_last_name;
} AnimRecord;

enum AnimationsEnum {
  ANIM_BALL_ROLLING = 0,
  ANIM_BALL_EFFECT_FAST,
  ANIM_MAX
};

static AnimRecord g_AnimData[ANIM_MAX] = {
  {0, 3}, /* ANIM_BALL_ROLLING - a ball rolling, 8 pixel diameter, centered. */
  {3, 6}, /* ANIM_BALL_EFFECT_FAST - halo circling around ball. */
};

