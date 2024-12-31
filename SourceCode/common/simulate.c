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
  uint8_t numberOfSteps;
  uint8_t iPlayer;
  player_pointer pPlayer;
  uint8_t stepShiftCount;
  fx stepPlayerVelocityX[MAX_PLAYERS];
  fx stepPlayerVelocityY[MAX_PLAYERS];

  /* Find the largest velocity component of all the players. */

  ZERO_FX(maxVelocity);
  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    absVelocity = pPlayer->velocity_x;
    ABS_FX(absVelocity);
    if (COMPARE_FX(&absVelocity, &maxVelocity) > 0)
      maxVelocity = absVelocity;
  }

  /* Find the number of steps needed to ensure the maximum velocity is less
     than one tile per step, or at most something like 7.999 pixels.  Can divide
     velocity by using the stepShiftCount to get the velocity per step. */

  for (numberOfSteps = 1, stepShiftCount = 0;
  (numberOfSteps & 0x80) == 0;
  numberOfSteps *= 2, stepShiftCount++)
  {
    /* See if the step size is less than a tile width. */
    if (GET_FX_INTEGER(maxVelocity) < TILE_PIXEL_WIDTH)
      break; /* Step size is small enough now. */
    DIV2_FX(maxVelocity, maxVelocity);
  }

  /* Calculate the step velocity for each player. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer != MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    stepPlayerVelocityX[iPlayer].as_int32 =
      pPlayer->velocity_x.as_int32 >> stepShiftCount;
    stepPlayerVelocityY[iPlayer].as_int32 =
      pPlayer->velocity_y.as_int32 >> stepShiftCount;
  }
  /* bleeble */
  
}

