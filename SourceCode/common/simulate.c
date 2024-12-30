/******************************************************************************
 * Nth Pong Wars, simulate.h for updating positions of moving objects.
 *
 * Given the player inputs and current state of the players and tiles, move
 * things around to their positions for the next frame.  Along the way, queue
 * up sound effects and changes to tiles.  Later the higher priority sounds
 * will get played and only changes to tiles will get sent to the display and
 * networked players.
 *
 * AGMS20241108 - Start this header file.
 */
#ifndef _SIMULATE_H
#define _SIMULATE_H 1

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
 * of a 3x3 tick-tak-toe board.
 *
 * Player collisions with players are found by testing all combinations, 4
 * players means 6 tests (1-2, 1-3, 1-4, 2-3, 2-4, 3-4).
 *
 * When a collision occurs, the player bounces off the object.  For walls and
 * tiles, it's a simple reflection of velocity.  Walls also ensure the new
 * position is not inside the wall, overriding it to be just outside the wall
 * if needed (otherwise you can get balls bouncing around inside the wall - a
 * fault in the original Pong game).  For players it's a momentum conserving
 * bounce, so possibly the hit player will end up moving faster.
 */
void Simulate(void)
{
  uint8_t iPlayer;

  uint8_t iPlayer;
  player_pointer pPlayer;

  vdp_setWriteAddress(_vdpSpriteAttributeTableAddr);

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != BRAIN_INACTIVE &&
    pPlayer->vdpSpriteY != SPRITE_NOT_DRAWABLE)
    {
      /* Do the main ball sprite, if on screen. */
      IO_VDPDATA = pPlayer->vdpSpriteY;
      IO_VDPDATA = pPlayer->vdpSpriteX;
      IO_VDPDATA = pPlayer->main_anim.current_name;
      IO_VDPDATA = pPlayer->vdpEarlyClock32Left | pPlayer->main_colour;
    }
    pPlayer++;
  } while (iPlayer-- != 0);
}

#endif /* _SIMULATE_H */

