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

#define DEBUG_PRINT_SIM 0 /* Turn on debug output. */

#include "soundscreen.h"


/*******************************************************************************
 * Calculate the new position and velocity of all players.
 *
 * To avoid missing collisions with tiles, we do the update in smaller steps
 * where the most a player moves in a step is a single tile width or height,
 * or maybe even half a tile so we know which side of the tile a player hits.
 * All the players use the same number of steps, so collisions between players
 * are done at the place in the middle of the movement where they would
 * actually collide.  Fortunately we have 16 bits of fraction, so fractional
 * step updates should be fairly accurate in adding up to the full update
 * distance.
 *
 * To check for tile collisions with a newly calculated position, we look at
 * the tile containing the new position and the eight tiles adjacent.  Think
 * of a 3x3 tic-tac-toe board.
 *
 * Player collisions with players are found by testing all combinations, 4
 * players means 6 tests (1-2, 1-3, 1-4, 2-3, 2-4, 3-4).
 *
 * When a collision occurs, the player bounces off the object.  For walls and
 * tiles, it's a simple reflection of velocity.  Walls also ensure the new
 * position is not inside the wall, overriding it to be just outside the wall
 * if needed (otherwise you can get balls bouncing around inside the wall - a
 * fault in the original Pong game).  The same forcing of position outside a
 * tile is used for tile collisions, otherwise players can force their way
 * through a tile with sufficient acceleration.  For players it's a momentum
 * conserving bounce, so possibly the hit player will end up moving faster.
 * Well not quite, some separation acceleration is applied too, otherwise the
 * players keep on hitting each other.  The effect (mostly speed changes) of
 * the collisions in a time step are applied in the order tiles, players, walls.
 *
 * To save on compute, 16 bit integer coordinates (whole pixels) are used for
 * collision detection, with only the more precise fixed point numbers for
 * position and velocity calculations.
 */
void Simulate(void)
{
  int16_t maxVelocity;
  uint8_t iStep;
  uint8_t numberOfSteps;
  uint8_t iPlayer;
  player_pointer pPlayer;
  uint8_t stepShiftCount;
  static uint8_t s_PreviousStepShiftCount = 0; /* To tell when it changes. */

#if DEBUG_PRINT_SIM
  DebugPrintString("\nStarting simulation update.\n");
#endif

  /* Reduce adaptive harvest sound effect amount of harvest limit once per
     frame.  New bigger harvests will bump it back up and make noise. */

  if (g_harvest_sound_threshold != 0)
    g_harvest_sound_threshold--;

  /* Find the largest velocity component (X or Y) of all the players,
     approximate it with only looking at integer portion for speed, ignoring
     the fractional part.  Also reset thrust harvested, which gets accumulated
     during collisions with tiles and gets used to increase velocity on the
     next update.  And do other per frame updates of the player (should these
     be in player.c or here?).  Also updates players's Manhattan Speed value
     (since we're finding absolute values of the velocity here anyway), which
     lags by a frame since it's determined before moving the player. */

  maxVelocity = 0; /* In quarter pixels per frame, for extra precision. */
  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    /* Various start of frame initialisations and updates for all players. */

    pPlayer->thrust_harvested = 0;

    if (pPlayer->player_collision_count)
      pPlayer->player_collision_count--;

    /* Decrement all active power-up timers.  But only every 4th frame (so 5hz)
       so we can get several seconds of power-up in a byte counter. */

    if (((uint8_t) g_FrameCounter & 3) == iPlayer)
    {
      uint8_t *pPowerUpTimer;
      uint8_t iPowerUp;
      pPowerUpTimer = pPlayer->power_up_timers + OWNER_PUPS_WITH_DURATION;
      for (iPowerUp = OWNER_PUPS_WITH_DURATION; iPowerUp < OWNER_MAX;
      iPowerUp++, pPowerUpTimer++)
      {
        uint8_t clock = *pPowerUpTimer;
        if (clock != 0)
          *pPowerUpTimer = clock - 1;
      }

      /* Update the OWNER_PUP_FLY power up, just once every 4th frame.  Raise
         the player's flying height (moves the shadow further away) by one pixel
         until it reaches a maximum.  If the power-up is off, lower the player's
         height until it reaches a minimum.  Player also changes colour
         depending on the height (changing the shadow from black to a dim
         player colour was just confusing). */

      if (pPlayer->power_up_timers[OWNER_PUP_FLY])
      {
        if (pPlayer->pixel_flying_height < MAX_FLYING_HEIGHT)
        {
          pPlayer->pixel_flying_height++;
#ifdef NABU_H
          if (pPlayer->pixel_flying_height == FLYING_ABOVE_TILES_HEIGHT)
            pPlayer->main_colour = k_PLAYER_COLOURS[iPlayer].sparkle;
#endif /* NABU_H */
        }
      }
      else
      {
        if (pPlayer->pixel_flying_height > MIN_FLYING_HEIGHT)
        {
          pPlayer->pixel_flying_height--;
#ifdef NABU_H
          if (pPlayer->pixel_flying_height == FLYING_ABOVE_TILES_HEIGHT - 1)
            pPlayer->main_colour = k_PLAYER_COLOURS[iPlayer].main;
#endif /* NABU_H */
        }
      }
    }

#if DEBUG_PRINT_SIM
    strcpy(g_TempBuffer, "Player #");
    AppendDecimalUInt16(iPlayer);
    strcat(g_TempBuffer, ": pos (");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->pixel_center_x));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->pixel_center_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->pixel_center_y));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->pixel_center_y));
    strcat(g_TempBuffer, "), vel (");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->velocity_x));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->velocity_y));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_y));
    strcat(g_TempBuffer, ").\n");
    DebugPrintString(g_TempBuffer);
#endif

    int16_t tempPlayerSpeed;

    if (pPlayer->power_up_timers[OWNER_PUP_STOP])
    {
      pPlayer->power_up_timers[OWNER_PUP_STOP] = 0; /* No duration needed. */
      ZERO_FX(pPlayer->velocity_x);
      ZERO_FX(pPlayer->velocity_y);
      tempPlayerSpeed = 0;
    }
    else /* Find the player's Manhattan velocity, multiplied 4 precision. */
    {
      int16_t absVelocity;

      absVelocity = MUL4INT_FX(&pPlayer->velocity_x);
      if (absVelocity < 0)
        absVelocity = -absVelocity;
      tempPlayerSpeed = absVelocity;
      if (absVelocity > maxVelocity)
        maxVelocity = absVelocity;

      absVelocity = MUL4INT_FX(&pPlayer->velocity_y);
      if (absVelocity < 0)
        absVelocity = -absVelocity;
      tempPlayerSpeed += absVelocity;
      if (absVelocity > maxVelocity)
        maxVelocity = absVelocity;
    }

    /* Save the Manhattan speed, into an 8 bit integer, which should be good
       enough since it is integer pixels per frame, and 255 would be moving a
       quarter the width of the screen every frame, way too fast to care about.
       We use four times the pixel speed so we can see movements of 1/4 of a
       pixel per update.  Typically speeds are under 8 pixels per frame (32
       quarter pixels), otherwise the game will slow down dramatically due to
       extra physics steps. */

    if (tempPlayerSpeed < 0)
      pPlayer->speed = 0; /* Shouldn't happen unless overflowed 16 bit speed! */
    else if (tempPlayerSpeed >= 256)
      pPlayer->speed = 255; /* Clamp at maximum unsigned 8 bit value. */
    else
      pPlayer->speed = tempPlayerSpeed;
  }

#if DEBUG_PRINT_SIM
  strcpy(g_TempBuffer, "Max velocity component: ");
  AppendDecimalUInt16(maxVelocity);
  strcat(g_TempBuffer, " (quarter pixels per update)\n");
  DebugPrintString(g_TempBuffer);
#endif

  /* Find the number of steps needed to ensure the maximum velocity is less
     than one tile per step, or at most something like 7.999 pixels.  Can divide
     velocity by using 2**stepShiftCount to get the velocity per step.  Note
     that the integer maxVelocity < TILE_PIXEL_WIDTH is true if the floating
     point version (which can be slightly larger) is also < TILE_PIXEL_WIDTH. */

  for (numberOfSteps = 1, stepShiftCount = 0;
  (numberOfSteps & 0x80) == 0;
  numberOfSteps += numberOfSteps, stepShiftCount++)
  {
    /* See if the step size is less than half a tile width, or whatever the
       level file says is an acceptable simulation resolution. */
    if (maxVelocity < g_PhysicsStepSizeLimit)
      break; /* Step size is small enough now. */
    maxVelocity >>= 1;
  }

#if DEBUG_PRINT_SIM
  strcpy(g_TempBuffer, "Have ");
  AppendDecimalUInt16(numberOfSteps);
  strcat(g_TempBuffer, " steps, shift by ");
  AppendDecimalUInt16(stepShiftCount);
  strcat(g_TempBuffer, ".\n");
  DebugPrintString(g_TempBuffer);
#endif

  /* Update our cached constant velocity change for collisions. */

  if (s_PreviousStepShiftCount != stepShiftCount)
  {
    COPY_FX(&g_SeparationVelocityFxAdd, &g_SeparationVelocityFxStepAdd);
    if (stepShiftCount != 0)
      DIV2Nth_FX(&g_SeparationVelocityFxStepAdd, stepShiftCount);
    s_PreviousStepShiftCount = stepShiftCount;
  }

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
    strcpy(g_TempBuffer, "Player #");
    AppendDecimalUInt16(iPlayer);
    strcat(g_TempBuffer, ": vel (");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->velocity_x));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->velocity_y));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_y));
    strcat(g_TempBuffer, "), step vel (");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->step_velocity_x));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->step_velocity_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalInt16(GET_FX_INTEGER(pPlayer->step_velocity_y));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->step_velocity_y));
    strcat(g_TempBuffer, ").\n");
    DebugPrintString(g_TempBuffer);
#endif
  }

  /* Update the players, step by step.  Hopefully most times players are moving
     at less than one tile per frame so we only have one step.  Top speed we
     can simulate is 128 steps, or 128 tiles per frame, half a screen width per
     frame, which is pretty fast and likely unplayable.

     First update the position, then do player to player collisions (since that
     affects tile depositing - forced harvest mode after a collision), then
     tile collisions and depositing and finally wall collisions (which ensure
     that the player is back on the board so we don't have to worry about
     invalid coordinates after the simulation for a step is done). */

  for (iStep = 0; iStep < numberOfSteps; iStep ++)
  {
#if DEBUG_PRINT_SIM
    strcpy(g_TempBuffer, "Substep ");
    AppendDecimalUInt16(iStep);
    strcat(g_TempBuffer, ":\n");
    DebugPrintString(g_TempBuffer);
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
      strcpy(g_TempBuffer, "Player #");
      AppendDecimalUInt16(iPlayer);
      strcat(g_TempBuffer, ": new pos (");
      AppendDecimalInt16(GET_FX_INTEGER(pPlayer->pixel_center_x));
      strcat(g_TempBuffer, ".");
      AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->pixel_center_x));
      strcat(g_TempBuffer, ", ");
      AppendDecimalInt16(GET_FX_INTEGER(pPlayer->pixel_center_y));
      strcat(g_TempBuffer, ".");
      AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->pixel_center_y));
      strcat(g_TempBuffer, ")\n");
      DebugPrintString(g_TempBuffer);
#endif
    }

  /* Check for player to player collisions.  If they are close enough, do the
     collision by exchanging velocities.  Players recently collided don't
     suffer more collisions, so they can escape from the scene without getting
     stuck on the other player. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE || pPlayer->player_collision_count)
      continue;

    int16_t playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);
    int16_t playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);

    uint8_t iOtherPlayer;
    player_pointer pOtherPlayer;
    for (iOtherPlayer = iPlayer + 1, pOtherPlayer = pPlayer + 1;
    iOtherPlayer < MAX_PLAYERS;
    iOtherPlayer++, pOtherPlayer++)
    {
      if (pOtherPlayer->brain == BRAIN_INACTIVE ||
      pOtherPlayer->player_collision_count)
        continue;

      /* Also have to be at near the same altitude as the other player. */

      int8_t deltaFlyingHeight;
      deltaFlyingHeight = (int8_t) pPlayer->pixel_flying_height -
        (int8_t) pOtherPlayer->pixel_flying_height;
      if (deltaFlyingHeight < 0)
        deltaFlyingHeight = -deltaFlyingHeight;
      if (deltaFlyingHeight >= PLAYER_PIXEL_DIAMETER_NORMAL)
        continue;

      /* Find out how far apart the players are.  If it's less than a ball
         width (actually two half widths from center to center), a collision
         has happened.  Has to touch in both X and Y. */

      bool touching;
      int16_t distance;

      distance = playerX - GET_FX_INTEGER(pOtherPlayer->pixel_center_x);
      touching = (distance > -PLAYER_PIXEL_DIAMETER_NORMAL &&
        distance < PLAYER_PIXEL_DIAMETER_NORMAL);
      if (!touching)
        continue;

      distance = playerY - GET_FX_INTEGER(pOtherPlayer->pixel_center_y);
      touching = (distance > -PLAYER_PIXEL_DIAMETER_NORMAL &&
        distance < PLAYER_PIXEL_DIAMETER_NORMAL);
      if (!touching)
        continue;

      /* A player on player collision has happened!  Exchange velocities. */

      pPlayer->player_collision_count += 15; /* Take a while to recover. */
      pOtherPlayer->player_collision_count += 15;
      PlaySound(SOUND_BALL_HIT, pPlayer);

      SWAP_FX(&pPlayer->velocity_x, &pOtherPlayer->velocity_x);
      SWAP_FX(&pPlayer->velocity_y, &pOtherPlayer->velocity_y);
      SWAP_FX(&pPlayer->step_velocity_x, &pOtherPlayer->step_velocity_x);
      SWAP_FX(&pPlayer->step_velocity_y, &pOtherPlayer->step_velocity_y);

#if 1
      /* Separate the players if they are moving too slowly - add a velocity
         boost in the existing biggest velocity X or Y component. */

      fx deltaVelXfx, deltaVelYfx;
      SUBTRACT_FX(&pPlayer->velocity_x, &pOtherPlayer->velocity_x, &deltaVelXfx);
      SUBTRACT_FX(&pPlayer->velocity_y, &pOtherPlayer->velocity_y, &deltaVelYfx);
      fx absVelX, absVelY;
      COPY_ABS_FX(&deltaVelXfx, &absVelX);
      COPY_ABS_FX(&deltaVelYfx, &absVelY);

      /* Find out which velocity component is larger. */
      if (COMPARE_FX(&absVelX, &absVelY) >= 0)
      {
        /* X velocity difference is bigger than Y.  Is it big enough to make
           the players separate already fast enough?  If it's small, tweak. */
        if (COMPARE_FX(&absVelX, &g_FrictionSpeedFx) < 0)
        {
          if (IS_NEGATIVE_FX(&deltaVelXfx)) /* Other player is moving right. */
          {
            ADD_FX(&g_SeparationVelocityFxAdd,
              &pOtherPlayer->velocity_x, &pOtherPlayer->velocity_x);
            ADD_FX(&g_SeparationVelocityFxStepAdd,
              &pOtherPlayer->step_velocity_x, &pOtherPlayer->step_velocity_x);
          }
          else /* Player moving right, so add to its velocity to separate. */
          {
            ADD_FX(&g_SeparationVelocityFxAdd,
              &pPlayer->velocity_x, &pPlayer->velocity_x);
            ADD_FX(&g_SeparationVelocityFxStepAdd,
              &pPlayer->step_velocity_x, &pPlayer->step_velocity_x);
          }
        }
      }
      else /* Y velocity component is larger. */
      {
        /* Is it big enough to make the players separate already fast enough?
           If it's small, tweak. */
        if (COMPARE_FX(&absVelY, &g_FrictionSpeedFx) < 0)
        {
          if (IS_NEGATIVE_FX(&deltaVelYfx)) /* Other player is moving down. */
          {
            ADD_FX(&g_SeparationVelocityFxAdd,
              &pOtherPlayer->velocity_y, &pOtherPlayer->velocity_y);
            ADD_FX(&g_SeparationVelocityFxStepAdd,
              &pOtherPlayer->step_velocity_y, &pOtherPlayer->step_velocity_y);
          }
          else /* Player is moving down, so add to its velocity to separate. */
          {
            ADD_FX(&g_SeparationVelocityFxAdd,
              &pPlayer->velocity_y, &pPlayer->velocity_y);
            ADD_FX(&g_SeparationVelocityFxStepAdd,
              &pPlayer->step_velocity_y, &pPlayer->step_velocity_y);
          }
        }
      }
#endif /* Separate players. */
    }
  }

    /* Check for player to tile collisions.  Like a tic-tac-toe board, examine
       9 tiles around the player, unless too close to the side of the play area,
       then it's less. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      uint8_t startCol, endCol, curCol, playerCol;
      uint8_t startRow, endRow, curRow, playerRow;
      int16_t playerX, playerY;
      int8_t velocityX, velocityY;
      tile_owner player_self_owner;

      if (pPlayer->brain == BRAIN_INACTIVE)
        continue;

      if (pPlayer->pixel_flying_height >= FLYING_ABOVE_TILES_HEIGHT)
        continue;

      playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);
      playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);

      playerCol = playerX / TILE_PIXEL_WIDTH;
      playerRow = playerY / TILE_PIXEL_WIDTH;
      player_self_owner = iPlayer + (tile_owner) OWNER_PLAYER_1;

      /* Tile miss distance affects how thick a trail the player leaves.  And
         which tiles they bounce off of.  And with walls, the smallest gap they
         can get through.  So we make the player slightly thinner than they
         show on screen so that they leave a smaller trail.  Though too thin
         and they can go through walls. */

      int8_t missDistance, missMinusDistance;
      if (pPlayer->power_up_timers[OWNER_PUP_WIDER])
        missDistance =
          (TILE_PIXEL_WIDTH + 3 * PLAYER_PIXEL_DIAMETER_NORMAL / 2) / 2;
      else
        missDistance =
          (TILE_PIXEL_WIDTH + PLAYER_PIXEL_DIAMETER_NORMAL) / 2;
      missMinusDistance = -missDistance;

      /* Just need the +1/0/-1 for direction of velocity. */
      velocityX = TEST_FX(&pPlayer->step_velocity_x);
      velocityY = TEST_FX(&pPlayer->step_velocity_y);

#if DEBUG_PRINT_SIM
      strcpy(g_TempBuffer, "Player #");
      AppendDecimalUInt16(iPlayer);
      strcat(g_TempBuffer, ": collision tests at tile (");
      AppendDecimalUInt16(playerCol);
      strcat(g_TempBuffer, ", ");
      AppendDecimalUInt16(playerRow);
      strcat(g_TempBuffer, "), vel dir (");
      AppendDecimalInt16(velocityX);
      strcat(g_TempBuffer, ", ");
      AppendDecimalInt16(velocityY);
      strcat(g_TempBuffer, ").\n");
      DebugPrintString(g_TempBuffer);
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
         player goes through tiles.  So just collect one bounce in each axis.
         Also collect the coordinates of the center of the closest tile they
         bounce off , so we can later move the player outside the tile
         (otherwise they can ram their way through a tile, even an
         indestructible one). */

      bool bounceOffX = false;
      bool bounceOffY = false;

      /* Start bounce off coordinates (tile centers are used) far away on
         opposite side, so all tile sides we may bounce off will be closer
         (can hit tile up to 2 tile widths away from the player).  Previously
         was initialising at the player position, which was incorrect in some
         situations. */

      int16_t bounceOffPixelX = playerX;
      if (velocityX < 0)
        bounceOffPixelX -= TILE_PIXEL_WIDTH * 3 + PLAYER_PIXEL_DIAMETER_NORMAL;
      else if (velocityX > 0)
        bounceOffPixelX += TILE_PIXEL_WIDTH * 3 + PLAYER_PIXEL_DIAMETER_NORMAL;

      int16_t bounceOffPixelY = playerY;
      if (velocityY < 0)
        bounceOffPixelY -= TILE_PIXEL_WIDTH * 3 + PLAYER_PIXEL_DIAMETER_NORMAL;
      else if (velocityY > 0)
        bounceOffPixelY += TILE_PIXEL_WIDTH * 3 + PLAYER_PIXEL_DIAMETER_NORMAL;

      /* Scan the up to 9 tiles around the player's position. */

      for (curRow = startRow; curRow <= endRow; curRow++)
      {
        tile_pointer pTile;
        pTile = g_tile_array_row_starts[curRow];
        if (pTile == NULL)
          break; /* Shouldn't happen. */

        pTile += startCol;
        for (curCol = startCol; curCol <= endCol; curCol++, pTile++)
        {
          /* Find relative (delta) position of player to tile, positive X values
             if player is to the right of tile, or positive Y if player below
             tile.  Can fit in a byte since we're only a few pixels away from
             the tile.  Theoretically should use the FX fixed point positions
             and velocities, but that's too much computation for a Z80 to do,
             so we use integer pixels. */

          int8_t deltaPosX = playerX - pTile->pixel_center_x;
          if (deltaPosX >= missDistance || deltaPosX <= missMinusDistance)
            continue; /* Too far away, missed. */

          int8_t deltaPosY = playerY - pTile->pixel_center_y;
          if (deltaPosY >= missDistance || deltaPosY <= missMinusDistance)
            continue; /* Too far away, missed. */

          tile_owner previousOwner = pTile->owner;

#if DEBUG_PRINT_SIM
          strcpy(g_TempBuffer, "Player #");
          AppendDecimalUInt16(iPlayer);
          strcat(g_TempBuffer, ": touches tile ");
          strcat(g_TempBuffer, g_TileOwnerNames[previousOwner]);
          if (previousOwner >= (tile_owner) OWNER_PLAYER_1 &&
          previousOwner <= (tile_owner) OWNER_PLAYER_4)
          {
            strcat(g_TempBuffer, "/");
            AppendDecimalUInt16(pTile->age);
          }
          strcat(g_TempBuffer, " at (");
          AppendDecimalUInt16(curCol);
          strcat(g_TempBuffer, ", ");
          AppendDecimalUInt16(curRow);
          strcat(g_TempBuffer, "), deltaPos (");
          AppendDecimalInt16(deltaPosX);
          strcat(g_TempBuffer, ", ");
          AppendDecimalInt16(deltaPosY);
          strcat(g_TempBuffer, ").\n");
          DebugPrintString(g_TempBuffer);
#endif
          /* Collided with empty tile? */

          if (previousOwner == (tile_owner) OWNER_EMPTY)
          {
            /* If we are harvesting, don't take over the tile, we extracted
              nothing from it, leave it empty.  Otherwise we get a little
              oscillation as the stationary player takes a tile, then
              harvests it back to empty the next update, making the
              score flicker.  If not harvesting, it becomes our tile.
              If we have a recent collision, don't take over the tile, to
              avoid having the player or the one it collided with bouncing
              off the new tile repeatedly. */

            if (!pPlayer->thrust_active && iStep == 0 &&
            pPlayer->player_collision_count == 0 &&
            (((((uint8_t) g_FrameCounter ^ iPlayer) & 3) == 0) ||
            (pPlayer->power_up_timers[OWNER_PUP_SOLID])))
              SetTileOwner(pTile, player_self_owner);

            continue; /* Just glide over empty tiles, no bouncing. */
          }

          /* Did we run over our existing tile?  If so, age it a bit (make it
             more solid). */

          uint8_t tileAge = pTile->age;

          if (previousOwner == player_self_owner)
          {
            /* If harvesting tiles for thrust, or after a collision to reduce
               the number of tiles that players keep on bouncing off of. */
            if (pPlayer->thrust_active || pPlayer->player_collision_count)
            {
              pPlayer->thrust_harvested += tileAge + 1;
              SetTileOwner(pTile, OWNER_EMPTY);
            }
            else /* Just running over our tiles, increase their age. */
            {
              /* To save on CPU, only increase tile age occasionally.  Also
                 avoids players picking up too much speed from harvesting
                 rapidly, which leads to AIs bouncing around the corner but
                 not getting into the corner.  Though the OWNER_PUP_SOLID
                 power-up overrides this and increases age every frame.  Note
                 age is 3 bits in the tile record, so maximum 7. */
              if (tileAge < 7 && iStep == 0 &&
              (((((uint8_t) g_FrameCounter ^ iPlayer) & 3) == 0) ||
              (pPlayer->power_up_timers[OWNER_PUP_SOLID])))
              {
                tileAge++;
                pTile->age = tileAge;
                RequestTileRedraw(pTile);
              }
            }
            continue; /* Don't collide, keep moving over own tiles. */
          }

#if DEBUG_PRINT_SIM
          strcpy(g_TempBuffer, "Player #");
          AppendDecimalUInt16(iPlayer);
          strcat(g_TempBuffer, ": Hit tile ");
          strcat(g_TempBuffer, g_TileOwnerNames[previousOwner]);
          if (previousOwner >= (tile_owner) OWNER_PLAYER_1 &&
          previousOwner <= (tile_owner) OWNER_PLAYER_4)
          {
            strcat(g_TempBuffer, "/");
            AppendDecimalUInt16(pTile->age);
          }
          strcat(g_TempBuffer, " at (");
          AppendDecimalUInt16(curCol);
          strcat(g_TempBuffer, ", ");
          AppendDecimalUInt16(curRow);
          strcat(g_TempBuffer, "), check for effects.\n");
          DebugPrintString(g_TempBuffer);
#endif
          /* Hit someone else's tile or a power-up.  If it's a tile, it gets
             weakened (age decreases) and when done it gets destroyed and
             perhaps (depending on harvest mode) replaced by a player tile.
             Power-ups are destroyed immediately, and have their effect applied
             to the player and perhaps are replaced by a player tile. */

          bool takeOverTile = false;
          if (previousOwner >= (tile_owner) OWNER_PLAYER_1 &&
          previousOwner <= (tile_owner) OWNER_PLAYER_4)
          {
            if (g_TileAgeFeature == 0 || tileAge == 0 ||
            pPlayer->power_up_timers[OWNER_PUP_BASH_WALL])
              takeOverTile = true;
            else /* Wear down the tile. */
            {
              tileAge--;
              pTile->age = tileAge;
              RequestTileRedraw(pTile);
            }
          }
          else if (previousOwner >= (tile_owner) OWNER_WALL_INDESTRUCTIBLE &&
          previousOwner <= (tile_owner) OWNER_WALL_DESTRUCTIBLE_P4)
          {
            /* Tile is indestructible, unless you are the player that can
               destroy that kind of tile. */

            takeOverTile =
              ((iPlayer + (tile_owner) OWNER_WALL_DESTRUCTIBLE_P1) ==
              previousOwner);
          }
          else /* A power up tile. */
          {
            takeOverTile = true;

            /* Activate a power up.  Since the power-up count down clock wraps
               at 256, and counts at a 5hz rate, allow for several power ups in
               a row before the clock wraps.  At 5hz, 50 is 10 seconds, should
               be enough. */

            if (previousOwner <= (tile_owner) OWNER_PUP_NORMAL)
            {
              /* Power down, go back to Normal - turn off all power-ups. */
              bzero(&pPlayer->power_up_timers, sizeof(pPlayer->power_up_timers));
              /* Also trigger a STOP power-up when you hit the NORMAL one. */
              pPlayer->power_up_timers[OWNER_PUP_STOP] = 50;
            }
            else if (previousOwner < (tile_owner) OWNER_MAX)
              pPlayer->power_up_timers[previousOwner] += 50;
          }

          if (takeOverTile)
          {
            if (pPlayer->thrust_active || pPlayer->player_collision_count)
              SetTileOwner(pTile, OWNER_EMPTY);
            else
              SetTileOwner(pTile, player_self_owner);
          }

          /* Do the bouncing.  First find out which side of the tile was hit,
             based on the side facing the player's current position.  It
             actually is within collision distance of all sides, but don't
             want to bounce off both sides of the tile.  No bounce if the
             player is moving away from the selected side.  Also skip sides
             that are adjacent to a bounceable tile, since they are between
             tiles and can't be actually hit. */

          int8_t absDeltaPosX, absDeltaPosY;

          if (deltaPosX < 0)
            absDeltaPosX = -deltaPosX;
          else
            absDeltaPosX = deltaPosX;

          if (deltaPosY < 0)
            absDeltaPosY = -deltaPosY;
          else
            absDeltaPosY = deltaPosY;

          if (absDeltaPosY >= absDeltaPosX)
          {
            /* Player is off the top or bottom of the tile.  We are assuming
               they aren't moving so fast that they have penetrated more than
               half the tile thickness. */

            if (deltaPosY < 0)
            {
              /* Player is on top half of the tile.  The player position is
                 inside a cone on the upwards Y axis of the tile center,
                 bounded by the 45 degree diagonals abs(X)==abs(Y) relative to
                 the tile center. */
#if DEBUG_PRINT_SIM
              strcpy(g_TempBuffer, "Player #");
              AppendDecimalUInt16(iPlayer);
              strcat(g_TempBuffer, ": Nearest to top side of tile.\n");
              DebugPrintString(g_TempBuffer);
#endif
              if (velocityY > 0) /* Moving downward to hit it. */
              {
                /* Is there another non-empty tile above this tile?  If so,
                   this side is impossible to actually hit. */

                tile_owner adjacentOwner = (tile_owner) OWNER_EMPTY;

                tile_pointer pAdjacentTile =
                  TileForColumnAndRow(curCol, curRow-1);
                if (pAdjacentTile != NULL)
                  adjacentOwner = pAdjacentTile->owner;
                else /* Off top of board, effectively a wall there. */
                  adjacentOwner = (tile_owner) OWNER_WALL_INDESTRUCTIBLE;

                if (adjacentOwner == (tile_owner) OWNER_EMPTY ||
                adjacentOwner == player_self_owner)
                {
                  int16_t tilePixelXY = pTile->pixel_center_y;
                  if (tilePixelXY < bounceOffPixelY)
                    bounceOffPixelY = tilePixelXY;
                  bounceOffY = true;
#if DEBUG_PRINT_SIM
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Bounced off top side of tile with center Y of ");
                  AppendDecimalUInt16(tilePixelXY);
                  strcat(g_TempBuffer, ".\n");
                  DebugPrintString(g_TempBuffer);
#endif
                }
#if DEBUG_PRINT_SIM
                else
                {
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Can't bounce off top interior side of tile pair.\n");
                  DebugPrintString(g_TempBuffer);
                }
#endif
              }
            }
            else  /* Hit on bottom side of the tile. */
            {
#if DEBUG_PRINT_SIM
              strcpy(g_TempBuffer, "Player #");
              AppendDecimalUInt16(iPlayer);
              strcat(g_TempBuffer, ": Nearest to bottom side of tile.\n");
              DebugPrintString(g_TempBuffer);
#endif
              if (velocityY < 0) /* Moving upward to hit it. */
              {
                /* Is there another non-empty tile below this tile?  If so,
                   this side is impossible to actually hit. */

                tile_owner adjacentOwner = (tile_owner) OWNER_EMPTY;

                tile_pointer pAdjacentTile =
                  TileForColumnAndRow(curCol, curRow+1);
                if (pAdjacentTile != NULL)
                  adjacentOwner = pAdjacentTile->owner;
                else /* Off bottom of board, effectively a wall there. */
                  adjacentOwner = (tile_owner) OWNER_WALL_INDESTRUCTIBLE;

                if (adjacentOwner == (tile_owner) OWNER_EMPTY ||
                adjacentOwner == player_self_owner)
                {
                  int16_t tilePixelXY = pTile->pixel_center_y;
                  if (tilePixelXY > bounceOffPixelY)
                    bounceOffPixelY = tilePixelXY;
                  bounceOffY = true;
#if DEBUG_PRINT_SIM
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Bounced off bottom side of tile with center Y of ");
                  AppendDecimalUInt16(tilePixelXY);
                  strcat(g_TempBuffer, ".\n");
                  DebugPrintString(g_TempBuffer);
#endif
                }
#if DEBUG_PRINT_SIM
                else
                {
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Can't bounce off bottom interior side of tile pair.\n");
                  DebugPrintString(g_TempBuffer);
                }
#endif
              }
            }
          }
          else /* Hit on left or right of the tile. */
          {
            /* Player is closer to left or right side of the tile than the
               top/bottom. */

            if (deltaPosX < 0)
            {
              /* Player is on left half of the tile. */
#if DEBUG_PRINT_SIM
                strcpy(g_TempBuffer, "Player #");
                AppendDecimalUInt16(iPlayer);
                strcat(g_TempBuffer, ": Nearest to left side of tile.\n");
                DebugPrintString(g_TempBuffer);
#endif
              if (velocityX > 0) /* Need to be moving right to hit it. */
              {
                /* Is there another non-empty tile left of this tile?  If so,
                   this side is impossible to actually hit. */

                tile_owner adjacentOwner = (tile_owner) OWNER_EMPTY;
                if (curCol >= 1) /* Not at the left edge of the board. */
                  adjacentOwner = pTile[-1].owner;
                else /* Off left of board, effectively a wall there. */
                  adjacentOwner = (tile_owner) OWNER_WALL_INDESTRUCTIBLE;

                if (adjacentOwner == (tile_owner) OWNER_EMPTY ||
                adjacentOwner == player_self_owner)
                {
                  int16_t tilePixelXY = pTile->pixel_center_x;
                  if (tilePixelXY < bounceOffPixelX)
                    bounceOffPixelX = tilePixelXY;
                  bounceOffX = true;
#if DEBUG_PRINT_SIM
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Bounced off left side of tile with center X of ");
                  AppendDecimalUInt16(tilePixelXY);
                  strcat(g_TempBuffer, ".\n");
                  DebugPrintString(g_TempBuffer);
#endif
                }
#if DEBUG_PRINT_SIM
                else
                {
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Can't bounce off left interior side of tile pair.\n");
                  DebugPrintString(g_TempBuffer);
                }
#endif
              }
            }
            else /* Player is on right half of the tile.  */
            {
#if DEBUG_PRINT_SIM
                strcpy(g_TempBuffer, "Player #");
                AppendDecimalUInt16(iPlayer);
                strcat(g_TempBuffer, ": Nearest to right side of tile.\n");
                DebugPrintString(g_TempBuffer);
#endif
              if (velocityX < 0) /* Need to be moving left to hit it. */
              {
                /* Is there another non-empty tile right of this tile?  If so,
                   this side is impossible to actually hit. */

                tile_owner adjacentOwner = (tile_owner) OWNER_EMPTY;
                if (curCol < g_play_area_width_tiles - 1) /* Board right. */
                  adjacentOwner = pTile[1].owner;
                else /* Off right of board, effectively a wall there. */
                  adjacentOwner = (tile_owner) OWNER_WALL_INDESTRUCTIBLE;

                if (adjacentOwner == (tile_owner) OWNER_EMPTY ||
                adjacentOwner == player_self_owner)
                {
                  int16_t tilePixelXY = pTile->pixel_center_x;
                  if (tilePixelXY > bounceOffPixelX)
                    bounceOffPixelX = tilePixelXY;
                  bounceOffX = true;
#if DEBUG_PRINT_SIM
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Bounced off right side of tile with center X of ");
                  AppendDecimalUInt16(tilePixelXY);
                  strcat(g_TempBuffer, ".\n");
                  DebugPrintString(g_TempBuffer);
#endif
                }
#if DEBUG_PRINT_SIM
                else
                {
                  strcpy(g_TempBuffer, "Player #");
                  AppendDecimalUInt16(iPlayer);
                  strcat(g_TempBuffer,
                    ": Can't bounce off right interior side of tile pair.\n");
                  DebugPrintString(g_TempBuffer);
                }
#endif
              }
            }
          }
        }
      }

      /* Now do the bouncing based on the directions we hit. */

      if (bounceOffX)
      {
        /* Bounce off left or right side.  Same velocity reversal. */
        NEGATE_FX(&pPlayer->velocity_x);
        NEGATE_FX(&pPlayer->step_velocity_x);

        /* Shove the player outside the tile side. */

        if (velocityX < 0) /* Player is moving leftwards. */
        {
          int16_t tileSideX = bounceOffPixelX + missDistance;
          if (playerX < tileSideX) /* Has gone too far inside the tile. */
          {
            INT_TO_FX(tileSideX, pPlayer->pixel_center_x);
#if DEBUG_PRINT_SIM
            strcpy(g_TempBuffer, "Player #");
            AppendDecimalUInt16(iPlayer);
            strcat(g_TempBuffer,
              ": Shoved to right side of tile, new player X is ");
            AppendDecimalUInt16(tileSideX);
            strcat(g_TempBuffer, ".\n");
            DebugPrintString(g_TempBuffer);
#endif
          }
        }
        else /* Player is moving rightwards. */
        {
          int16_t tileSideX = bounceOffPixelX + missMinusDistance;
          if (playerX > tileSideX) /* Has gone too far inside the tile. */
          {
            INT_TO_FX(tileSideX, pPlayer->pixel_center_x);
#if DEBUG_PRINT_SIM
            strcpy(g_TempBuffer, "Player #");
            AppendDecimalUInt16(iPlayer);
            strcat(g_TempBuffer,
              ": Shoved to left side of tile, new player X is ");
            AppendDecimalUInt16(tileSideX);
            strcat(g_TempBuffer, ".\n");
            DebugPrintString(g_TempBuffer);
#endif
          }
        }
      }

      if (bounceOffY)
      { /* Bounce off top or bottom side. */
        NEGATE_FX(&pPlayer->velocity_y);
        NEGATE_FX(&pPlayer->step_velocity_y);

        /* Shove the player outside the tile side. */

        if (velocityY < 0) /* Player is moving upwards. */
        {
          int16_t tileSideY = bounceOffPixelY + missDistance;
          if (playerY < tileSideY) /* Has gone too far inside the tile. */
          {
            INT_TO_FX(tileSideY, pPlayer->pixel_center_y);
#if DEBUG_PRINT_SIM
            strcpy(g_TempBuffer, "Player #");
            AppendDecimalUInt16(iPlayer);
            strcat(g_TempBuffer,
              ": Shoved to bottom side of tile, new player Y is ");
            AppendDecimalUInt16(tileSideY);
            strcat(g_TempBuffer, ".\n");
            DebugPrintString(g_TempBuffer);
#endif
          }
        }
        else /* Player is moving downwards. */
        {
          int16_t tileSideY = bounceOffPixelY + missMinusDistance;
          if (playerY > tileSideY) /* Has gone too far inside the tile. */
          {
            INT_TO_FX(tileSideY, pPlayer->pixel_center_y);
#if DEBUG_PRINT_SIM
            strcpy(g_TempBuffer, "Player #");
            AppendDecimalUInt16(iPlayer);
            strcat(g_TempBuffer,
              ": Shoved to top side of tile, new player Y is ");
            AppendDecimalUInt16(tileSideY);
            strcat(g_TempBuffer, ".\n");
            DebugPrintString(g_TempBuffer);
#endif
          }
        }
      }

      if (bounceOffX || bounceOffY)
        PlaySound(SOUND_TILE_HIT, pPlayer);
    }

    /* Bounce the players off the walls.  Also forces their position to be on
       the board, so do this as the last simulation action. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain == BRAIN_INACTIVE)
        continue;

      int16_t playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);

      if (playerX < g_play_area_wall_left_x)
      {
        if (IS_NEGATIVE_FX(&pPlayer->velocity_x))
        {
          NEGATE_FX(&pPlayer->velocity_x);
          NEGATE_FX(&pPlayer->step_velocity_x);
        }
        INT_TO_FX(g_play_area_wall_left_x, pPlayer->pixel_center_x);
        PlaySound(SOUND_WALL_HIT, pPlayer);
      }

      if (playerX > g_play_area_wall_right_x)
      {
        if (!IS_NEGATIVE_FX(&pPlayer->velocity_x))
        {
          NEGATE_FX(&pPlayer->velocity_x);
          NEGATE_FX(&pPlayer->step_velocity_x);
        }
        INT_TO_FX(g_play_area_wall_right_x, pPlayer->pixel_center_x);
        PlaySound(SOUND_WALL_HIT, pPlayer);
      }

      int16_t playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);

      if (playerY > g_play_area_wall_bottom_y)
      {
        if (!IS_NEGATIVE_FX(&pPlayer->velocity_y))
        {
          NEGATE_FX(&pPlayer->velocity_y);
          NEGATE_FX(&pPlayer->step_velocity_y);
        }
        INT_TO_FX(g_play_area_wall_bottom_y, pPlayer->pixel_center_y);
        PlaySound(SOUND_WALL_HIT, pPlayer);
      }

      if (playerY < g_play_area_wall_top_y)
      {
        if (IS_NEGATIVE_FX(&pPlayer->velocity_y))
        {
          NEGATE_FX(&pPlayer->velocity_y);
          NEGATE_FX(&pPlayer->step_velocity_y);
        }
        INT_TO_FX(g_play_area_wall_top_y, pPlayer->pixel_center_y);
        PlaySound(SOUND_WALL_HIT, pPlayer);
      }
    }
  }
}

