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

void Simulate(void)
{
  uint8_t iPlayer;

  /* Calculate the new acceleration and velocity of all players.  We'll do the
     positions and collisions incrementally later. */
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
  {
    /* Acceleration is gravity (if any) and joystick direction. */
  }
}

#endif /* _SIMULATE_H */

