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
  OWNER_PUP_NORMAL = 16, /* Power-up that makes you normal; resets other pups. */
  OWNER_PUP_FAST, /* Power-up that makes you faster. */
  OWNER_PUP_SLOW, /* Power-up that makes you slower. */
  OWNER_PUP_STOP, /* Power-up that makes you stop. */
  OWNER_PUP_THROUGH_4, /* Run through next 4 squares, rather than bouncing. */
  OWNER_MAX
} tile_owner;

/* This is the main tile status record.  There is one for each tile in the
   playing area, which can include off-screen tiles.
*/
typedef struct tile_struct {
  int16_t pixel_center_x;
  int16_t pixel_center_y;
  /* Position of the tile center in the play area, in pixels.  Used during
     collision detection to avoid recalculating it.  Need 16 bits since the
     play area can be larger than the NABU screen, so can go over 256 in pixel
     coordinates.  Top left tile would have top left corner at pixel (0,0) and
     thus the center at (4,4) when the tiles are 8 pixels wide and tall.
     Signed, because players can go off screen, though tiles can't, but we do
     math with both.  Not using a fixed point number here to save space. */

  tile_owner owner;
  /* What kind of tile is this?  Empty, player owned, or a power-up. */

  unsigned int dirty_screen : 1;
  /* Set to TRUE if this tile needs to be redrawn on the next screen update.
     Only relevant for tiles that are on screen, ignored elsewhere. */

  unsigned int dirty_remote : 1;
  /* Set to TRUE if this tile needs to be sent to remote players on the next
     network update.  Relevant for all tiles in the play area, which can be
     bigger than the screen. */

#ifdef NABU_H
  uint16_t vdp_address;
  /* Location of this tile in NABU video memory.  Points to the name table
     entry for the tile, 0 if off screen. Makes it easier to find runs of
     adjacent changed tiles so we don't have to take extra time to change the
     VDP write address (it auto-increments in hardware).  Also avoids having
     to calculate it (visible window into the virtual play area can move).
     Since there is at most 16KB of video RAM, the number is from 0 to 16383,
     and zero means off screen (points to font data by the way, not to
     character data, so zero will never be used for on-screen tiles). */

  uint8_t vdp_name;
  /* Which character in the font are we displaying for this tile?  Cached value
     of what is in video memory (at vdp_address), so we can tell if it needs
     updating.  Set to 0 to force an update; we never use that font entry. */
#endif /* NABU_H */

} tile_record, *tile_pointer;

/* We keep a global array of tiles, rather than dynamically allocating them.
   It has to be at least as large as the play area, which can be bigger than
   the actual screen size, if we're using windowing to show a portion of the
   area on computers with lower resolution screens.  NABU screen is 32x24, so
   minimum 768 tiles.  Even if ROWS * COLUMNS is less than TILES_ARRAY_SIZE,
   you still have a limit on the number of rows too. */
#define TILES_ARRAY_SIZE 2024
#define TILES_MAX_ROWS 255

/* An array of tiles.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
extern tile_record g_tile_array[TILES_ARRAY_SIZE];

/* We keep an array of pointers to the start of each row of tiles, to save on
   doing a slow multiplication.  Just index this array with the Y coordinate
   and you'll get the start of that row. */
extern tile_pointer g_tile_array_row_starts[TILES_MAX_ROWS];

/* Size of the current play area, in tiles.  This can be larger or smaller than
   the screen size.  Hopefully width * height <= TILES_ARRAY_SIZE, if not then
   tiles after the end of the array are ignored (bottom of the play area) and
   the game may not work properly. */
extern uint8_t g_play_area_height_tiles;
extern uint8_t g_play_area_width_tiles;

/* These are calculated from the play area size, when you call InitTileArray(),
   so you don't need to recalculate it all the time when doing tile loops. */
extern uint16_t g_play_area_num_tiles; /* Number of tiles in the play area. */
extern tile_pointer g_play_area_end_tile; /* Just past the last tile. */

/* Size of the screen area we can draw tiles in, and position of the top left
   corner in the play area (it can move around to follow the balls).  Only the
   active area is considered, score and other stuff around the visible area
   isn't included. */
extern uint8_t g_screen_height_tiles;
extern uint8_t g_screen_width_tiles;
extern uint8_t g_screen_top_X_tiles;
extern uint8_t g_screen_top_Y_tiles;
bleeble - need offset between screen and play area, int16_t.

extern bool InitTileArray(void);
/* Sets the tile array to the given size (g_screen_width_tiles by
   g_play_area_height_tiles), returns TRUE if successful.  FALSE if the size is
   too big to handle.  This involves setting up the row indices, then clearing
   each tile to be empty, and then activating the current screen window
   (see ActivateTileArrayWindow). */

extern void ActivateTileArrayWindow(void);
/* Recalculates VDP addresses for the tiles which are on screen, and sets them
   to NULL if off screen.  Uses the values in the g_screen_*_tiles globals.
   Call this after you change those globals to move the window around.  Will
   clip the globals so that they fit on the real screen. */

#endif /* _TILES_H */

