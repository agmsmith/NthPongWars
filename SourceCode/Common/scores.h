/******************************************************************************
 * Nth Pong Wars, scores.h for keeping track of scoring.
 *
 * AGMS20250119 - Start this header file.
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
#ifndef _SCORES_H
#define _SCORES_H 1

/* Counts the number of frames drawn on screen, reset at level start.  Also
   used as a general timer for things like aging tiles.  Will run at best at
   60hz, but typically 30hz or even 20hz if a lot is happening on screen.  Score
   displays only last 4 digits, but it's a 16 bit counter, which is allowed to
   wrap around so that differences work. */
extern uint16_t g_FrameCounter;

/* The current number of points needed to win the game.  Decreases over time. */
extern uint16_t g_ScoreGoal;

/* The number of frames it took to compute the game update last time, mostly
   for debugging as a letter near the frame rate.  Varies upwards from 1,
   will usually be 2 (30hz frame rate). */
extern uint8_t g_ScoreFramesPerUpdate;

/* Resets the goal score and forces a score display redraw on next update. */
extern void InitialiseScores(void);

/* Converts the player's scores into colourful text, cached in each player.
   Also update the goal text. */
extern void UpdateScores(void);

/* Update the screen display with the current scores.  They're the top line
   of the screen, snowing each player's score in their colour, followed by the
   goal score to win. */
extern void CopyScoresToScreen(void);

#endif /* _SCORES_H */

