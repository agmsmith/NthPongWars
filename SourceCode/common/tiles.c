/******************************************************************************
 * Nth Pong Wars, tiles.c for implementing tile data structures.  Definitive
 * comments are in tiles.h, rather than copied here.
 */

#include "tiles.h"

/******************************************************************************/

tile_record g_tile_array[TILES_ARRAY_SIZE];
tile_pointer g_tile_array_row_starts[TILES_MAX_ROWS];

uint8_t g_play_area_height_tiles = 24;
uint8_t g_play_area_width_tiles = 32;
uint16_t g_play_area_num_tiles;
tile_pointer g_play_area_end_tile;

uint8_t g_screen_height_tiles = 24;
uint8_t g_screen_width_tiles = 32;
uint8_t g_screen_top_X_tiles = 0;
uint8_t g_screen_top_Y_tiles = 0;

/******************************************************************************/

/* Sets the tile array to the given size (g_screen_width_tiles by
   g_play_area_height_tiles), returns TRUE if successful.  FALSE if the size is
   too big to handle.  This involves setting up the row indices, then clearing
   each tile to be empty, and then activating the current screen window
   (see ActivateTileArrayWindow).
*/
bool InitTileArray(void)
{
  uint16_t numRemaining;
  tile_pointer pTile;
  uint8_t col, row;
  int16_t x, y;

  g_play_area_num_tiles = g_play_area_height_tiles * g_screen_width_tiles;
  g_play_area_end_tile = NULL;
  if (g_play_area_height_tiles == 0 || g_play_area_width_tiles == 0)
    return false;

  if (g_play_area_num_tiles > TILES_ARRAY_SIZE)
    return false;
  g_play_area_end_tile = g_tile_array + g_play_area_num_tiles;

  /* Fill in all the row starts, NULL when past the end of actual data for
     easier debugging. */

  pTile = g_tile_array;
  numRemaining = g_play_area_num_tiles;
  for (row = 0; row != TILES_MAX_ROWS; row++)
  {
    g_tile_array_row_starts[row] = pTile;
    if (numRemaining >= g_play_area_width_tiles)
    {
      pTile += g_play_area_width_tiles;
      numRemaining -= g_play_area_width_tiles;
    }
    else
    {
      pTile = NULL;
      numRemaining = 0;
    }
  }

  /* Clear all the tiles to an empty state. */

  y = TILE_PIXEL_HEIGHT / 2; /* For centers of tiles. */
  for (row = 0; row != g_play_area_height_tiles; row++, y += TILE_PIXEL_HEIGHT)
  {
    pTile = g_tile_array_row_starts[row];
    x = TILE_PIXEL_WIDTH / 2;
    for (col = 0; col != g_play_area_width_tiles;
    col++, pTile++, x += TILE_PIXEL_WIDTH)
    {
      pTile->pixel_center_x = x;
      pTile->pixel_center_y = y;
      pTile->owner = OWNER_EMPTY;
      pTile->dirty_screen = true;
      pTile->dirty_remote = true;
    }
  }

  ActivateTileArrayWindow();
  return true;
}


/* Recalculates VDP addresses for the tiles which are on screen, and sets them
   to NULL if off screen.  Uses the values in the g_screen_*_tiles globals.
   Call this after you change those globals to move the window around.  Will
   clip the globals so that they fit on the real screen.
*/
void ActivateTileArrayWindow(void)
{
#ifdef NABU_H
  uint8_t col, row;
  uint16_t vdpAddress;

uint8_t g_play_area_height_tiles = 24;
uint8_t g_play_area_width_tiles = 32;
uint16_t g_play_area_num_tiles;
tile_pointer g_play_area_end_tile;

uint8_t g_screen_height_tiles = 24;
uint8_t g_screen_width_tiles = 32;
uint8_t g_screen_top_X_tiles = 0;
uint8_t g_screen_top_Y_tiles = 0;

  /* Sanitise the screen rectangle to be on screen, and not negative in width
     or height, though it can be zero width or height. */

  if (g_screen_top_X_tiles < 0)
    g_screen_top_X_tiles = 0;
  else if (g_screen_top_X_tiles > _vdpCursorMaxXFull) /* Usually 32 wide */
    g_screen_top_X_tiles = _vdpCursorMaxXFull;

  if (g_screen_top_X_tiles + g_screen_width_tiles > _vdpCursorMaxXFull)
    g_screen_width_tiles = _vdpCursorMaxXFull - g_screen_top_X_tiles;

  if (g_screen_top_Y_tiles < 0)
    g_screen_top_Y_tiles = 0;
  else if (g_screen_top_Y_tiles > _vdpCursorMaxYFull) /* Usually 24 tall */
    g_screen_top_Y_tiles = _vdpCursorMaxYFull;

  if (g_screen_top_Y_tiles + g_screen_height_tiles > _vdpCursorMaxYFull)
    g_screen_height_tiles = _vdpCursorMaxYFull - g_screen_top_Y_tiles;

  vdpAddress = _vdpPatternNameTableAddr + g_screen_top_X_tiles +
    _vdpCursorMaxXFull * g_screen_top_Y_tiles;
  for (row = 0; row != g_play_area_height_tiles; row++)
  {
    pTile = g_tile_array_row_starts[row];
    for (col = 0; col != g_play_area_width_tiles; col++, pTile++)
    {
      pTile-> bleeble;
    }
  }

#endif /* NABU_H */
}

