/******************************************************************************
 * Nth Pong Wars, players.c for player drawing and thinking.
 *
 * AGMS20241220 - Started this code file.
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

#define DEBUG_PRINT_OCTANTS 0 /* Turn on debug printfs, uses floating point. */

#include "players.h"
#include "scores.h"

#ifdef NABU_H
#include "Art/NthPong1.h" /* Artwork data definitions for player sprites. */

/* Predefined colour choices for the players, used on the NABU.  The palette is
   so limited that it's not worthwhile letting the player choose.  The first
   three are fairly distinct, though the fourth redish one is easily confused
   with others, which is why it's fourth. */

typedef struct colour_triplet_struct {
  uint8_t main;
  uint8_t shadow;
  uint8_t sparkle;
} colour_triplet_record;

static const colour_triplet_record k_PLAYER_COLOURS[MAX_PLAYERS] = {
  { VDP_MED_GREEN, VDP_DARK_GREEN, VDP_LIGHT_GREEN },
  { VDP_LIGHT_BLUE, VDP_DARK_BLUE, VDP_CYAN },
  { VDP_LIGHT_YELLOW, VDP_DARK_YELLOW, VDP_GRAY },
  { VDP_MED_RED, VDP_DARK_RED, VDP_LIGHT_RED}
};
#endif /* NABU_H */

player_record g_player_array[MAX_PLAYERS];

fx g_play_area_wall_bottom_y;
fx g_play_area_wall_left_x;
fx g_play_area_wall_right_x;
fx g_play_area_wall_top_y;

uint8_t g_KeyboardFakeJoystickStatus;
#ifndef NABU_H
  uint8_t g_JoystickStatus[4];
#endif


/* Set up the initial player data, mostly colours and animations. */
void InitialisePlayers(void)
{
  uint8_t iPlayer;
  uint16_t pixelCoord;
  player_pointer pPlayer;

  /* Cache the positions of the walls. */

  INT_TO_FX(g_play_area_height_pixels - PLAYER_PIXEL_DIAMETER_NORMAL / 2,
    g_play_area_wall_bottom_y);
  INT_TO_FX(PLAYER_PIXEL_DIAMETER_NORMAL / 2, g_play_area_wall_left_x);
  INT_TO_FX(g_play_area_width_pixels - PLAYER_PIXEL_DIAMETER_NORMAL / 2,
    g_play_area_wall_right_x);
  INT_TO_FX(PLAYER_PIXEL_DIAMETER_NORMAL / 2, g_play_area_wall_top_y);

#if 0
printf("Walls adjusted for player size:\n");
printf("(%f to %f, %f to %f)\n",
  GET_FX_FLOAT(g_play_area_wall_left_x),
  GET_FX_FLOAT(g_play_area_wall_right_x),
  GET_FX_FLOAT(g_play_area_wall_top_y),
  GET_FX_FLOAT(g_play_area_wall_bottom_y)
);
#endif

  pixelCoord = 32; /* Scatter players across screen. */
  for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
  iPlayer++, pPlayer++)
  {
    INT_TO_FX(pixelCoord, pPlayer->pixel_center_x);
    INT_TO_FX(pixelCoord, pPlayer->pixel_center_y);
    pixelCoord += 32;
    pPlayer->pixel_flying_height = 2;
    INT_TO_FX(iPlayer * 9, pPlayer->velocity_x);
    DIV4_FX(pPlayer->velocity_x, pPlayer->velocity_x);
    INT_TO_FX(1, pPlayer->velocity_y);

    pPlayer->brain = ((player_brain) BRAIN_KEYBOARD); /* Everybody at once! */
    pPlayer->main_colour =
#ifdef NABU_H
      k_PLAYER_COLOURS[iPlayer].main;
#else /* ncurses uses 1 to 4.  init_pair() sets it up, 0 for B&W text. */
      iPlayer + 1;
#endif

#ifdef NABU_H
    pPlayer->main_anim = g_SpriteAnimData[SPRITE_ANIM_BALL_ROLLING];
    /* Now just using VDP_BLACK pPlayer->shadow_colour = k_PLAYER_COLOURS[iPlayer].shadow; */
    pPlayer->sparkle_colour = k_PLAYER_COLOURS[iPlayer].sparkle;
    pPlayer->sparkle_anim = g_SpriteAnimData[SPRITE_ANIM_NONE];
#endif /* NABU_H */
  }
}


/* Add or remove points from a player's score.  Code actually part of player.c
   code, but we need the declaration in tiles.h for use by SetTileOwner.
*/
void AdjustPlayerScore(uint8_t iPlayer, int8_t score_change)
{
  g_player_array[iPlayer].score += score_change;
}


static void UpdateOneAnimation(SpriteAnimPointer pAnim)
{
  if (pAnim->current_delay == 0)
  {
    pAnim->current_delay = pAnim->delay;
    pAnim->current_name += 4; /* 4 characters per 16x16 sprite. */
    if (pAnim->current_name >= pAnim->past_last_name)
      pAnim->current_name = pAnim->first_name;
  }
  else
  {
    pAnim->current_delay--;
  }
}


/* Update animations for the next frame.  Also convert location of the player
   to hardware pixel coordinates. */
void UpdatePlayerAnimations(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;
  int16_t screenX;
  int16_t screenY;

  pPlayer = g_player_array;
  for (iPlayer = MAX_PLAYERS; iPlayer-- != 0; pPlayer++)
  {
    if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE))
    {
      UpdateOneAnimation(&pPlayer->main_anim);

#ifdef NABU_H
      if (pPlayer->sparkle_anim.type != ((SpriteAnimationType) SPRITE_ANIM_NONE))
        UpdateOneAnimation(&pPlayer->sparkle_anim);

      /* Update the sprite screen coordinates.  Convert from player coordinates.
         The Nabu has a 256x192 pixel screen and 16x16 sprites, VDP top of
         screen for sprites is (0,-1) and sprite coordinates are top left corner
         of the sprite.  Player coordinates are center of player ball, 8 pixels
         away from the top left of the 16x16 sprite:

         Position Type    Player X    Player Y    Sprite X   Sprite Y
         Off screen       -8          -8          -16        -17
         Partly on screen -7 to 7    -7 to 7      -15..-1    -16..-2
         On screen        8 to 248    8 to 184    0..240     -1..175
         Partly on screen 249 to 263  185 to 199  241..255   176..190
         Off screen       264         200         256        191
      */

      screenX = GET_FX_INTEGER(pPlayer->pixel_center_x);
      screenX += g_sprite_window_offset_x;

      screenY = GET_FX_INTEGER(pPlayer->pixel_center_y);
      screenY += g_sprite_window_offset_y;

      if (screenX <= -16 || screenY <= -17 || screenX >= 256 || screenY >= 191)
      { /* Sprite is off screen, flag it as not drawable. */
        pPlayer->vdpSpriteY = SPRITE_NOT_DRAWABLE;
      }
      else /* Partly or fully on screen. */
      {
        pPlayer->vdpSpriteY = screenY;
        if (screenX < 0) /* Hardware trick to do a few negative coordinates. */
        {
          pPlayer->vdpEarlyClock32Left = 0x80;
          pPlayer->vdpSpriteX = screenX + 32;
        }
        else
        {
          pPlayer->vdpEarlyClock32Left = 0;
          pPlayer->vdpSpriteX = screenX;
        }
      }

      /* Find the coordinates of the shadow sprite, under the ball.  Slightly
         offset to show height above the board.  Have to calculate it separately
         since it may be on screen while the ball is off screen.  Or maybe it's
         not drawn at all, if the ball is on the ground. */

      if (pPlayer->pixel_flying_height == 0)
        pPlayer->vdpShadowSpriteY = SPRITE_NOT_DRAWABLE;
      else
      {
        screenX += pPlayer->pixel_flying_height;
        screenY += pPlayer->pixel_flying_height;
        if (screenX <= -16 || screenY <= -17 || screenX >= 256 || screenY >= 191)
        { /* Sprite is off screen, flag it as not drawable. */
          pPlayer->vdpShadowSpriteY = SPRITE_NOT_DRAWABLE;
        }
        else /* Partly or fully on screen. */
        {
          pPlayer->vdpShadowSpriteY = screenY;
          if (screenX < 0)
          {
            pPlayer->vdpShadowEarlyClock32Left = 0x80;
            pPlayer->vdpShadowSpriteX = screenX + 32;
          }
          else
          {
            pPlayer->vdpShadowEarlyClock32Left = 0;
            pPlayer->vdpShadowSpriteX = screenX;
          }
        }
      }
#endif /* NABU_H */
    }
  }
}

/* Convert the current velocity values into an octant angle direction.
*/
static void UpdatePlayerVelocityDirection(player_pointer pPlayer)
{
  /* Find out which octant the velocity vector is currently in.  It will
     actually be between two octant directions, somewhere in a 45 degree
     segment.  So we'll come up with lower and upper angles, with the
     upper one being 45 degrees clockwise from the lower one.  If it is
     exactly on an octant angle, that will become the lower angle.

           6
       5  -y   7
     4  -x o +x  0
       3  +y   1
           2
  */

  int8_t xDir, yDir;
  uint8_t octantLower = 0xFF; /* An invalid value you should never see. */
  bool rightOnOctant = false; /* If velocity is exactly in octant direction. */

  xDir = TEST_FX(&pPlayer->velocity_x);
  yDir = TEST_FX(&pPlayer->velocity_y);

  if (xDir == 0) /* X == 0 */
  {
    if (yDir == 0) /* Y == 0 */
      octantLower = 0; /* Actually undefined, zero length vector. */
    else if (yDir >= 0) /* Y > 0, using >= test since it's faster. */
    {
      octantLower = 2;
      rightOnOctant = true;
    }
    else /* Y < 0 */
    {
      octantLower = 6;
      rightOnOctant = true;
    }
  }
  else if (xDir >= 0) /* X > 0 */
  {
    if (yDir == 0) /* X > 0, Y == 0 */
    {
      octantLower = 0;
      rightOnOctant = true;
    }
    else if (yDir >= 0) /* X > 0, Y > 0 */
    {
      int8_t xyDelta;
      xyDelta = COMPARE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
      if (xyDelta > 0) /* |X| > |Y| */
        octantLower = 0;
      else /* |X| <= |Y| */
      {
        octantLower = 1;
        rightOnOctant = (xyDelta == 0);
      }
    }
    else /* X > 0, Y < 0 */
    {
      int8_t xyDelta;
      fx negativeY;
      COPY_NEGATE_FX(&pPlayer->velocity_y, &negativeY);
      xyDelta = COMPARE_FX(&pPlayer->velocity_x, &negativeY);
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 7;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 6;
    }
  }
  else /* X < 0 */
  {
    if (yDir == 0) /* X < 0, Y == 0 */
    {
      octantLower = 4;
      rightOnOctant = true;
    }
    else if (yDir >= 0) /* X < 0, Y >= 0 */
    {
      int8_t xyDelta;
      fx negativeX;
      COPY_NEGATE_FX(&pPlayer->velocity_x, &negativeX);
      xyDelta = COMPARE_FX(&negativeX, &pPlayer->velocity_y);
      if (xyDelta >= 0) /* |X| >= |Y| */
      {
        octantLower = 3;
        rightOnOctant = (xyDelta == 0);
      }
      else /* |X| < |Y| */
        octantLower = 2;
    }
    else /* X < 0, Y < 0 */
    {
      int8_t xyDelta;
      xyDelta = COMPARE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
      if (xyDelta < 0) /* |X| > |Y| */
        octantLower = 4;
      else /* |X| <= |Y| */
      {
        octantLower = 5;
        rightOnOctant = (xyDelta == 0);
      }
    }
  }
  pPlayer->velocity_octant = octantLower;
  pPlayer->velocity_octant_right_on = rightOnOctant;
}


/* Rotate the player's velocity values towards an octant angle direction.
   Prerequisite: player's velocity_octant is up to date.
*/
static void TurnVelocityTowardsOctant(player_pointer pPlayer,
  uint8_t desired_octant)
{
  uint8_t head_towards_octant = 0;
  bool head_clockwise = false;
  bool heading_found = false;
  uint8_t currentLowerOctant, currentUpperOctant;
  currentLowerOctant = pPlayer->velocity_octant;
  currentUpperOctant = ((currentLowerOctant + 1) & 0x07);

#if DEBUG_PRINT_OCTANTS
printf("  CurrentLower %d, CurrentUpper %d, Desired %d.\n",
currentLowerOctant, currentUpperOctant, desired_octant);
#endif

  if (desired_octant == currentUpperOctant)
  {
    head_towards_octant = desired_octant;
    head_clockwise = true;
    heading_found = true;
  }
  else if (desired_octant == currentLowerOctant)
  {
    head_towards_octant = desired_octant;
    head_clockwise = false;
    heading_found = true;
  }
  else if (pPlayer->velocity_octant_right_on)
  {
    /* Widen the angles being considered, since we're exactly on
       currentLowerOctant and want to go past it to the next lower octant. */

    currentLowerOctant = ((currentLowerOctant - 1) & 0x07);
#if DEBUG_PRINT_OCTANTS
printf("  CurrentLower moved to %d.\n", currentLowerOctant);
#endif
    if (desired_octant == currentLowerOctant)
    {
      head_towards_octant = currentLowerOctant;
      head_clockwise = false;
      heading_found = true;
    }
  }

  /* Not currently near the desired octant, pick closest one. */

  if (!heading_found)
  {
    uint8_t lowerDelta, upperDelta;

    /* Counterclockwise distance from current lower to desired octant. */
    lowerDelta = ((currentLowerOctant - desired_octant) & 7);

    /* Clockwise rotation amount from current upper to desired octant. */
    upperDelta = ((desired_octant - currentUpperOctant) & 7);
#if DEBUG_PRINT_OCTANTS
printf("  LowerDelta %d, UpperDelta %d.\n", lowerDelta, upperDelta);
#endif
    if (lowerDelta < upperDelta)
    {
      head_towards_octant = currentLowerOctant;
      head_clockwise = false;
    }
    else
    {
      head_towards_octant = currentUpperOctant;
      head_clockwise = true;
    }
  }

#if DEBUG_PRINT_OCTANTS
printf("  Head towards octant %d, clockwise %d.\n",
head_towards_octant, head_clockwise);
#endif

  /* Set the maximum amount to move between axis.  0.5 pixels per update seems
     like a nice speed for doing a turn by steering. */
  fx amountToMove;
  INT_FRACTION_TO_FX(0, 0x8000, amountToMove);

  switch (head_towards_octant)
  {
    case 0:
      if (head_clockwise)
      { /* Reduce negative Y towards 0, add to positive X. */
        fx positiveY;
        COPY_NEGATE_FX(&pPlayer->velocity_y, &positiveY);
        if (COMPARE_FX(&positiveY, &amountToMove) < 0)
          COPY_FX(&positiveY, &amountToMove); /* Limited by available Y. */
        ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
      }
      else
      { /* Reduce positive Y towards 0, add to positive X. */
        if (COMPARE_FX(&pPlayer->velocity_y, &amountToMove) < 0)
          COPY_FX(&pPlayer->velocity_y, &amountToMove);
        SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
      }
      break;

    case 1:
      if (head_clockwise)
      { /* Reduce positive X, add to positive Y until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y|
             Note that we can't just subtract/add the half balance from both,
             since there could be rounding errors in the last bit. */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
          ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce positive Y, add to positive X until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
          ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        }
      }
      break;

    case 2:
      if (head_clockwise)
      { /* Reduce positive X towards 0, add to positive Y. */
        if (COMPARE_FX(&pPlayer->velocity_x, &amountToMove) < 0)
          COPY_FX(&pPlayer->velocity_x, &amountToMove);
        SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
      }
      else
      { /* Reduce negative X towards 0, add to positive Y. */
        fx positiveX;
        COPY_NEGATE_FX(&pPlayer->velocity_x, &positiveX);
        if (COMPARE_FX(&positiveX, &amountToMove) < 0)
          COPY_FX(&positiveX, &amountToMove);
        ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
      }
      break;

    case 3:
      if (head_clockwise)
      { /* Reduce positive Y, increase negative X until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_NEGATE_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        }
      }
      else
      { /* Reduce negative X, increase positive Y until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_NEGATE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
          ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        }
      }
      break;

    case 4:
      if (head_clockwise)
      { /* Reduce positive Y towards 0, increase negative X. */
        if (COMPARE_FX(&pPlayer->velocity_y, &amountToMove) < 0)
          COPY_FX(&pPlayer->velocity_y, &amountToMove);
        SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
      }
      else
      { /* Reduce negative Y towards 0, increase negative X. */
        fx positiveY;
        COPY_NEGATE_FX(&pPlayer->velocity_y, &positiveY);
        if (COMPARE_FX(&positiveY, &amountToMove) < 0)
          COPY_FX(&positiveY, &amountToMove);
        ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
      }
      break;

    case 5:
      if (head_clockwise)
      { /* Reduce negative X, increase negative Y until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
          SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce negative Y, increase negative X until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        }
      }
      break;

    case 6:
      if (head_clockwise)
      { /* Reduce negative X towards 0, increase negative Y. */
        fx positiveX;
        COPY_NEGATE_FX(&pPlayer->velocity_x, &positiveX);
        if (COMPARE_FX(&positiveX, &amountToMove) < 0)
          COPY_FX(&positiveX, &amountToMove);
        ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
      }
      else
      { /* Reduce positive X towards 0, increase negative Y. */
        if (COMPARE_FX(&pPlayer->velocity_x, &amountToMove) < 0)
          COPY_FX(&pPlayer->velocity_x, &amountToMove);
        SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
      }
      break;

    case 7:
      if (head_clockwise)
      { /* Reduce negative Y, increase positive X until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_NEGATE_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
          ADD_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce positive X, increase negative Y until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        if (COMPARE_FX(&balance, &amountToMove) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_NEGATE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &amountToMove, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &amountToMove, &pPlayer->velocity_x);
        }
      }
      break;

    default:
      break;
  }
}


/* Figure out what joystick action the player should do based on various
   algorithms.
*/
static void BrainUpdateJoystick(player_pointer pPlayer)
{
  pPlayer->joystick_inputs = 0;
}


/* Process the joystick and keyboard inputs.  Input activity assigns a player
   to the joystick or keyboard.  Also updates brains to generate fake joystick
   inputs if needed.  Then use the joystick inputs to modify the player's
   velocities (steering if just specifying a direction, thrusting if the fire
   button is pressed too).
*/
void UpdatePlayerInputs(void)
{
  uint8_t iInput;
  uint8_t iPlayer;
  player_pointer pPlayer;
  static bool input_consumed[5]; /* Joysticks [0] to [3], Keyboard is [4]. */

  /* Update the player's control inputs with joystick or keyboard input from
     the Human players or AI input.  Also keep track of which inputs we used. */

  for (iInput = 0; iInput < sizeof (input_consumed); iInput++)
    input_consumed[iInput] = false;

  /* Copy inputs to player records.  For AI players, generate the inputs. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == ((player_brain) BRAIN_INACTIVE))
      continue;

    if (pPlayer->brain == ((player_brain) BRAIN_KEYBOARD))
    {
      pPlayer->joystick_inputs = g_KeyboardFakeJoystickStatus;
      input_consumed[4] = true;
      continue;
    }

    if (pPlayer->brain == ((player_brain) BRAIN_JOYSTICK))
    {
      uint8_t iStick;
      iStick = pPlayer->brain_info.iJoystick;
      input_consumed[iStick] = true;
      /* Note Nabu has high bits always set in joystick data, we want zero for
         no input from the user. */
      pPlayer->joystick_inputs = (g_JoystickStatus[iStick] & 0x1F);
      continue;
    }

    /* One of the fancier brains or a remote player, do fancy update. */
    BrainUpdateJoystick(pPlayer);
  }

  /* Look for unconsumed active inputs, and assign an idle player to them. */

  for (iInput = 0; iInput < sizeof (input_consumed); iInput++)
  {
    uint8_t joyStickData;

    if (input_consumed[iInput])
      continue; /* Input already used up by an assigned player. */

    if (iInput < 4)
      joyStickData = g_JoystickStatus[iInput];
    else
      joyStickData = g_KeyboardFakeJoystickStatus;
    if ((joyStickData & 0x1F) == 0)
      continue; /* No buttons pressed, no activity. */

    /* Find an idle player and assign it to this input. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE))
        continue;

      pPlayer->brain_info.iJoystick = iInput;
      pPlayer->brain = (iInput < 4) ?
        ((player_brain) BRAIN_JOYSTICK) :
        ((player_brain) BRAIN_KEYBOARD);
      pPlayer->joystick_inputs = joyStickData;
#if 0
printf("Player %d assigned to %s #%d.\n", iPlayer,
  (pPlayer->brain == ((player_brain) BRAIN_JOYSTICK)) ? "joystick" : "keyboard",
  pPlayer->brain_info.iJoystick);
#endif
      break;
    }
  }

  /* Process the player actions.  Do idle timeout for inactive players. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    uint8_t joyStickData;

    if (pPlayer->brain == ((player_brain) BRAIN_INACTIVE))
      continue;

    /* If idle too long (30 seconds), deactivate the player. */

    joyStickData = pPlayer->joystick_inputs;
    if (joyStickData)
      pPlayer->last_brain_activity_time = g_FrameCounter;
    else if (g_FrameCounter - pPlayer->last_brain_activity_time > 30 * 30)
    {
      pPlayer->brain = ((player_brain) BRAIN_INACTIVE);
      continue;
    }

    /* Apply joystick actions.  Or just drift if not specifying a direction
       or using the fire button. */

    pPlayer->thrust_active = false;
    if (joyStickData)
    {
      /* If thrusted in the last frame and harvested some points, speed up in
         the joystick direction.  Number of tiles harvested is converted to a
         fraction of pixel per update velocity (else things go way too fast). */

      if (pPlayer->thrust_harvested)
      {
        fx thrustAmount;
        INT_FRACTION_TO_FX(0, pPlayer->thrust_harvested * (int) 1024,
          thrustAmount);

        if (joyStickData & Joy_Left)
          SUBTRACT_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);
        if (joyStickData & Joy_Right)
          ADD_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);
        if (joyStickData & Joy_Up)
          SUBTRACT_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);
        if (joyStickData & Joy_Down)
          ADD_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);
      }

      if (joyStickData & Joy_Button)
      { /* Fire button starts harvesting mode, need direction too. */
        if ((joyStickData & (Joy_Left | Joy_Down | Joy_Right | Joy_Up)))
          pPlayer->thrust_active = true; /* Harvest tiles we pass over. */
      }
      else /* Fire not pressed, just steer by rotating velocity direction. */
      {
        /* Convert the joystick inputs to a direction in octants. */

        uint8_t desired_octant = 0;
        if (joyStickData & Joy_Left)
        {
          if (joyStickData & Joy_Up)
            desired_octant = 5;
          else if (joyStickData & Joy_Down)
            desired_octant = 3;
          else
            desired_octant = 4;
        }
        else if (joyStickData & Joy_Right)
        {
          if (joyStickData & Joy_Up)
            desired_octant = 7;
          else if (joyStickData & Joy_Down)
            desired_octant = 1;
          else
            desired_octant = 0;
        }
        else if (joyStickData & Joy_Up)
            desired_octant = 6;
        else if (joyStickData & Joy_Down)
            desired_octant = 2;

        UpdatePlayerVelocityDirection(pPlayer);
#if DEBUG_PRINT_OCTANTS
printf("Player %d Velocity (%f,%f) octant %d, right on %d, desire %d.\n",
iPlayer,
GET_FX_FLOAT(pPlayer->velocity_x), GET_FX_FLOAT(pPlayer->velocity_y),
pPlayer->velocity_octant, pPlayer->velocity_octant_right_on, desired_octant);
#endif

        TurnVelocityTowardsOctant(pPlayer, desired_octant);
#if DEBUG_PRINT_OCTANTS
printf("  Turned velocity (%f,%f).\n",
GET_FX_FLOAT(pPlayer->velocity_x), GET_FX_FLOAT(pPlayer->velocity_y));
#endif
      }
    }

#if 1
    /* Add some friction, to reduce excessive velocity which slows the frame
       rate and is uncontrollable for the user.  But only if going so fast that
       it would make the physics run multiple sub-steps and thus slow the frame
       rate down.  Don't need absolute value, it will bounce off a wall soon
       if it's going that fast. */

    if (GET_FX_INTEGER(pPlayer->velocity_x) >= 2 ||
    GET_FX_INTEGER(pPlayer->velocity_y) >= 2)
    {
      fx portionOfVelocity;
      DIV256_FX(pPlayer->velocity_x, portionOfVelocity);
      SUBTRACT_FX(&pPlayer->velocity_x, &portionOfVelocity, &pPlayer->velocity_x);
      DIV256_FX(pPlayer->velocity_y, portionOfVelocity);
      SUBTRACT_FX(&pPlayer->velocity_y, &portionOfVelocity, &pPlayer->velocity_y);
    }
#endif

    /* Update special effect animations and other things. */
    
    /* Thrust animation is shown when some acceleration points are harvested.
       But we only get thrust points every other frame as it oscillates while
       sitting over a tile creating then destroying ownership (unless it's
       moving really fast).  Show the animation when the buttons are held
       down, and force it to be static (not advancing) if no harvest. */

    SpriteAnimationType newAnimType;
    newAnimType = pPlayer->thrust_active ? 
      (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_THRUST :
      (SpriteAnimationType) SPRITE_ANIM_NONE;
    if (newAnimType != pPlayer->sparkle_anim.type)
      pPlayer->sparkle_anim = g_SpriteAnimData[newAnimType];
    else /* Current animation is continuing to play. */
    {
      if (newAnimType == (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_THRUST &&
      pPlayer->thrust_harvested < 4)
        pPlayer->sparkle_anim.current_delay++; /* Stop the clock ticking. */
    }
  }
}


#ifdef NABU_H
/* Copy all the players to hardware sprites.  Returns the number of the next
   free sprite.  Inactive players don't use any sprites.  Also inactive sparkle
   animations don't use a sprite.  They are copied in priority order, all balls,
   then sparkles, then shadows.  Should be the first thing updated as the
   vertical blanking starts, since sprites updated during display look bad.
*/
void CopyPlayersToSprites(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;

  vdp_setWriteAddress(_vdpSpriteAttributeTableAddr);

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE) &&
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

  /* Do the sparkle sprites. */

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE) &&
    pPlayer->sparkle_anim.type != ((SpriteAnimationType) SPRITE_ANIM_NONE) &&
    pPlayer->vdpSpriteY != SPRITE_NOT_DRAWABLE)
    {
      /* Do the sparkle power-up sprite.  Same location as ball. */
      IO_VDPDATA = pPlayer->vdpSpriteY;
      IO_VDPDATA = pPlayer->vdpSpriteX;
      IO_VDPDATA = pPlayer->sparkle_anim.current_name;
      IO_VDPDATA = pPlayer->vdpEarlyClock32Left | pPlayer->sparkle_colour;
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  /* Do the shadow sprites.  Same shape as ball sprite, but different colour
     and slightly offset to show height above ground. */

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE) &&
    pPlayer->vdpShadowSpriteY != SPRITE_NOT_DRAWABLE)
    {
      IO_VDPDATA = pPlayer->vdpShadowSpriteY;
      IO_VDPDATA = pPlayer->vdpShadowSpriteX;
      IO_VDPDATA = pPlayer->main_anim.current_name;
      IO_VDPDATA = pPlayer->vdpShadowEarlyClock32Left | VDP_BLACK;
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  /* No more sprites to draw, terminate sprite list for the VDP with magic
     vertical position of 208 or $D0. */
  IO_VDPDATA = 0xD0;
}
#endif /* NABU_H */

