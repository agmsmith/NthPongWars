/******************************************************************************
 * Nth Pong Wars, scores.h for keeping track of scoring.
 *
 * AGMS20250119 - Start this header file.
 */
#ifndef _SCORES_H
#define _SCORES_H 1

/* Counts the number of frames drawn on screen.  Also used as a general timer
   for things like aging tiles.  Will be at most 60hz, but typically 20hz. */
extern uint16_t g_FrameCounter;

/* Current score of each player.  Counts the number of tiles they own, plus
   extra points for aged tiles.  Once a player has reached the goal score,
   the game is over.  Gave up on doing it as percentages, too CPU expensive.
   Score incremental updates are done in SetTileOwner(). */
extern uint16_t g_PlayerScores[MAX_PLAYERS];

/* The current number of points needed to win the game.  Decreases over time. */
extern uint16_t g_ScoreGoal;

/* Resets the goal (based on the number of tiles in the play area), recomputes
   the score for each player based on the state of the tiles in the play area,
   in case we want to reload a saved game or have pre-made scenarios. */
extern void InitialiseScores();

/* Update the screen display with the current scores. */
extern void DrawScores();

#endif /* _SCORES_H */

