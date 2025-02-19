/******************************************************************************
 * Nth Pong Wars, scores.c for keeping track of scoring.
 */

#include "scores.h"
#include "tiles.h"

/* Our globals. */

uint16_t g_FrameCounter;
uint16_t g_ScoreGoal;
uint8_t g_ScoreFramesPerUpdate;

static char s_ScoreGoalText[8];
static uint16_t s_ScoreGoalDisplayed;
/* Score as last drawn on the screen.  Only need to redraw if changed. */

static uint8_t s_ScoreFramesPerUpdateDisplayed;
/* Previously displayed value, so we can skip drawing unchanged update codes. */


/* Resets the goal (based on the number of tiles in the play area), recomputes
   the score for each player based on the state of the tiles in the play area,
   in case we want to reload a saved game or have pre-made scenarios.
*/
void InitialiseScores(void)
{
  uint8_t col, row;
  uint8_t iPlayer;
  player_pointer pPlayer;
  tile_pointer pTile;

  g_FrameCounter = 0;

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    pPlayer->score = 0;
    pPlayer->score_displayed = 0x4321; /* Force a display update next time. */
  }

  g_ScoreGoal = g_play_area_num_tiles;
  s_ScoreGoalDisplayed = 0xFEDC; /* Force a display update next time. */

  s_ScoreFramesPerUpdateDisplayed = 255; /* Force an update. */

  /* Count up the tiles to get the starting score for each player. */

  for (row = 0; row != g_play_area_height_tiles; row++)
  {
    pTile = g_tile_array_row_starts[row];
    if (pTile == NULL)
      break;
    for (col = 0; col != g_play_area_width_tiles; col++, pTile++)
    {
      tile_owner owner;
      owner = pTile->owner;
      if (owner < OWNER_PLAYER_1 || owner > OWNER_PLAYER_4)
        continue;
      iPlayer = owner - OWNER_PLAYER_1;
      g_player_array[iPlayer].score += 1;
    }
  }
}


/* Convert a binary 16 bit number to 3 digits and write to the given
   destination string.  The fontOffset is added to each ASCII code to get
   colourful digits from the game font.  Returns end of string pointer.
*/
static char * Write3DigitColourfulNumber(
  uint16_t number, char *pDest, uint8_t fontOffset)
{
  char numberText[10]; /* 16 bits, largest is "65535", + 3 zeroes. */
  char *pSource;

  strcpy(numberText, "000"); /* Leading zeroes. */
  pSource = fast_utoa(number, numberText + 3);
  pSource -= 3; /* Back up 3 characters from the end. */
  for (; *pSource != 0; pDest++, pSource++)
  {
    *pDest = fontOffset + *pSource;
  }
  *pDest = 0;
  return pDest;
}


/* Converts the player's scores into colourful text, cached in each player.
   Also update the goal text.
*/
void UpdateScores(void)
{
  uint8_t fontOffset;
  uint8_t iPlayer;
  player_pointer pPlayer;

  fontOffset = 80; /* Difference between ASCII '0' and first colourful 0. */
  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->score != pPlayer->score_displayed)
    {
      if (pPlayer->brain == BRAIN_INACTIVE)
      {
        /* Just display some dots in the player's colour. */
        memset(pPlayer->score_text, '0' + 10 + fontOffset, 3);
        pPlayer->score_text[3] = 0;
      }
      else /* Display a colourful score number. */
      {
        Write3DigitColourfulNumber(pPlayer->score, pPlayer->score_text,
          fontOffset /* Font offset to a colour digit set */);
      }
    }
    fontOffset += 11; /* Next batch of colourful digits and % sign. */
  }

  if (s_ScoreGoalDisplayed != g_ScoreGoal)
    Write3DigitColourfulNumber(g_ScoreGoal, s_ScoreGoalText, 0);
}


/* Update the screen display with the current scores.  They're the top line
   of the screen, snowing each player's score in their colour, followed by the
   goal score to win. */
void CopyScoresToScreen(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    char *pChar;
    char letter;

    if (pPlayer->score == pPlayer->score_displayed)
      continue;
    pPlayer->score_displayed = pPlayer->score; /* Now up to date. */

    vdp_setWriteAddress(_vdpPatternNameTableAddr + iPlayer * 4);
    for (pChar = pPlayer->score_text; (letter = *pChar) != 0; pChar++)
      IO_VDPDATA = letter;
  }

  if (s_ScoreGoalDisplayed != g_ScoreGoal)
  {
    char *pChar;
    char letter;

    s_ScoreGoalDisplayed = g_ScoreGoal; /* Up to date. */

    vdp_setWriteAddress(_vdpPatternNameTableAddr + 21);
    for (pChar = s_ScoreGoalText; (letter = *pChar) != 0; pChar++)
      IO_VDPDATA = letter;
  }

#if 1
  /* Display the frames per update code letter on the screen if needed,
     Normally will be FB, but FA if faster, FC if slower than the typical 2
     frames per update (1 missed frame typical). */

  if (s_ScoreFramesPerUpdateDisplayed != g_ScoreFramesPerUpdate)
  {
    vdp_setWriteAddress(_vdpPatternNameTableAddr + 26);
    s_ScoreFramesPerUpdateDisplayed = g_ScoreFramesPerUpdate;
    IO_VDPDATA = g_ScoreFramesPerUpdate + 'A';
  }

  /* Draw the frame counter every time. */
  {
    char *pChar;
    char letter;

    strcpy(TempBuffer, "0000"); /* 4 leading zeroes. */
    pChar = fast_utoa(g_FrameCounter, TempBuffer + 4) - 4; /* Last four digits. */
    vdp_setWriteAddress(_vdpPatternNameTableAddr + 28);
    for (; (letter = *pChar) != 0; pChar++)
      IO_VDPDATA = letter;
  }
#endif
}

