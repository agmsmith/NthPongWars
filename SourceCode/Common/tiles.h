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
#ifndef _TILES_H
#define _TILES_H 1

/* Size of the tiles in pixels.  On the NABU, character cells are 8 pixels.
   Might have to make these defines platform dependent.  Assume height is the
   same as width.  */
#define TILE_PIXEL_WIDTH 8

/* Offset from the player screen coordinates to the top left corner of the
   sprite.  Since the players are 8 pixels in diameter, centered in the sprite
   16x16 box, the offset is 8 pixels in both X and Y. */
#define PLAYER_SCREEN_TO_SPRITE_OFFSET 8

/* The various things that a tile can be.  Empty space, or coloured to show
   the player who owns it, or a power up.
*/
typedef enum tile_owner_enum {
  OWNER_EMPTY = 0,
  OWNER_PLAYER_1,
  OWNER_PLAYER_2,
  OWNER_PLAYER_3,
  OWNER_PLAYER_4,
  OWNER_PUP_NORMAL, /* Power-up that makes you normal.  Also first power up. */
  OWNER_PUP_STOP, /* Power-up that makes you stop. */
  OWNER_PUPS_WITH_DURATION, /* Power-ups that have a timeout start here. */
  OWNER_PUP_FAST = OWNER_PUPS_WITH_DURATION, /* Makes you faster. */
  OWNER_PUP_FLY, /* Run through squares, rather than bouncing. */
  OWNER_PUP_WIDER, /* Makes the player wider in effect; more tiles hit. */
  OWNER_MAX
};
typedef uint8_t tile_owner; /* Want 8 bits, not a 16 bit enum. */


/* List of names for each of the owner enums, mostly for debugging. */
extern const char * g_TileOwnerNames[OWNER_MAX];

/* Animations are done by changing the character displayed for a particular tile
   type.  A NUL terminated string lists the characters to be used in sequence,
   once per frame.  For slower animations, repeat the character.  Animations
   that are one character long save time - after being displayed, they no longer
   need updates. */
extern const char *g_TileAnimData[OWNER_MAX];

/* How many of each kind of power-up tile do we want on screen?  Checks every
   few seconds and makes a new power up if below quota.  Non-power-up quotas
   are ignored. */
extern uint8_t g_TileOwnerQuotas[OWNER_MAX];

/* Number of each kind of tile on the screen.  Some are player tiles, some are
   power up tiles, etc.  Score of each player is the number of their tiles,
   also need the count of the number of each kind of power-up to determine when
   to make a new one. */
extern uint16_t g_TileOwnerCounts[OWNER_MAX];

/* This is the main tile status record.  There is one for each tile in the
   playing area, which can include off-screen tiles if the play area is bigger
   than the screen window.
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

  uint8_t animationIndex;
  /* Which frame of animation to display.  If the "owner" has a related
     animation (an array of characters to be cycled through), this keeps track
     of them, counting up from zero to the number of frames-1, and repeating.
     The corresponding character from g_TileAnimData[owner] gets drawn. */

  uint8_t displayedChar;
  /* Which character in the font are we displaying for this tile?  Cached value
     of what is in video memory (at vdp_address) on the NABU, or character in
     the Curses display, so we can tell if it needs updating.  Set to 0 to force
     an update; we never use that font entry. */

  unsigned char dirty_screen : 1;
  /* Set to TRUE if this tile needs to be redrawn on the next screen update.
     Only relevant for tiles that are on screen, ignored elsewhere.
     Using an extra unsigned char rather than unsigned int, otherwise
     intermediate values blow up to 16 bits. */

  unsigned char dirty_remote : 1;
  /* Set to TRUE if this tile needs to be sent to remote players on the next
     network update.  Relevant for all tiles in the play area, which can be
     bigger than the screen. */

  unsigned char animated : 1;
  /* Set to TRUE if this tile has an animation and needs to be updated every
     frame.  The animation is related to the tile's owner, so if the owner
     changes, this needs updating.  Tiles with a one frame animation get this
     set to FALSE after the first update. */

  unsigned char age : 3;
  /* Age of the tile owned by the player.  0 means new, 7 is the oldest (has
     been travelled over 7 extra times).  Will affect the graphic drawn for the
     tile (gets more solid the older it is) and the thrust available from
     harvesting it. */

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
#endif /* NABU_H */

} tile_record, *tile_pointer;

/* We keep a global array of tiles, rather than dynamically allocating them.
   It has to be at least as large as the play area, which can be bigger than
   the actual screen size, if we're using windowing to show a portion of the
   area on computers with lower resolution screens.  NABU screen is 32x24, so
   minimum 768 tiles.  Even if ROWS * COLUMNS is less than TILES_ARRAY_SIZE,
   you still have a limit of TILES_MAX_ROWS on the number of rows, and 256 on
   columns since both of those get stored in bytes. */
#define TILES_ARRAY_SIZE 800
#define TILES_MAX_ROWS 256

/* An array of tiles.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a static array sized in advance to be
   as large as possible. */
extern tile_record g_tile_array[TILES_ARRAY_SIZE];

/* We keep an array of pointers to the start of each row of tiles, to save on
   doing a slow multiplication.  Just index this array with the Y coordinate
   and you'll get the start of that row, or NULL if you are past the end of
   the play area. */
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
extern uint16_t g_play_area_height_pixels;
extern uint16_t g_play_area_width_pixels;

/* Size of the screen area we can draw tiles in, Only the active area is
   considered, score and other stuff around the visible area isn't included. */
extern uint8_t g_screen_height_tiles;
extern uint8_t g_screen_width_tiles;
extern uint8_t g_screen_top_X_tiles;
extern uint8_t g_screen_top_Y_tiles;

/* Cached values updated by ActivateTileArrayWindow(), number of pixels delta
   for moving sprites around to compensate for the window not being at the
   screen top.  Generally like g_screen_top_X_tiles * TILE_PIXEL_WIDTH */
#ifdef NABU_H
extern int16_t g_sprite_window_offset_x;
extern int16_t g_sprite_window_offset_y;
#endif

/* Where the visible screen is in the play area.  It can move around to follow
   the balls if the play area is larger than the screen.  This top left corner
   in play area tiles corresponds to the top left corner of the g_screen_*
   rectangle.  Negative coordinates aren't implemented. */
extern uint8_t g_play_area_col_for_screen;
extern uint8_t g_play_area_row_for_screen;

/* A cache to keep track of which tiles are animated.  If more than the cache
  maximum are animated, the cache isn't used and we revert to scanning all tiles
  for animated ones, slowly.  These are the tiles with the animated flag set
  and are on screen, meaning their animations are more than one frame long so
  they need updating always.  If you set the cache index to 0xFF, it will force
  a full iteration over all tiles and recompute which tiles should be in the
  cache.

  After more than 30 animated tiles and nothing else being updated, you run out
  of time to update the screen within one frame.  But it's still faster than the
  full screen scan, so use double that.
*/
#define MAX_ANIMATED_CACHE 60
extern tile_pointer g_cache_animated_tiles[MAX_ANIMATED_CACHE];
extern uint8_t g_cache_animated_tiles_index; /* Next free cache entry. */

/* A cache to keep track of which tiles are dirty on screen (have the
   dirty_screen flag set).  If more than the cache maximum are dirty, the cache
   isn't used and we revert to scanning the array of all tiles, slowly, for
   dirty flags.  No particular order of tiles, though it would be nice to have
   them in increasing VDP address order to save on VDP address changes.

   In practice, a lot of the animations are slower (else too fast to see), so
   the dirty cache doesn't have to be as big as the animation cache.
*/
#define MAX_DIRTY_SCREEN_CACHE 40
extern tile_pointer g_cache_dirty_screen_tiles[MAX_DIRTY_SCREEN_CACHE];
extern uint8_t g_cache_dirty_screen_tiles_index; /* Next free cache entry. */

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
   clip the globals so that they fit on the real screen.  Tries to be fast, so
   you can move the window often, but it will probably cost a missed frame. */

extern void RequestTileRedraw(tile_pointer pTile);
/* Request redraw of a tile.  Takes care of updating animation stuff, setting
   dirty flags. */

extern tile_owner SetTileOwner(tile_pointer pTile, tile_owner newOwner);
/* Change the owner of the tile to the given one.  Takes care of updating
   animation stuff, setting dirty flags, updating score.  Returns previous
   owner. */

#define GetPlayerScore(iPlayer) (g_TileOwnerCounts[OWNER_PLAYER_1 + iPlayer])
/* A player's score is just a count of the number of tiles in their colour. */

extern void AddNextPowerUpTile(void);
/* Adds a new power-up tile for ones which are under the quota set for the
   game.  Location is in a predictable pattern on purpose.  If we are already
   at quote for all tile types, does nothing.  Recommend calling this every
   couple of seconds. */

extern void UpdateTileAnimations(void);
/* Go through all the tiles and update displayedChar for the current frame,
   using the animation data for each type of tile.  If the character changes,
   mark the tile as dirty. */

extern void CopyTilesToScreen(void);
/* Go through all the tiles that are on-screen and update the screen as needed.
  Clear the dirty_screen flags of the ones updated.  Works for NABU and Curses
  screens. */

extern void DumpTilesToTerminal(void);
/* For debugging, print all the tiles on the terminal.  For the NABU, this works
   best if you redirect output (STAT CON:=UC1:) to be over the network (rather
   than on screen) so that it doesn't interfere with the VDP graphics mode. */

#endif /* _TILES_H */

