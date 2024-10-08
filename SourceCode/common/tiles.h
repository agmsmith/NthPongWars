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

/*******************************************************************************
 * Constants, types.
 */

#define MAX_PLAYERS 4
/* The maximum number of players allowed.  On the NABU, it's limited to 4 due to
   colour palette limits and the maximum number of visible sprites (4 visible,
   more per scan line don't get drawn). */

/* Size of the tiles in pixels.  On the NABU, character cells are 8 pixels.
   Might have to make these defines platform dependent. */
#define TILE_PIXEL_HEIGHT 8
#define TILE_PIXEL_WIDTH 8

/* This is the main tile status record.  There is one for each tile in the
   playing area, which can include off-screen tiles.
*/
typedef struct tile_struct {
  __uint8_t pixel_x;
  __uint8_t pixel_y;
  /* Position of the tile top left corner on screen, in pixels.  Irrelevant for
     off-screen tiles. */

__uint8_t owner;
  /* What kind of tile is this?  0 means empty, 1 to MAX_PLAYERS means owned by
     that player, higher numbers are used for power-ups. */

} tile_record;

/* We keep a global array of tiles, rather than dynamically allocating them.  It has to be at least as large as the play area, which can be bigger than the actual screen size, if we're using windowing to show a portion of the area on computers with lower resolution screens. */ 
#define TILES_ARRAY_SIZE (24 * 32)

/*******************************************************************************
 * Global Variables
 */

static tile_record g_tile_array[TILES_ARRAY_SIZE];

#endif /* _TILES_H */

