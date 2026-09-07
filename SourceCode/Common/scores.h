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
   will usually be 3 (20hz frame rate). */
extern uint8_t g_ScoreFramesPerUpdate;


/* For keeping track of high scores, locally and world wide over the
   Internet.  The global ones can have daily, weekly, monthly, yearly and
   all-time high score lists. */

#define MAX_SCORE_NAME_LENGTH 10 /* For "Yellow-bot" */
#define MAX_SCORE_TABLE_ENTRIES 10 /* So you can have a top 10 list. */

enum high_score_table_types_enum {
  HIGH_SCORE_TABLE_LOCAL = 0,
  HIGH_SCORE_TABLE_DAILY,
  HIGH_SCORE_TABLE_WEEKLY,
  HIGH_SCORE_TABLE_MONTHLY,
  HIGH_SCORE_TABLE_YEARLY,
  HIGH_SCORE_TABLE_ALLTIME,
  HIGH_SCORE_TABLE_MAX
};
typedef uint8_t high_score_table_type; /* Want 8 bits, not a 16 bit enum. */

/* One of these structures keeps track of each high score entry. */

typedef struct high_score_struct {
  uint16_t score;
  /* Total of the scores of all levels played.  16 bits (65535 max) should be
     enough; most levels are under 700 for a score so that's something like
     90 levels in a whole game, which would be too long a campaign. */

  uint8_t level_count;
  /* How many levels have been played in this game campaign to get the score.
     Useful for having the player quit early but still get a high score. */

  uint8_t win_count;
  /* Number of levels won in the campaign.  Can combine it with level_count to
     get a win percentage. */

  char name[MAX_SCORE_NAME_LENGTH+1];
  /* Name the player enters, blank padded, NUL at end, only ASCII printable
     characters (0x20 to 0x7F).  Default is the player colour.  AI players
     use colour+bot.  Longest would be "Yellow-Bot". */

  uint8_t editable_by_player;
  /* If this score is from the just finished game, this is the related player
     number.  Or set to MAX_PLAYERS if it is an old score.  This lets a
     particular user edit the name of this entry in the high score list while
     it is being displayed. */

  uint16_t year; /* Full year number, Common Era (CE or AD) dating system. */
  uint8_t month; /* 0 (January) to 11 (December). */
  uint8_t day; /* 1 to 31. */
  uint8_t hour; /* 0 to 23.  Probably just the local time zone. */
  uint8_t minute; /* 0 to 59. */
  /* When the score was achieved.  So you can see how long it existed. */

  /* The server side may store IP address and other things. */
} high_score_record, *high_score_pointer;


/* Local high scores are kept in memory, loaded when the program starts,
   written out to a file every time they change (so it will still work even if
   files don't work). */

extern high_score_record g_LocalHighScores[MAX_SCORE_TABLE_ENTRIES];

/* Given a single high score record in pNewScore, updates pScoreTable to
   include a copy of it if the score is high enough to be in the table.
   Returns TRUE if the table was changed, FALSE otherwise. */
extern bool MergeHighScore(high_score_pointer pNewScore,
  high_score_pointer pScoreTable);

/* Prints a score record to a text format in a buffer.  Returns a pointer to
   the NUL byte written at the end, or NULL if something went wrong. */
extern char * PrintHighScore(high_score_pointer pScore, char *pBuffer,
  uint8_t bufferSize);

/* Reads a high score from a file.  The format is:
   name in ASCII printable characters (0x20 to 0x7F), tab (0x09),
   score in base 10 ASCII digits, comma,
   level_count in base 10 ASCII digits, comma,
   win_count in base 10 ASCII digits, comma,
   year in base 10 ASCII digits (all of the year's digits), comma,
   month in base 10 ASCII digits (0 to 11), comma,
   day in base 10 ASCII digits (1 to 31), comma, 
   hour in base 10 ASCII digits (0 to 23), comma,
   minute in base 10 ASCII digits (0 to 59), comma,
   future other stuff ignored like IP address,
   line feed or NUL byte to mark end of record.
   Returns TRUE if it read something, FALSE at end of file.
   Note that editable_by_player is set to MAX_PLAYERS to turn off editing. */
extern bool ReadHighScore(char *pBuffer, high_score_pointer pScore);


/* Resets the goal score and forces a score display redraw on next update. */
extern void InitialiseScores(void);

/* Converts the player's scores into colourful text, cached in each player.
   Also update the goal text. */
extern void UpdateScores(void);

/* Update the screen display with the current scores.  They're the top line
   of the screen, showing each player's score in their colour, followed by the
   goal score to win. */
extern void CopyScoresToScreen(void);


#endif /* _SCORES_H */

