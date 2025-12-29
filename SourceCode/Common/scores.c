/******************************************************************************
 * Nth Pong Wars, scores.c for keeping track of scoring.
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
  uint8_t iPlayer;
  player_pointer pPlayer;

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    pPlayer->score_displayed = 0x4321; /* Force a display update next time. */
  }

  g_ScoreGoal = gVictoryStartingTileCount;
  s_ScoreGoalDisplayed = 0xFEDC; /* Force a display update next time. */

  s_ScoreFramesPerUpdateDisplayed = 255; /* Force an update. */
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
    uint16_t score = GetPlayerScore(iPlayer);

    if (score != pPlayer->score_displayed)
    {
      /* Prepare a RAM copy of a colourful score number or other info. */

      Write3DigitColourfulNumber(score, pPlayer->score_text,
        fontOffset /* Font offset to a colour digit set */);
    }

    /* See if the score is getting close to the goal, then turn on faster
       flashing of the lozenge after the score display. */

    uint16_t deltaScore = g_ScoreGoal - score;
    if (deltaScore < 64)
    {
      /* Bit mask uses a lower bit the closer the player is to the goal, so
         it will flash faster when used to mask test the frame counter. */
      pPlayer->score_getting_close_mask = (0x08 >>
        (3 - ((uint8_t) deltaScore) / (uint8_t) 16)); /* 0 to 3 shifts. */
    }
    else
      pPlayer->score_getting_close_mask = 0;

    fontOffset += 11; /* Next batch of colourful digits and % sign. */
  }

  if (s_ScoreGoalDisplayed != g_ScoreGoal)
    Write3DigitColourfulNumber(g_ScoreGoal, s_ScoreGoalText, 0);
}


/* Update the screen display with the current scores.  They're the top line
   of the screen, snowing each player's score in their colour, followed by the
   goal score to win.
*/
void CopyScoresToScreen(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    char *pChar;
    char letter;
    uint16_t score = GetPlayerScore(iPlayer);
    bool addressSet = false;

    if (score != pPlayer->score_displayed)
    {
      pPlayer->score_displayed = score; /* Now up to date. */

      vdp_setWriteAddress(_vdpPatternNameTableAddr + iPlayer * 4);
      addressSet = true;
      for (pChar = pPlayer->score_text; (letter = *pChar) != 0; pChar++)
        IO_VDPDATA = letter;
    }

    char newLozenge = ' ';
    if (pPlayer->score_getting_close_mask &&
    (g_FrameCounter & pPlayer->score_getting_close_mask))
      newLozenge = 80 + ':' + 11 * iPlayer;

    if (newLozenge != pPlayer->lozenge_text)
    {
      pPlayer->lozenge_text = newLozenge;
      if (!addressSet)
        vdp_setWriteAddress(_vdpPatternNameTableAddr + 3 + iPlayer * 4);
      IO_VDPDATA = newLozenge;
    }
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
    IO_VDPDATA = g_ScoreFramesPerUpdate + ('A' - 1);
  }
#endif

  /* Draw the frame counter every time. */
  {
    char *pChar;
    char letter;

    strcpy(g_TempBuffer, "0000"); /* 4 leading zeroes. */
    pChar = fast_utoa(g_FrameCounter, g_TempBuffer + 4) - 4; /* Last four digits. */
    vdp_setWriteAddress(_vdpPatternNameTableAddr + 28);
    for (; (letter = *pChar) != 0; pChar++)
      IO_VDPDATA = letter;
  }

#if 0
  /* Debug which audio effect channels are busy playing (music is always
     playing), display ABC on second line, or blank if that channel is quiet. */

  vdp_setWriteAddress(_vdpPatternNameTableAddr + 32);
  uint8_t iChannel;
  for (iChannel = 0; iChannel < 3; iChannel++)
  {
    char letter = ' ';
    if (CSFX_busy(iChannel))
      letter = 'A' + iChannel;
    IO_VDPDATA = letter;
  }
#endif
}

