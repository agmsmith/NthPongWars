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

/* Normal size of the player diameter in pixels.  Currently that's the only
   diameter; it's too much work to make it variable (breaks animations). */
#define PLAYER_DIAMETER_NORMAL = 8

/* The various brains that can run a player.
*/
typedef enum brain_enum {
  BRAIN_INACTIVE = 0, /* This player is not active, don't display it. */
  BRAIN_JOYSTICK, /* Player controlled by a Human using a joystick. */
  BRAIN_NETWORK, /* Player controlled by a remote entity over the network. */
  BRAIN_ALGORITHM, /* Player controlled by software, AI or other code. */
  BRAIN_MAX
} player_brain;


/* This is the main player status record.  Where they are, what they are doing.
   Inactive players also get a record, in case they join a game in progress.
*/
typedef struct player_struct {
  __uint16_t pixel_center_x;
  __uint16_t pixel_center_y;
  /* Position of the tile center in the play area, in pixels.  Used during
     collision detection to avoid recalculating it.  Need 16 bits since the
     play area can be larger than the NABU screen, so can go over 256 in pixel
     coordinates.  Top left tile would have top left corner at pixel (0,0) and
     thus the center at (4,4) when the tiles are 8 pixels wide and tall. */

  player_brain brain;
    /* Inactive invisible player or where this player gets its moves from. */

  __uint32_t brain_info;
    /* Extra information about this brain.  Joystick number for joysticks,
       network identification for remote players, algorithm to use for AI. */

  __uint8_t colour;
  /* Custom colour picked by the player.  Can't conflict with other players
     colour choices.  On the NABU this is a number from 0 to 15, which has some
     visual duplicates. */

  __uint16_t vdp_address;
  /* Location of this tile in NABU video memory.  Points to the name table
     entry for the tile, 0 if off screen. Makes it easier to find runs of
     adjacent changed tiles so we don't have to take extra time to change the
     VDP write address (it auto-increments in hardware).  Also avoids having
     to calculate it (visible window into the virtual play area can move). */

  tile_owner owner;
  /* What kind of tile is this?  Empty, player owned, or a power-up. */

} tile_record;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
static player_record g_player_array[MAX_PLAYERS];


#endif /* _PLAYERS_H */

