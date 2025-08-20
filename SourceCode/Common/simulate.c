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

#define DEBUG_PRINT_SIM 0 /* Turn on debug printfs, uses floating point. */

#include "sounds.h"


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
 *
 * To save on compute, 16 bit integer coordinates (whole pixels) are used for
 * collision detection, with only the more precise fixed point numbers for
 * position and velocity calculations.
 */
void Simulate(void)
{
  int16_t absVelocity;
  int16_t maxVelocity;
  uint8_t iStep;
  uint8_t numberOfSteps;
  uint8_t iPlayer;
  player_pointer pPlayer;
  uint8_t stepShiftCount;
  static uint8_t s_PreviousStepShiftCount = 0; /* To tell when it changes. */

#if DEBUG_PRINT_SIM
printf("\nStarting simulation update.\n");
#endif

  /* Reduce adaptive harvest sound effect amount of harvest limit once per
     frame.  New bigger harvests will bump it back up and make noise. */

  if (g_harvest_sound_threshold != 0)
    g_harvest_sound_threshold--;

  /* Find the largest velocity component (X or Y) of all the players,
     approximate it with only looking at integer portion for speed, ignoring
     the fractional part.  Also reset thrust harvested, which gets accumulated
     during collisions with tiles and gets used to increase velocity on the
     next update.  Also updates players's Manhattan Speed value (since we're
     finding absolute values of the velocity here anyway), which lags by a
     frame since it's determined before moving the player. */

  maxVelocity = 0;
  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    /* Various start of frame initialisations for all players go here. */

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
    }

#if DEBUG_PRINT_SIM
printf("Player %d: pos (%f, %f), vel (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y)
);
#endif
    int16_t tempSpeed;

    if (pPlayer->power_up_timers[OWNER_PUP_STOP])
    {
      pPlayer->power_up_timers[OWNER_PUP_STOP] = 0; /* No duration needed. */
      ZERO_FX(pPlayer->velocity_x);
      ZERO_FX(pPlayer->velocity_y);
      tempSpeed = 0;
    }
    else
    {
      absVelocity = GET_FX_INTEGER(pPlayer->velocity_x);
      if (absVelocity < 0)
        absVelocity = -absVelocity;
      tempSpeed = absVelocity;
      if (absVelocity > maxVelocity)
        maxVelocity = absVelocity;

      absVelocity = GET_FX_INTEGER(pPlayer->velocity_y);
      if (absVelocity < 0)
        absVelocity = -absVelocity;
      tempSpeed += absVelocity;
      if (absVelocity > maxVelocity)
        maxVelocity = absVelocity;
    }

    /* Save the Manhattan speed, into an 8 bit integer, which should be good
       enough since it is integer pixels per frame, and 256 would be moving the
       width of the screen every frame, way too fast to care about. */

    if (tempSpeed < 0)
      pPlayer->speed = 0; /* Shouldn't happen unless overflowed 16 bit speed! */
    else if (tempSpeed >= 256)
      pPlayer->speed = 255; /* Clamp at maximum unsigned 8 bit value. */
    else
      pPlayer->speed = tempSpeed;
  }

#if DEBUG_PRINT_SIM
printf("Max integer velocity component: %d\n", maxVelocity);
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
    /* See if the step size is less than a tile width. */
    if (maxVelocity < TILE_PIXEL_WIDTH)
      break; /* Step size is small enough now. */
    maxVelocity >>= 1;
  }

#if DEBUG_PRINT_SIM
printf("Have %d steps, shift by %d\n", numberOfSteps, stepShiftCount);
#endif

  /* Update our cached constant velocity change for collisions. */

  if (s_PreviousStepShiftCount != stepShiftCount)
  {
    g_SeparationVelocityFxStepAdd.as_int32 =
      g_SeparationVelocityFxAdd.as_int32 >> stepShiftCount;
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
     frame, which is pretty fast and likely unplayable.

     First update the position, then do player to player collisions (since that
     affects tile depositing - forced harvest mode after a collision), then
     tile collisions and despositing and finally wall collisions (which ensure
     that the player is back on the board so we don't have to worry about
     invalid coordinates after the simulation for a step is done). */

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

#if 1 /* Separate the players. */
      /* Separate the players if they are moving too slowly - add a velocity
         boost in the existing biggest velocity X or Y component. */

      fx deltaVelXfx, deltaVelYfx;
      SUBTRACT_FX(&pPlayer->velocity_x, &pOtherPlayer->velocity_x, &deltaVelXfx);
      SUBTRACT_FX(&pPlayer->velocity_y, &pOtherPlayer->velocity_y, &deltaVelYfx);
      fx absVelX, absVelY;
      COPY_ABS_FX(&deltaVelXfx, &absVelX);
      COPY_ABS_FX(&deltaVelYfx, &absVelY);

      fx tooSlowSpeedFx;
      INT_TO_FX(FRICTION_SPEED, tooSlowSpeedFx);

      /* Find out which velocity component is larger. */
      if (COMPARE_FX(&absVelX, &absVelY) >= 0)
      {
        /* X velocity difference is bigger than Y.  Is it big enough to make
           the players separate already fast enough?  If it's small, tweak. */
        if (COMPARE_FX(&absVelX, &tooSlowSpeedFx) < 0)
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
        if (COMPARE_FX(&absVelY, &tooSlowSpeedFx) < 0)
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
      int8_t velocityX, velocityY;
      bool bounceOffX, bounceOffY;
      tile_owner player_self_owner;

      if (pPlayer->brain == BRAIN_INACTIVE)
        continue;

      int16_t playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);
      int16_t playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);

      playerCol = playerX / TILE_PIXEL_WIDTH;
      playerRow = playerY / TILE_PIXEL_WIDTH;
      player_self_owner = iPlayer + (tile_owner) OWNER_PLAYER_1;

      /* Just need the +1/0/-1 for direction of velocity. */
      velocityX = TEST_FX(&pPlayer->step_velocity_x);
      velocityY = TEST_FX(&pPlayer->step_velocity_y);

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
          /* Find relative (delta) position of player to tile, positive X values
             if player is to the right of tile, or positive Y if player below
             tile.  Can fit in a byte since we're only a few pixels away from
             the tile.  Theoretically should use the FX fixed point positions
             and velocities, but that's too much computation for a Z80 to do,
             so we use integer pixels. */

          int8_t missDistance, missMinusDistance;
          if (pPlayer->power_up_timers[OWNER_PUP_WIDER])
            missDistance =
              ((TILE_PIXEL_WIDTH + PLAYER_PIXEL_DIAMETER_NORMAL) / 2);
          else
            missDistance = (TILE_PIXEL_WIDTH / 2 + 1);
          missMinusDistance = -missDistance;

          int8_t deltaPosX, deltaPosY;
          deltaPosX = playerX - pTile->pixel_center_x;
          if (deltaPosX >= missDistance || deltaPosX <= missMinusDistance)
            continue; /* Too far away, missed. */

          deltaPosY = playerY - pTile->pixel_center_y;
          if (deltaPosY >= missDistance || deltaPosY <= missMinusDistance)
            continue; /* Too far away, missed. */

          tile_owner previousOwner = pTile->owner;
#if DEBUG_PRINT_SIM
printf("Player %d: Hit tile %s at (%d,%d)\n",
  iPlayer, g_TileOwnerNames[previousOwner], curCol, curRow);
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

            if (!pPlayer->thrust_active && pPlayer->player_collision_count == 0)
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
                 not getting into the corner.  Note age is 3 bits, max 7. */
              if (tileAge < 7 &&
              (((uint8_t) g_FrameCounter ^ iPlayer) & 3) == 0)
              {
                tileAge++;
                pTile->age = tileAge;
                RequestTileRedraw(pTile);
              }
            }
            continue; /* Don't collide, keep moving over own tiles. */
          }

          /* Hit someone else's tile or a power-up.  If it's a tile, it gets
             weakened (age decreases) and when done it gets destroyed and
             perhaps (depending on harvest mode) replaced by a player tile.
             Power-ups are destroyed immediately, and have their effect applied
             to the player and perhaps are replaced by a player tile. */

          bool takeOverTile = false;
          if (previousOwner >= (tile_owner) OWNER_PLAYER_1 &&
          previousOwner <= (tile_owner) OWNER_PLAYER_4)
          {
            if (tileAge == 0)
              takeOverTile = true;
            else /* Wear down the tile. */
            {
              tileAge--;
              pTile->age = tileAge;
              RequestTileRedraw(pTile);
            }
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

#if DEBUG_PRINT_SIM
printf("Player %d: Bouncing off occupied tile (%d,%d).\n",
  iPlayer, curCol, curRow);
#endif
          /* Do the bouncing.  First find out which side of the tile was hit,
             based on the direction the player was moving.  No bounce if the
             player is moving away. */

          bool velXIsTowardsTile, velYIsTowardsTile;

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
        NEGATE_FX(&pPlayer->velocity_x);
        NEGATE_FX(&pPlayer->step_velocity_x);
      }

      if (bounceOffY)
      { /* Bounce off top or bottom side. */
        NEGATE_FX(&pPlayer->velocity_y);
        NEGATE_FX(&pPlayer->step_velocity_y);
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

