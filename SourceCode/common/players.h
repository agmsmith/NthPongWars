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

/* Sizes allowed of the player (ball diameter in pixels).  Having in-between
   sizes doesn't add much to the game, just a few ones are great for sweeping
   out large areas.  To keep things simple, we have 8x8 sprites, 16x16 sprites,
   and hardware double size 16x16 = 32x32 sprites.  So we have to do our
   animations in 16x16 and make a reduced size 8x8 version too.  The player is
   considered to be spherical (with this diameter), centered in the hardware
   sprite box. */
#define PLAYER_DIAMETER_NORMAL = 8
#define PLAYER_DIAMETER_BIG = 16
#define PLAYER_DIAMETER_HUGE = 32

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
     they take too long, they get replaced by an AI. */
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
  __uint16_t pixel_center_x;
  __uint16_t pixel_center_y;
  /* Position of the center of the player in the play area, in pixels.  Used
     during collision detection to avoid recalculating it.  Need 16 bits since
     the play area can be larger than the NABU screen, so can go over 256 in
     pixel coordinates.  Player size can vary, so the sprite's top left corner
     has to be calculated from this. */

  player_brain brain;
    /* What controls this player, or marks it as inactive (not drawn). */

  __uint32_t brain_info;
    /* Extra information about this brain.  Joystick number for joysticks,
       network identification for remote players, algorithm to use for AI. */

  __uint8_t colour;
  /* Custom colour picked by the player.  Can't conflict with other players
     colour choices.  On the NABU this is a number from 0 to 15, which has some
     visual duplicates. */

  __uint16_t vdp_address;
  /* Location of this sprite in NABU video memory.  Points to the name table
     entry for the sprite. */

} player_record;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
static player_record g_player_array[MAX_PLAYERS];

#endif /* _PLAYERS_H */

