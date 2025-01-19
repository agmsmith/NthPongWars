/******************************************************************************
 * Nth Pong Wars, simulate.c for updating positions of moving objects.
 *
 * Given the player inputs and current state of the players and tiles, move
 * things around to their positions for the next frame.  Along the way, queue
 * up sound effects and changes to tiles.  Later the higher priority sounds
 * will get played and only changes to tiles will get sent to the display and
 * networked players.
 *
 * AGMS20241108 - Start this header file.
 */

#define DEBUG_PRINT_SIM 0 /* Turn on debug printfs, uses floating point. */

/*******************************************************************************
 * Calculate the new position and velocity of all players.
 *
 * To avoid missing collisions with tiles, we do the update in smaller steps
 * where the most a player moves in a step is a single tile width or height.
 * All the players use the same number of steps, so collisions between players
 * are done at the place in the middle of the movement where they would
 * actually collide.  Fortunately we have 16 bits of fraction, so fractional
 * step updates should be fairly accurate in adding up to the full update
 * distance.
 *
 * To check for tile collisions with a newly calculated position, we only look
 * at the tile containing the new position and the three tiles adjacent, in the
 * quadrant in the direction that the player's position is offset from the
 * center of the current tile, since they could only be close enough to collide
 * with the nearer tiles (ball width == tile width == 8 pixels).  If the player
 * is exactly in the center of a tile, we have to check it and all 8
 * surrounding tiles.  Similarly, if it is exactly at the center X but not Y,
 * we have to check two quadrants (6 tiles including the current tile).  Think
 * of a 3x3 tic-tac-toe board.
 *
 * Player collisions with players are found by testing all combinations, 4
 * players means 6 tests (1-2, 1-3, 1-4, 2-3, 2-4, 3-4).
 *
 * When a collision occurs, the player bounces off the object.  For walls and
 * tiles, it's a simple reflection of velocity.  Walls also ensure the new
 * position is not inside the wall, overriding it to be just outside the wall
 * if needed (otherwise you can get balls bouncing around inside the wall - a
 * fault in the original Pong game).  For players it's a momentum conserving
 * bounce, so possibly the hit player will end up moving faster.  The effect
 * (mostly speed changes) of the collisions in a time step are applied in the
 * order tiles, players, walls.
 */
void Simulate(void)
{
  fx absVelocity;
  fx maxVelocity;
  uint8_t iStep;
  uint8_t numberOfSteps;
  uint8_t iPlayer;
  player_pointer pPlayer;
  uint8_t stepShiftCount;

#if DEBUG_PRINT_SIM
printf("\nStarting simulation update.\n");
#endif

  /* Find the largest velocity component of all the players. */

  ZERO_FX(maxVelocity);
  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

#if DEBUG_PRINT_SIM
printf("Player %d: pos (%f, %f), vel (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y)
);
#endif

    absVelocity = pPlayer->velocity_x;
    ABS_FX(&absVelocity);
    if (COMPARE_FX(&absVelocity, &maxVelocity) > 0)
      maxVelocity = absVelocity;

    absVelocity = pPlayer->velocity_y;
    ABS_FX(&absVelocity);
    if (COMPARE_FX(&absVelocity, &maxVelocity) > 0)
      maxVelocity = absVelocity;
  }

#if DEBUG_PRINT_SIM
printf("Max velocity component: %f\n",
  GET_FX_FLOAT(maxVelocity)
);
#endif

  /* Find the number of steps needed to ensure the maximum velocity is less
     than one tile per step, or at most something like 7.999 pixels.  Can divide
     velocity by using 2**stepShiftCount to get the velocity per step. */

  for (numberOfSteps = 1, stepShiftCount = 0;
  (numberOfSteps & 0x80) == 0;
  numberOfSteps *= 2, stepShiftCount++)
  {
    /* See if the step size is less than a tile width. */
    if (GET_FX_INTEGER(maxVelocity) < TILE_PIXEL_WIDTH)
      break; /* Step size is small enough now. */
    DIV2_FX(maxVelocity, maxVelocity);
  }

#if DEBUG_PRINT_SIM
printf("Have %d steps, shift by %d\n", numberOfSteps, stepShiftCount);
#endif

  /* Calculate the step velocity for each player. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;
    pPlayer->step_velocity_x.as_int32 =
      pPlayer->velocity_x.as_int32 >> stepShiftCount;
    pPlayer->step_velocity_y.as_int32 =
      pPlayer->velocity_y.as_int32 >> stepShiftCount;
#if DEBUG_PRINT_SIM
printf("Player %d: vel (%f, %f), step vel (%f, %f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y),
  GET_FX_FLOAT(pPlayer->step_velocity_x),
  GET_FX_FLOAT(pPlayer->step_velocity_y)
);
#endif
  }

  /* Update the players, step by step.  Hopefully most times players are moving
     at less than one tile per frame so we only have one step.  Top speed we
     can simulate is 128 steps, or 128 tiles per frame, half a screen width per
     frame, which is pretty fast and likely unplayable. */

  for (iStep = 0; iStep < numberOfSteps; iStep ++)
  {
#if DEBUG_PRINT_SIM
printf("Substep %d:\n", iStep);
#endif
    /* Calculate the new position of all the players for this step; just
       add step velocity to position.  Need to do this before checking for any
       collisions since we could have player hitting player. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain == BRAIN_INACTIVE)
        continue;
      ADD_FX(&pPlayer->pixel_center_x, &pPlayer->step_velocity_x,
        &pPlayer->pixel_center_x);
      ADD_FX(&pPlayer->pixel_center_y, &pPlayer->step_velocity_y,
        &pPlayer->pixel_center_y);
#if DEBUG_PRINT_SIM
printf("Player %d: new pos (%f, %f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y)
);
#endif
    }

    /* Check for player to tile collisions.  Like a tic-tac-toe board, examine
       9 tiles around the player, unless too close to the side of the play area
       then it's less. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      uint8_t startCol, endCol, curCol, playerCol;
      uint8_t startRow, endRow, curRow, playerRow;
      int16_t playerX, playerY;
      int8_t velocityX, velocityY;
      bool bounceOffX, bounceOffY;

      playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);
      playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);
      playerCol = playerX / TILE_PIXEL_WIDTH;
      playerRow = playerY / TILE_PIXEL_WIDTH;

      /* Just need the +1/0/-1 for direction of velocity. */
      velocityX = COMPARE_FX(&pPlayer->velocity_x, &gfx_Constant_Zero);
      velocityY = COMPARE_FX(&pPlayer->velocity_y, &gfx_Constant_Zero);

#if DEBUG_PRINT_SIM
printf("Player %d (%d, %d) tile collisions:\n", iPlayer, playerCol, playerRow);
#endif
      if (playerCol > 0)
        startCol = playerCol - 1;
      else /* Player near left side of board, no tiles past edge. */
        startCol = 0;

      if (playerCol < g_play_area_width_tiles - 1)
        endCol = playerCol + 1;
      else /* Player is at right edge of board, no tiles past there to check. */
        endCol = g_play_area_width_tiles - 1;

      if (playerRow > 0)
        startRow = playerRow - 1;
      else /* Player near top side of board, no tiles past edge. */
        startRow = 0;

      if (playerRow < g_play_area_height_tiles - 1)
        endRow = playerRow + 1;
      else /* Player near bottom side of board, no tiles past edge. */
        endRow = g_play_area_height_tiles - 1;

      /* Keep track of the kind of bouncing the player will do.  Want to avoid
         bouncing an even number of times in the same direction, otherwise the
         player goes through tiles.  So just collect one bounce in each axis. */

      bounceOffX = false;
      bounceOffY = false;

      for (curRow = startRow; curRow <= endRow; curRow++)
      {
        tile_pointer pTile;
        pTile = g_tile_array_row_starts[curRow];
        if (pTile == NULL)
        {
#if DEBUG_PRINT_SIM
printf("Bug: Failed to find tile on row %d for player %d.\n",
  curRow, iPlayer);
#endif
          break; /* Shouldn't happen. */
        }

        pTile += startCol;
        for (curCol = startCol; curCol <= endCol; curCol++, pTile++)
        {
          int8_t deltaPosX, deltaPosY;
          bool velXIsTowardsTile, velYIsTowardsTile;
          tile_owner previousOwner;

          /* Find relative (delta) position of player to tile, positive X values
             if player is to the right of tile, or positive Y if player below
             tile.  Can fit in a byte since we're only a few pixels away from
             the tile.  Theoretically should use the FX fixed point positions
             and velocities, but that's too much computation for a Z80 to do,
             so we use integer pixels. */

#if 0 /* Realistic mode, hits many nearby tiles. */
#define SIM_MISS_DISTANCE ((TILE_PIXEL_WIDTH + PLAYER_PIXEL_DIAMETER_NORMAL) / 2)
#else /* Game mode, thinner streak of hit tiles left behind moving ball. */
#define SIM_MISS_DISTANCE (TILE_PIXEL_WIDTH / 2 + 1)
#endif

          deltaPosX = playerX - pTile->pixel_center_x;
          if (deltaPosX >= SIM_MISS_DISTANCE || deltaPosX <= -SIM_MISS_DISTANCE)
            continue; /* Too far away, missed. */

          deltaPosY = playerY - pTile->pixel_center_y;
          if (deltaPosY >= SIM_MISS_DISTANCE || deltaPosY <= -SIM_MISS_DISTANCE)
            continue; /* Too far away, missed. */

          previousOwner = SetTileOwner(pTile, OWNER_PLAYER_1 + iPlayer);

#if DEBUG_PRINT_SIM
printf("Player %d: Hit tile %s at (%d,%d)\n",
  iPlayer, g_TileOwnerNames[previousOwner], curCol, curRow);
#endif
          if (previousOwner == OWNER_EMPTY ||
          previousOwner == OWNER_PLAYER_1 + iPlayer)
          {
            continue; /* Don't collide, keep moving over empty or own tiles. */
          }

          /* Hit someone else's tile, bounce off it. */

#if DEBUG_PRINT_SIM
printf("Player %d: Bouncing off occupied tile (%d,%d).\n",
  iPlayer, curCol, curRow);
#endif
          /* Do the bouncing.  First find out which side of the tile was hit,
             based on the direction the player was moving.  No bounce if the
             player is moving away. */

          if (velocityX < 0)
          { /* If moving left, and to right of the tile, can be a hit. */
            velXIsTowardsTile = (deltaPosX >= 0);
          }
          else if (velocityX > 0)
          { /* If moving right, and to left of the tile, can be a hit. */
            velXIsTowardsTile = (deltaPosX < 0);
          }
          else /* Not moving much. */
          {
            velXIsTowardsTile = false;
          }

          if (velocityY < 0)
          { /* If moving up, and below the tile, can be a hit. */
            velYIsTowardsTile = (deltaPosY >= 0);
          }
          else if (velocityY > 0)
          { /* If moving down, and above the tile, can be a hit. */
            velYIsTowardsTile = (deltaPosY < 0);
          }
          else /* Not moving much. */
          {
            velYIsTowardsTile = false;
          }

          /* Which side of the tile was hit?  Look for a velocity component
             going towards the tile, and if both components are towards the
             tile then use both.  The tile side which that velocity
             runs into will be the bounce side and reflect that velocity
             component.  Do nothing if moving away in both directions. */

          if (velXIsTowardsTile)
            bounceOffX = true;

          if (velYIsTowardsTile)
            bounceOffY = true;
        }
      }

      /* Now do the bouncing based on the directions we hit. */

      if (bounceOffX)
      { /* Bounce off left or right side. */
        playNoteDelay(0, 50, 30);
        NEGATE_FX(&pPlayer->velocity_x);
        NEGATE_FX(&pPlayer->step_velocity_x);
      }

      if (bounceOffY)
      { /* Bounce off top or bottom side. */
        playNoteDelay(1, 55, 30);
        NEGATE_FX(&pPlayer->velocity_y);
        NEGATE_FX(&pPlayer->step_velocity_y);
      }
    }

    /* Bounce the players off the walls. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain == BRAIN_INACTIVE)
        continue;

      if (COMPARE_FX(&pPlayer->pixel_center_y, &g_play_area_wall_bottom_y) > 0)
      {
        if (!IS_NEGATIVE_FX(&pPlayer->velocity_y))
        {
          NEGATE_FX(&pPlayer->velocity_y);
          NEGATE_FX(&pPlayer->step_velocity_y);
        }
        pPlayer->pixel_center_y = g_play_area_wall_bottom_y;
        playNoteDelay(0, 60, 90);
      }

      if (COMPARE_FX(&pPlayer->pixel_center_x, &g_play_area_wall_left_x) < 0)
      {
        if (IS_NEGATIVE_FX(&pPlayer->velocity_x))
        {
          NEGATE_FX(&pPlayer->velocity_x);
          NEGATE_FX(&pPlayer->step_velocity_x);
        }
        pPlayer->pixel_center_x = g_play_area_wall_left_x;
        playNoteDelay(1, 61, 90);
      }

      if (COMPARE_FX(&pPlayer->pixel_center_x, &g_play_area_wall_right_x) > 0)
      {
        if (!IS_NEGATIVE_FX(&pPlayer->velocity_x))
        {
          NEGATE_FX(&pPlayer->velocity_x);
          NEGATE_FX(&pPlayer->step_velocity_x);
        }
        pPlayer->pixel_center_x = g_play_area_wall_right_x;
        playNoteDelay(1, 62, 90);
      }

      if (COMPARE_FX(&pPlayer->pixel_center_y, &g_play_area_wall_top_y) < 0)
      {
        if (IS_NEGATIVE_FX(&pPlayer->velocity_y))
        {
          NEGATE_FX(&pPlayer->velocity_y);
          NEGATE_FX(&pPlayer->step_velocity_y);
        }
        pPlayer->pixel_center_y = g_play_area_wall_top_y;
        playNoteDelay(0, 63, 90);
      }
    }
  }
}

