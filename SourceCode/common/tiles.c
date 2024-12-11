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
uint16_t g_play_area_num_tiles = 0;
tile_pointer g_play_area_end_tile = NULL;

uint8_t g_screen_height_tiles = 24;
uint8_t g_screen_width_tiles = 32;
uint8_t g_screen_top_X_tiles = 0;
uint8_t g_screen_top_Y_tiles = 0;

uint8_t g_play_area_col_for_screen = 0;
uint8_t g_play_area_row_for_screen = 0;

const char *g_TileAnimData[OWNER_MAX] =
{
#ifdef NABU_H
  "\x02", /* OWNER_EMPTY */
  "\x03", /* OWNER_PLAYER_1 */
  "\x04", /* OWNER_PLAYER_2 */
  "\x05", /* OWNER_PLAYER_3 */
  "\x06", /* OWNER_PLAYER_4 */
  "NNoorrmmaall  ", /* OWNER_PUP_NORMAL */
  "Fast", /* OWNER_PUP_FAST */
  "SSSSlllloooowwww    ", /* OWNER_PUP_SLOW */
  "Stop. . . .", /* OWNER_PUP_STOP */
  "Through ", /* OWNER_PUP_RUN_THROUGH */
#else /* Curses just uses plain characters. */
  " ", /* OWNER_EMPTY */
  "1", /* OWNER_PLAYER_1 */
  "2", /* OWNER_PLAYER_2 */
  "3", /* OWNER_PLAYER_3 */
  "4", /* OWNER_PLAYER_4 */
  "NNoorrmmaall  ", /* OWNER_PUP_NORMAL */
  "Fast", /* OWNER_PUP_FAST */
  "SSSSlllloooowwww    ", /* OWNER_PUP_SLOW */
  "Stop. . . .", /* OWNER_PUP_STOP */
  "Through", /* OWNER_PUP_RUN_THROUGH */
#endif /* NABU_H */
};


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
      pTile->animationIndex = 0;
      pTile->displayedChar = 0;
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
   clip the globals so that they fit on the real screen.  Tries to be fast, so
   you can move the window often.
*/
void ActivateTileArrayWindow(void)
{
#ifdef NABU_H
  uint8_t col, row;
  uint8_t playScreenBottom;
  uint8_t playScreenLeft;
  uint8_t playScreenRight;
  uint8_t playScreenTop;
  tile_pointer pTile;
  uint16_t vdpAddress;

  /* Sanitise the screen rectangle to be on screen, and not negative in width
     or height, though it can be zero width or height.  Assume the screen is 32
     characters wide and 24 lines tall on the NABU, since that's the graphics
     mode, and having constants avoids an expensive general multiplication. */

  const uint8_t SCREEN_WIDTH = 32;
  const uint8_t SCREEN_HEIGHT = 24;
#else /* Perhaps for Curses, get the real screen size? */
  const uint8_t SCREEN_WIDTH = 32;
  const uint8_t SCREEN_HEIGHT = 24;
#endif /* NABU_H */

  if (g_screen_top_X_tiles > SCREEN_WIDTH)
    g_screen_top_X_tiles = SCREEN_WIDTH;

  if (g_screen_top_X_tiles + g_screen_width_tiles > SCREEN_WIDTH)
    g_screen_width_tiles = SCREEN_WIDTH - g_screen_top_X_tiles;

  if (g_screen_top_Y_tiles > SCREEN_HEIGHT)
    g_screen_top_Y_tiles = SCREEN_HEIGHT;

  if (g_screen_top_Y_tiles + g_screen_height_tiles > SCREEN_HEIGHT)
    g_screen_height_tiles = SCREEN_HEIGHT - g_screen_top_Y_tiles;

#ifdef NABU_H
  /* Define a rectangle in play area space, where the screen is.  Top left
     included, bottom right is just past the end. */

  playScreenLeft = g_play_area_col_for_screen;
  playScreenRight = g_play_area_col_for_screen + g_screen_width_tiles;
  playScreenTop = g_play_area_row_for_screen;
  playScreenBottom = g_play_area_row_for_screen + g_screen_height_tiles;

  /* Go through all tiles, setting their VDP addresses if on screen, clearing
     to NULL if off screen. */

  for (row = 0; row != g_play_area_height_tiles; row++)
  {
    pTile = g_tile_array_row_starts[row];
    if (row >= playScreenTop && row < playScreenBottom)
    { /* Row is partly on screen. */
      vdpAddress = 0;
      for (col = 0; col != g_play_area_width_tiles; col++, pTile++)
      {
        /* Watch out for special case where playScreenLeft == playScreenRight */
        if (col == playScreenRight)
          vdpAddress = 0;
        else if (col == playScreenLeft)
        {
          uint8_t screenX;
          uint8_t screenY;
          screenX = col - playScreenLeft + g_screen_top_X_tiles;
          screenY = row - playScreenTop + g_screen_top_Y_tiles;
          vdpAddress = _vdpPatternNameTableAddr + screenX +
            SCREEN_WIDTH * screenY;
        }
        if (pTile->vdp_address != vdpAddress)
          pTile->dirty_screen = true; /* Tile has moved on screen, redraw. */
        pTile->vdp_address = vdpAddress;
        if (vdpAddress != 0)
          vdpAddress++; /* Avoid redoing that slow address calculation. */
      }
    }
    else /* Whole row is off screen. */
    {
      for (col = 0; col != g_play_area_width_tiles; col++, pTile++)
      {
        pTile->vdp_address = 0;
      }
    }
  }
#endif /* NABU_H */
}


/* Go through all the tiles and update displayedChar for the current frame,
   using the animation data for each type of tile.  If the character changes,
   mark the tile as dirty.
*/
void UpdateTileAnimations(void)
{
  uint8_t col, row;
  tile_pointer pTile;

  /* For more efficient Z80 code and fewer memory accesses, these loops run
     backwards, down to zero. */

  row = g_play_area_height_tiles;
  do {
    row--;
    pTile = g_tile_array_row_starts[row];
    col = g_play_area_width_tiles;
    do {
      col--;
      uint8_t newChar;
      const char *pAnimString;
      uint8_t animIndex;
      pAnimString = g_TileAnimData[pTile->owner];
      animIndex = pTile->animationIndex;
      newChar = pAnimString[++animIndex];
      if (newChar == 0) /* End of animation string, go back to start. */
      {
        newChar = pAnimString[0];
        animIndex = 0;
      }
      pTile->animationIndex = animIndex;
      if (pTile->displayedChar != newChar)
      {
        pTile->displayedChar = newChar;
#ifdef NABU_H
        if (pTile->vdp_address != NULL)
          pTile->dirty_screen = true;
#endif /* NABU_H */
        pTile->dirty_remote = true;
      }
      pTile++;
    } while (col != 0);
  } while (row != 0);
}


/* Go through all the tiles that are on-screen and update the screen as needed.
  Clear the dirty_screen flags of the ones updated.  Works for NABU and Curses
  screens.
*/
void CopyTilesToScreen(void)
{
#ifdef NABU_H
  uint8_t col, row;
  uint8_t playScreenBottom;
  uint8_t playScreenLeft;
  uint8_t playScreenRight;
  uint8_t playScreenTop;
  tile_pointer pTile;
  uint16_t vdpAddress;

  /* Define a rectangle in play area space, where the screen is.  Top left
     included, bottom right is just past the end. */

  playScreenLeft = g_play_area_col_for_screen;
  playScreenRight = g_play_area_col_for_screen + g_screen_width_tiles;
  playScreenTop = g_play_area_row_for_screen;
  playScreenBottom = g_play_area_row_for_screen + g_screen_height_tiles;

  /* Go through just the visible tiles. */

  for (row = playScreenTop; row != playScreenBottom; row++)
  {
    vdpAddress = 0;
    pTile = g_tile_array_row_starts[row];
    for (col = playScreenLeft; col != playScreenRight; col++, pTile++)
    {
      if (pTile->dirty_screen)
      {
        pTile->dirty_screen = false;
        if (pTile->vdp_address != vdpAddress)
        {
          vdpAddress = pTile->vdp_address;
          vdp_setWriteAddress(vdpAddress);
        }
        IO_VDPDATA = pTile->displayedChar;
        vdpAddress++;
      }
    }
  }
#endif /* NABU_H */
}


/* For debugging, print all the tiles on the terminal.
*/
void DumpTilesToTerminal(void)
{
  tile_pointer pTile;

  printf("Tile data dump...\n");
  printf("Play area %dx%d tiles (%d).\n",
    g_play_area_width_tiles, g_play_area_height_tiles, g_play_area_num_tiles);
  printf("Screen area %dx%d, top at (%d,%d).\n",
    g_screen_width_tiles, g_screen_height_tiles,
    g_screen_top_X_tiles, g_screen_top_Y_tiles);
  printf("Screen location in play area (%d,%d).\n",
    g_play_area_col_for_screen, g_play_area_row_for_screen);

  for (pTile = g_tile_array; pTile != g_play_area_end_tile; pTile++)
  {
    printf("(%d,%d) Owner=%d Anim=%d Disp=%d DirtS=%d, DirtR=%d, VDP=%d\n",
      (int) pTile->pixel_center_x,
      (int) pTile->pixel_center_y,
      (int) pTile->owner,
      (int) pTile->animationIndex,
      (int) pTile->displayedChar,
      (int) pTile->dirty_screen,
      (int) pTile->dirty_remote,
#ifdef NABU_H
      (int) pTile->vdp_address
#else
      0
#endif /* NABU_H */
      );
  }
}

