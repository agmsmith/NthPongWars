/******************************************************************************
 * Nth Pong Wars, players.h for player data structures.
 *
 * The players are represented as ball shaped objects (easier collision
 * detection) and on the NABU are rendered as hardware sprites.  The balls are
 * coloured to reflect the player choice (maybe with a drop-shadow), and
 * animated if a power-up is applied to the object (if several power-ups are
 * active, the animations sequence through the individual power up animations).
 *
 * As they collide with squares on the board which aren't their own colour, they
 * change ownership of the square to the player and then bounce off the square.
 * They have position (in pixels), velocity (pixels per frame) and acceleration
 * (player thrust and world gravity) and bounce off things like walls and other
 * balls.  The collisions are handled with some fancy algorithms so they can
 * move large distances and yet collide with things in the way, and also move
 * slowly at fractional pixels per frame.
 *
 * AGMS20241024 - Start this header file.
 */
#ifndef _PLAYERS_H
#define _PLAYERS_H 1

#include "fixed_point.h"

/* The maximum number of players allowed.  On the NABU, it's limited to 4 due to
   colour palette limits and the maximum number of visible sprites (4 visible,
   more per scan line don't get drawn, and we use sprites for balls). */
#define MAX_PLAYERS 4

/* Variable sized balls are problematic.  Having in-between sizes doesn't add
   much to the game, just a few ones are great for sweeping out large areas.
   But even then, we'd double the animation space needed (we have only 2K of
   VRAM for all frames), so we're sticking with fixed size players.  For nice
   detail on the NABU, we use 16x16 sprites (all sprites have to be the same)
   and just draw an 8x8 ball in the center of the sprite and animate power-up
   effects around the ball.  The player is considered to be spherical (with this
   diameter), centered in the hardware sprite box. */
#define PLAYER_PIXEL_DIAMETER_NORMAL = 8

/* The various brains that can run a player.
*/
typedef enum brain_enum {
  BRAIN_INACTIVE = 0, /* This player is not active, don't display it. */
  BRAIN_JOYSTICK, /* Player controlled by a Human using a joystick. */
  BRAIN_NETWORK, /* Player controlled by a remote entity over the network. */
  BRAIN_ALGORITHM_MIN, /* Range of Brains for players controlled by software. */
  BRAIN_IDLE_DELAY = BRAIN_ALGORITHM_MIN,
  /* This one doesn't accelerate, just drifts around, for 30 seconds, then it
     randomly picks another brain type.  It's used for disconnected players, so
     they can reconnect within the time delay without much happening.  But if
     they take too long, they get replaced by an AI or become inactive. */
  BRAIN_CONSTANT_SPEED,
  /* Tries to speed up to move at a constant speed in whatever direction it is
     already going in. */
  BRAIN_HOMING,
  /* Tries to go towards another player at a constant speed.  Other player
     choice changes sequentially every 10 seconds. */
  BRAIN_ALGORITHM_MAX,
  BRAIN_MAX = BRAIN_ALGORITHM_MAX
} player_brain;


/* This is the main player status record.  Where they are, what they are doing.
   Inactive players also get a record, in case they join a game in progress.
*/
typedef struct player_struct {
  fx pixel_center_x;
  fx pixel_center_y;
  /* Position of the center of the player in the play area, in pixels.  Used
     during collision detection to avoid recalculating it.  Need 16 bits since
     the play area can be larger than the NABU screen, so can go over 256 in
     pixel coordinates.  Signed, because players can go off screen, though
     tiles can't, but we do math with both.  The sprite's top left corner has
     to be calculated from the center. */

  player_brain brain;
    /* What controls this player, or marks it as inactive (not drawn). */

  __uint32_t brain_info;
    /* Extra information about this brain.  Joystick number for joysticks,
       network identification for remote players, algorithm stuff for AI. */

  __uint8_t main_colour;
  /* Predefined colour for this player's main graphic.  On the NABU this is a
     number from 1 to 15 in the standard TMS9918A palette, see the
     k_PLAYER_COLOURS[iPlayer].main value.  In Unix, using ncurses, this is a
     number from 1 to 4, which we'll calculate from the player index. */

#ifdef NABU_H
  __uint8_t shadow_colour;
  /* Predefined colour for this player's shadow sprite.  We're using colours
     which have a darker version of the same colour, so that can be a
     drop-shadow.  Caches the values from k_PLAYER_COLOURS[player#]. */

  __uint8_t sparkle_colour;
  /* Predefined colour for this player's sparkle sprite.  It's usually used for
     power-up animations and other such things around the ball.  Caches the
     values from k_PLAYER_COLOURS[player#]. */

  __uint16_t sprite_vdp_address;
  /* Location of this player's sprite attribute data in NABU video memory.  We
     may later use a second sprite for a drop-shadow - same as main sprite
     except a darker colour and slightly offset, can simulate height above the
     ground by changing the offset. */

  /* What value we have stored in the VDP sprite attributes for our player main
     sprite.  If we compute a different value, we need to write it to the VDP
     and update these cached values.  Order is the same order as in the
     hardware, thus Y is before X. */
  uint8_t main_sprite_vdp_y_position;
  uint8_t main_sprite_vdp_x_position;
  uint8_t main_sprite_vdp_name;
  /* The name is the index to a 8 byte pattern element in the sprite generator
     table.  16x16 sprites use 4 of those elements in a AC/BD left to right
     order.  So animations need 4 elements (32 bytes) per frame.  The players
     can reuse the same animations (one for each power up), so 256/4 = 64 total
     animation frames are available in VRAM (might be able to dynamically load
     animations only for currently active power-ups). */
  uint8_t main_sprite_vdp_flags_colour;
  /* Current colour, and a 32 pixel shift flag for sprites with -32 < X < 0 */
  
  __uint8_t sprite_vdp_name;
  /* What graphic are we showing for the sprite?  Could be an animation. */
#endif /* NABU_H */
} player_record;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
static player_record g_player_array[MAX_PLAYERS];


#ifdef NABU_H
/* Predefined colour choices for the players, used on the NABU.  The palette is
   so limited that it's not worthwhile letting the player choose.  The first
   three are fairly distinct, though the fourth redish one is easily confused
   with others, which is why it's fourth. */

typedef struct colour_triplet_struct {
  uint8_t main;
  uint8_t shadow;
  uint8_t sparkle;
} colour_triplet_record;

static const colour_triplet_record k_PLAYER_COLOURS[MAX_PLAYERS] = {
  { VDP_MED_GREEN, VDP_DARK_GREEN, VDP_LIGHT_GREEN },
  { VDP_LIGHT_BLUE, VDP_DARK_BLUE, VDP_CYAN },
  { VDP_LIGHT_YELLOW, VDP_DARK_YELLOW, VDP_GRAY },
  { VDP_MED_RED, VDP_DARK_RED, VDP_LIGHT_RED}
};

/* Assignments of sprites.  There are 32, and they are displayed in priority
   from #0 to #31, with #0 on top.  If more than 4 overlap on a scan line,
   the top 4 are displayed and the rest disappear.  So put the balls first!
*/
typedef enum sprite_assignment_enum {
  SPRITE_NUM_BALLS = 0, /* First 4 sprites are the balls for 4 players. */
  SPRITE_NUM_EFFECTS = 4, /* Four sprites for the special effects around balls. */
  SPRITE_NUM_SHADOWS = 8, /* Four sprites for the drop shadows under balls. */
} sprite_numbers;

#endif /* NABU_H */

#endif /* _PLAYERS_H */

