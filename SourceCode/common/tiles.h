/******************************************************************************
 * Nth Pong Wars, tiles.h for tile data structures.
 *
 * The screen is broken up into tiles (usually 8 by 8 pixels), each of which
 * can be owned by one of the players (changes colour) or be empty or be a
 * power-up.  The data structures enable quick updates by keeping track of
 * which tiles have changed and clustering runs of changed tiles for faster
 * TMS9918A video processor updates and faster network updates.  The game area
 * can also vary in size (limited by memory) and a movable window into a
 * portion of the game area can be drawn on the screen (possibly moving around
 * to follow the action on a game area larger than the screen).
 *
 * AGMS20240718 - Start this header file.
 */
#ifndef _TILES_H
#define _TILES_H 1

#include "fixed_point.h"

/* The maximum number of players allowed.  On the NABU, it's limited to 4 due to
   colour palette limits and the maximum number of visible sprites (4 visible,
   more per scan line don't get drawn, and we use sprites for balls). */
#define MAX_PLAYERS 4

/* Size of the tiles in pixels.  On the NABU, character cells are 8 pixels.
   Might have to make these defines platform dependent. */
#define TILE_PIXEL_HEIGHT 8
#define TILE_PIXEL_WIDTH 8

/* The various things that a tile can be.  Empty space, or coloured to show
   the player who owns it, or a power up.
*/
typedef enum tile_owner_enum {
  OWNER_EMPTY = 0,
  OWNER_PLAYER_1,
  OWNER_PLAYER_2,
  OWNER_PLAYER_3,
  OWNER_PLAYER_4,
  OWNER_MAX  
} tile_owner;

/* This is the main tile status record.  There is one for each tile in the
   playing area, which can include off-screen tiles.
*/
typedef struct tile_struct {
  __uint16_t pixel_center_x;
  __uint16_t pixel_center_y;
  /* Position of the tile center in the play area, in pixels.  Used during
     collision detection to avoid recalculating it.  Need 16 bits since the
     play area can be larger than the NABU screen, so can go over 256 in pixel
     coordinates.  Top left tile would have top left corner at pixel (0,0) and
     thus the center at (4,4) when the tiles are 8 pixels wide and tall. */

  __uint16_t vdp_address;
  /* Location of this tile in NABU video memory.  Points to the name table
     entry for the tile, 0 if off screen. Makes it easier to find runs of
     adjacent changed tiles so we don't have to take extra time to change the
     VDP write address (it auto-increments in hardware).  Also avoids having
     to calculate it (visible window into the virtual play area can move). */

  tile_owner owner;
  /* What kind of tile is this?  Empty, player owned, or a power-up. */

} tile_record;

/* We keep a global array of tiles, rather than dynamically allocating them.
   It has to be at least as large as the play area, which can be bigger than
   the actual screen size, if we're using windowing to show a portion of the
   area on computers with lower resolution screens.  NABU screen is 32x24, so
   minimum 768 tiles. */
#define TILES_ARRAY_SIZE 2024

/* An array of tiles.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
static tile_record g_tile_array[TILES_ARRAY_SIZE];

/* Size of the current play area, in tiles.  This can be larger or smaller than
   the screen size.  Hopefully width * height <= TILES_ARRAY_SIZE, if not then
   tiles after the end of the array are ignored (bottom of the play area) and
   the game may not work properly. */
static __uint8_t g_play_area_height_tiles = 24;
static __uint8_t g_play_area_width_tiles = 32;

/* Size of the screen area we can draw tiles in, and position of the top left
   corner in the play area (it can move around to follow the balls).  Only the
   active area is considered, score and other stuff around the visible area
   isn't included. */
static __uint8_t g_screen_height_tiles = 24;
static __uint8_t g_screen_width_tiles = 32;
static __uint8_t g_screen_top_X_tiles = 0;
static __uint8_t g_screen_top_Y_tiles = 0;

#endif /* _TILES_H */

