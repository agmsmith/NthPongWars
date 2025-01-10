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

    /* Check for tile collisions. */

/* bleeble */

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
#if DEBUG_PRINT_SIM
printf("Player %d: Bounced bottom wall\n", iPlayer);
printf("Player %d: Pos (%f, %f), Vel (%f,%f), Step (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y),
  GET_FX_FLOAT(pPlayer->step_velocity_x),
  GET_FX_FLOAT(pPlayer->step_velocity_y)
);
#endif
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
#if DEBUG_PRINT_SIM
printf("Player %d: Bounced left wall\n", iPlayer);
printf("Player %d: Pos (%f, %f), Vel (%f,%f), Step (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y),
  GET_FX_FLOAT(pPlayer->step_velocity_x),
  GET_FX_FLOAT(pPlayer->step_velocity_y)
);
#endif
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
#if DEBUG_PRINT_SIM
printf("Player %d: Bounced right wall\n", iPlayer);
printf("Player %d: Pos (%f, %f), Vel (%f,%f), Step (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y),
  GET_FX_FLOAT(pPlayer->step_velocity_x),
  GET_FX_FLOAT(pPlayer->step_velocity_y)
);
#endif
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
#if DEBUG_PRINT_SIM
printf("Player %d: Bounced top wall\n", iPlayer);
printf("Player %d: Pos (%f, %f), Vel (%f,%f), Step (%f,%f)\n", iPlayer,
  GET_FX_FLOAT(pPlayer->pixel_center_x),
  GET_FX_FLOAT(pPlayer->pixel_center_y),
  GET_FX_FLOAT(pPlayer->velocity_x),
  GET_FX_FLOAT(pPlayer->velocity_y),
  GET_FX_FLOAT(pPlayer->step_velocity_x),
  GET_FX_FLOAT(pPlayer->step_velocity_y)
);
#endif
      }
    }
  }
}

