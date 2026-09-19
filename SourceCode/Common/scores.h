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

/* The current date and time in "yyyy.MM.dd HH:mm" format, plus a NUL at end.
   Updated when the level ends as part of the high score table updating. */
#define MAX_SCORE_DATE_LENGTH 16
extern char g_CurrentScoreDateTime[MAX_SCORE_DATE_LENGTH+1];

/* Resets the goal score and forces a score display redraw on next update. */
extern void InitialiseScores(void);

/* Converts the player's scores into colourful text, cached in each player.
   Also update the goal text. */
extern void UpdateScores(void);

/* Update the screen display with the current scores.  They're the top line
   of the screen, showing each player's score in their colour, followed by the
   goal score to win. */
extern void CopyScoresToScreen(void);

/* Convert a binary 16 bit number to N digits (max 5) and write to the given
   destination string.  The fontOffset is added to each ASCII code to get
   colourful digits from the game font.  If nDigits is zero then it doesn't do
   leading zeroes and the string is as long as it is.  Returns end of string
   pointer. */
extern char * WriteNDigitColourfulNumber(
  uint16_t number, uint8_t nDigits, char *pDest, uint8_t fontOffset);


/******************************************************************************
 * For keeping track of high scores, locally and world wide over the
 * Internet.  The global ones can have daily, weekly, monthly, yearly and
 * all-time high score lists.
 */

#define MAX_SCORE_NAME_LENGTH 12 /* For "Yellow-bot" or "AGMS20260917". */
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
extern const char *g_TableTypeNames[HIGH_SCORE_TABLE_MAX]; /* Enum to string. */

/* One of these structures keeps track of each high score entry. */

typedef struct high_score_struct {
  uint16_t score;
  /* Total of the scores of all levels played.  16 bits (65535 max) should be
     enough; most levels are under 700 for a score so that's something like
     90 levels in a whole game, which would be too long a campaign. */

  uint8_t win_count;
  /* Number of levels won in the campaign.  Can combine it with level_count to
     get a win percentage. */

  uint8_t level_count;
  /* How many levels have been played in this game campaign to get the score.
     Useful for having the player quit early but still get a high score. */

  char name[MAX_SCORE_NAME_LENGTH+1];
  /* Name the player enters, blank padded, NUL at end, only ASCII printable
     characters (0x20 to 0x7F).  Default is the player colour.  AI players
     use colour+bot.  Longest would be "Yellow-Bot". */

  uint8_t editable_by_player;
  /* If this score is from the just finished game, this is the related player
     number.  Or set to MAX_PLAYERS if it is an old score.  This lets a
     particular user edit the name of this entry in the high score list while
     it is being displayed. */

  char date_of_score[MAX_SCORE_DATE_LENGTH+1];
  /* The date the score was achieved in local time (whatever that is).  Will
     be of the format "yyyy.MM.dd HH:mm" followed by a NUL character.  Not
     broken down into separate numbers to save on code space. */

  /* The server side may store IP address and other things. */
} high_score_record, *high_score_pointer;


/* Local high scores are kept in memory, loaded when the program starts,
   written out to a file every time they change (so it will still work even if
   files don't work).  In order of decreasing score. */
extern high_score_record g_LocalHighScores[MAX_SCORE_TABLE_ENTRIES];

/* Global high scores from the server are temporarily loaded into memory in
   this [MAX_SCORE_TABLE_ENTRIES] array, which overwrites the g_tile_array tile
   data to save space (no tiles when displaying high scores).  NULL when no
   data is available.  g_LoadedScoreTableType specifies what kind of table was
   last loaded. */
extern high_score_pointer g_LoadedHighScores;
extern high_score_table_type g_LoadedScoreTableType;

/* Given a single high score record in pNewScore, updates scoreTable to
   include a copy of it if the score is high enough to be in the table.
   Returns TRUE if the table was changed, FALSE otherwise. */
extern bool MergeHighScore(high_score_pointer pNewScore,
  high_score_record scoreTable[MAX_SCORE_TABLE_ENTRIES]);

/* Reads a high score table from the given data source (local file or global
   network server) into the specified table, replacing its contents.  Returns
   TRUE if something was read, FALSE if no data was read.  Actual code is in
   levels.c since that's where our file handling functions are. */
extern bool ReadHighScoreTable(high_score_table_type table_type,
  high_score_record scoreTable[MAX_SCORE_TABLE_ENTRIES]);

/* Writes the given data to the data storage system implied by the high score
   type.  Local file for local scores, global server for other scores.  Returns
   FALSE if something went wrong.  Actual code is in levels.c since that's
   where our file handling functions are. */
extern bool WriteHighScoreTable(high_score_table_type table_type,
  high_score_record scoreTable[MAX_SCORE_TABLE_ENTRIES]);

/* A level has just finished.  Update the various score counts and add them to
   the local high score table if they qualify, and write out the local table.
   Doesn't ask players to enter names etc, that's the job of a special level at
   the end of the game.  Returns TRUE if there is a new high score. */
extern bool UpdateHighScoresForLevelFinished(void);

#endif /* _SCORES_H */

