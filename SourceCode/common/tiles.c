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

/* Computed location of the screen in the play area, to avoid recalculating it
   every frame.  Updated by ActivateTileArrayWindow(). */
static uint8_t s_PlayScreenBottom = 0;
static uint8_t s_PlayScreenLeft = 0;
static uint8_t s_PlayScreenRight = 0;
static uint8_t s_PlayScreenTop = 0;

tile_pointer g_cache_animated_tiles[MAX_ANIMATED_CACHE];
uint8_t g_cache_animated_tiles_index = 0;

tile_pointer g_cache_dirty_screen_tiles[MAX_DIRTY_SCREEN_CACHE];
uint8_t g_cache_dirty_screen_tiles_index = 0;

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

  g_play_area_end_tile = NULL;
  g_play_area_num_tiles = g_play_area_height_tiles * g_play_area_width_tiles;
  if (g_play_area_num_tiles == 0)
    return false;
  if (g_play_area_num_tiles > TILES_ARRAY_SIZE)
    return false;
  g_play_area_end_tile = g_tile_array + g_play_area_num_tiles;

  /* Fill in all the row start pointers, NULL when past the end of actual data
     for easier past end handling.  Also don't have a partial row. */

  pTile = g_tile_array;
  numRemaining = g_play_area_num_tiles;
  row = 0;
  do {
    if (numRemaining >= g_play_area_width_tiles)
    {
      g_tile_array_row_starts[row] = pTile;
      pTile += g_play_area_width_tiles;
      numRemaining -= g_play_area_width_tiles;
    }
    else /* No more full rows of tiles. */
    {
      g_tile_array_row_starts[row] = NULL;
    }
    row++;
    /* Normally it is a full 256 entry array and row is a byte sized index which
      overflows at 255.  So normally don't need this test. */
    #if TILES_MAX_ROWS < 256
    if (row == TILES_MAX_ROWS)
      break;
    #endif
  } while (row != 0); /* Stop when row overflows back to zero. */

  /* Clear all the tiles to an empty state. */

  y = TILE_PIXEL_WIDTH / 2; /* For centers of tiles. */
  for (row = 0; row != g_play_area_height_tiles; row++, y += TILE_PIXEL_WIDTH)
  {
    pTile = g_tile_array_row_starts[row];
    if (pTile == NULL)
      return false; /* Sanity check, algorithm is wrong, fix code. */
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
      pTile->animated = true; /* First update will recalculate this. */
    }
  }

  ActivateTileArrayWindow();

  return true;
}


/* Recalculates VDP addresses for the tiles which are on screen, and sets them
   to NULL if off screen.  Uses the values in the g_screen_*_tiles globals.
   Call this after you change those globals to move the window around.  Will
   clip the globals so that they fit on the real screen.  Tries to be fast, so
   you can move the window often, but it will probably cost a missed frame.
*/
void ActivateTileArrayWindow(void)
{
#ifdef NABU_H
  uint8_t col, row;
  tile_pointer pTile;
  uint16_t vdpAddress;

  /* Sanitise the screen rectangle to be on screen, and not negative in width
     or height, though it can be zero width or height.  Assume the screen is 32
     characters wide and 24 lines tall on the NABU, since that's the graphics
     mode, and having constants avoids an expensive general multiplication. */

  const uint8_t SCREEN_WIDTH = 32;
  const uint8_t SCREEN_HEIGHT = 24;
#else /* TODO: for Curses, get the real screen size? */
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

  /* Find the location of the screen in play area coordinates.  Top left
     included, bottom right is just past the end.  Can be partially or
     completely outside the play area, clipped to be reasonable. */

  s_PlayScreenLeft = g_play_area_col_for_screen;
  if (s_PlayScreenLeft >= g_play_area_width_tiles)
    s_PlayScreenLeft = g_play_area_width_tiles;
  s_PlayScreenRight = g_play_area_col_for_screen + g_screen_width_tiles;
  if (s_PlayScreenRight > g_play_area_width_tiles)
    s_PlayScreenRight = g_play_area_width_tiles;
  if (s_PlayScreenRight < s_PlayScreenLeft) /* Can happen with byte overflow. */
    s_PlayScreenRight = s_PlayScreenLeft;

  s_PlayScreenTop = g_play_area_row_for_screen;
  if (s_PlayScreenTop >= g_play_area_height_tiles)
    s_PlayScreenTop = g_play_area_height_tiles;
  s_PlayScreenBottom = g_play_area_row_for_screen + g_screen_height_tiles;
  if (s_PlayScreenBottom > g_play_area_height_tiles)
    s_PlayScreenBottom = g_play_area_height_tiles;
  if (s_PlayScreenBottom < s_PlayScreenTop) /* Work around overflow errors. */
    s_PlayScreenBottom = s_PlayScreenTop;

#ifdef NABU_H
  /* Go through all tiles, setting their VDP addresses if on screen, clearing
     to NULL if off screen. */

  for (row = 0; row != g_play_area_height_tiles; row++)
  {
    pTile = g_tile_array_row_starts[row];
    if (row >= s_PlayScreenTop && row < s_PlayScreenBottom)
    { /* Row is at least partly on screen. */
      vdpAddress = 0;
      for (col = 0; col < g_play_area_width_tiles; col++, pTile++)
      {
        /* Watch out for special case where s_PlayScreenLeft == s_PlayScreenRight */
        if (col == s_PlayScreenRight)
          vdpAddress = 0;
        else if (col == s_PlayScreenLeft)
        {
          uint8_t screenX;
          uint8_t screenY;
          screenX = col - s_PlayScreenLeft + g_screen_top_X_tiles;
          screenY = row - s_PlayScreenTop + g_screen_top_Y_tiles;
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
      for (col = 0; col < g_play_area_width_tiles; col++, pTile++)
      {
        pTile->vdp_address = 0;
      }
    }
  }
#endif /* NABU_H */

  /* Force caches to be recomputed on the next update, and do a full screen
     update of the tiles. */

  g_cache_animated_tiles_index = 0xFF;
  g_cache_dirty_screen_tiles_index = 0xFF;
}


/* Change the owner of the tile to the given one.  Takes care of updating
   animation stuff and getting it drawn later.
*/
void SetTileOwner(tile_pointer pTile, tile_owner newOwner)
{
  if (pTile == NULL || pTile->owner == newOwner || newOwner >= OWNER_MAX)
    return; /* Nothing to do. */
  pTile->owner = newOwner;

  pTile->animationIndex = 0;
  if (!pTile->animated)
  {
    pTile->animated = true;

    /* Add the tile to the animated ones, so it gets the display data updated,
       which will cause it to be marked dirty and drawn if needed. */
    if (g_cache_animated_tiles_index < MAX_ANIMATED_CACHE)
      g_cache_animated_tiles[g_cache_animated_tiles_index++] = pTile;
    else if (g_cache_animated_tiles_index != 0xFF)
      g_cache_animated_tiles_index++;
  }
}


/* Update the animation state of a tile.  Mark as dirty if it changed and needs
   redrawing (but not network dirty, only care about tile owner changing for
   that).  Also if we see it is a one frame animation, flag it as not animated
   so it doesn't get useless future calls to this function.
*/
static void UpdateOneTileAnimation(tile_pointer pTile)
{
  uint8_t newChar;
  const char *pAnimString;
  uint8_t animIndex;

  pAnimString = g_TileAnimData[pTile->owner];
  animIndex = pTile->animationIndex;
  newChar = pAnimString[++animIndex];
  if (newChar == 0) /* End of animation string, go back to start. */
  {
    newChar = pAnimString[0];
    if (animIndex <= 1) /* Only one frame of animation, not really animated. */
      pTile->animated = false;
    animIndex = 0;
  }
  pTile->animationIndex = animIndex;

  if (pTile->displayedChar != newChar)
  {
    pTile->displayedChar = newChar;
#ifdef NABU_H
    if (!pTile->dirty_screen && pTile->vdp_address != 0)
#else
    if (!pTile->dirty_screen)
#endif
    {
      pTile->dirty_screen = true;
      if (g_cache_dirty_screen_tiles_index < MAX_DIRTY_SCREEN_CACHE)
        g_cache_dirty_screen_tiles[g_cache_dirty_screen_tiles_index++] = pTile;
      else if (g_cache_dirty_screen_tiles_index != 0xFF)
        g_cache_dirty_screen_tiles_index++;
    }
  }
}


/* Go through all the tiles and update displayedChar for the current frame,
   using the animation data for each type of tile.  If the character changes,
   mark the tile as dirty.  For speed, we just look at cached animated tiles,
   unless the cache is overflowing, then we do the full iteration over all
   tiles.
*/
void UpdateTileAnimations(void)
{
  uint8_t col, row;
  tile_pointer pTile;

  if (g_cache_animated_tiles_index > MAX_ANIMATED_CACHE)
  {
    /* Overflowing cache, do all tiles and reset the cache contents.  For more
       efficient Z80 code and fewer memory accesses, these loops run backwards,
       down to zero. */

    g_cache_animated_tiles_index = 0;
    row = g_play_area_height_tiles;
    do {
      row--;
      pTile = g_tile_array_row_starts[row];
      if (pTile == NULL)
        break;
      col = g_play_area_width_tiles;
      do {
        col--;
        UpdateOneTileAnimation(pTile);

        /* Add tile to animated tiles cache if it is animated and on screen.
           If cache is full, keep counting them, up to the maximum count. */
#ifdef NABU_H
        if (pTile->animated && pTile->vdp_address != 0)
#else
        if (pTile->animated)
#endif
        {
          if (g_cache_animated_tiles_index < MAX_ANIMATED_CACHE)
            g_cache_animated_tiles[g_cache_animated_tiles_index++] = pTile;
          else if (g_cache_animated_tiles_index != 0xFF)
            g_cache_animated_tiles_index++;
        }

        pTile++;
      } while (col != 0);
    } while (row != 0);
  }
  else /* Cache is not overflowing, just update the cached animated tiles. */
  {
    /* Update the animations for all tiles in the cache.  If some stop being
       animated and on screen, remove them from the cache. */

    uint8_t oldIndex, newIndex;
    newIndex = 0;
    for (oldIndex = 0; oldIndex < g_cache_animated_tiles_index; oldIndex++)
    {
      pTile = g_cache_animated_tiles[oldIndex];
      UpdateOneTileAnimation(pTile);
      if (pTile->animated)
      {
        if (oldIndex != newIndex) /* Not already in that position in cache. */
          g_cache_animated_tiles[newIndex] = pTile;
        newIndex++;
      }
    }
    g_cache_animated_tiles_index = newIndex;
  }
}


/* Go through all the tiles that are on-screen and update the screen as needed.
  Clear the dirty_screen flags of the ones updated.  Works for NABU and Curses
  screens.
*/
void CopyTilesToScreen(void)
{
  uint8_t cacheIndex;
  uint8_t col, row;
  tile_pointer pTile;
#ifdef NABU_H
  uint16_t vdpAddress;
#endif

  if (s_PlayScreenLeft >= g_play_area_width_tiles)
    return; /* Screen is past the right side of the play area, nothing to do. */
  if (s_PlayScreenTop >= g_play_area_height_tiles)
    return; /* Screen is below the play area, nothing to draw. */

  if (g_cache_dirty_screen_tiles_index > MAX_DIRTY_SCREEN_CACHE)
  {
    /* Cache invalid or overflowed, update all visible tiles. */

    for (row = s_PlayScreenTop; row != s_PlayScreenBottom; row++)
    {
#ifdef NABU_H
      vdpAddress = 0;
#endif
      pTile = g_tile_array_row_starts[row];
      if (pTile == NULL)
        break; /* Something went wrong, shouldn't happen. */
      pTile += s_PlayScreenLeft;
      for (col = s_PlayScreenLeft; col != s_PlayScreenRight; col++, pTile++)
      {
        if (pTile->dirty_screen)
        {
          pTile->dirty_screen = false;
#ifdef NABU_H
          if (pTile->vdp_address != vdpAddress)
          {
            vdpAddress = pTile->vdp_address;
            vdp_setWriteAddress(vdpAddress);
          }
          IO_VDPDATA = pTile->displayedChar;
          vdpAddress++;
#endif
        }
      }
    }
  }
  else /* Only update the dirty tiles in the cache list. */
  {
    for (cacheIndex = g_cache_dirty_screen_tiles_index; cacheIndex != 0; )
    {
      cacheIndex--;
      pTile = g_cache_dirty_screen_tiles[cacheIndex];
      pTile->dirty_screen = false;
#ifdef NABU_H
      vdp_setWriteAddress(pTile->vdp_address);
      IO_VDPDATA = pTile->displayedChar;
#endif
    }
  }
  g_cache_dirty_screen_tiles_index = 0;
}


static void DumpOneTileToTerminal(tile_pointer pTile)
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


/* For debugging, print all the tiles on the terminal.
*/
void DumpTilesToTerminal(void)
{
  uint8_t index;

  printf("Tile data dump...\n");

#if 0
  /* Dump all tiles in the tile array. */
  tile_pointer pTile;
  for (pTile = g_tile_array; pTile != g_play_area_end_tile; pTile++)
    DumpOneTileToTerminal(pTile);
#endif

  printf("Cached animated tiles, %d:\n", (int) g_cache_animated_tiles_index);
  if (g_cache_animated_tiles_index <= MAX_ANIMATED_CACHE)
  {
    for (index = 0; index < g_cache_animated_tiles_index; index++)
      DumpOneTileToTerminal(g_cache_animated_tiles[index]);
  }

  printf("Cached dirty tiles, %d:\n", (int) g_cache_dirty_screen_tiles_index);
  if (g_cache_dirty_screen_tiles_index <= MAX_DIRTY_SCREEN_CACHE)
  {
    for (index = 0; index < g_cache_dirty_screen_tiles_index; index++)
      DumpOneTileToTerminal(g_cache_dirty_screen_tiles[index]);
  }

  printf("Play area %dx%d tiles (%d).\n",
    g_play_area_width_tiles, g_play_area_height_tiles, g_play_area_num_tiles);
  printf("Screen area %dx%d, top at (%d,%d).\n",
    g_screen_width_tiles, g_screen_height_tiles,
    g_screen_top_X_tiles, g_screen_top_Y_tiles);
  printf("Screen location in play area (%d,%d).\n",
    g_play_area_col_for_screen, g_play_area_row_for_screen);
}

