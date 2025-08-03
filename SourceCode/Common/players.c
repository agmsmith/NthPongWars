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
#include "sounds.h"

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

int16_t g_play_area_wall_bottom_y;
int16_t g_play_area_wall_left_x;
int16_t g_play_area_wall_right_x;
int16_t g_play_area_wall_top_y;

fx g_SeparationVelocityFxAdd;
fx g_SeparationVelocityFxStepAdd;

uint8_t g_KeyboardFakeJoystickStatus;
#ifndef NABU_H
  uint8_t g_JoystickStatus[4];
#endif

/* List of column/row locations for AIs to move towards, and some instructions,
   has a byte sized index in the opcodes so the array is currently limited to
   at most 256 elements. */
target_list_item_record g_target_list[] = {
  {8, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {15, TARGET_CODE_DELAY}, /* Delay 3 seconds to hang around there. */
  {9, TARGET_CODE_STEER}, /* Steer to next corner. */
  {15, TARGET_CODE_DELAY}, /* Delay 3 seconds to hang around there. */
  {8, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {1, TARGET_CODE_SPEED}, /* Avoid accelerating and eating tiles. */
  {255, TARGET_CODE_STEER}, /* Drift mode, no direction specified. */
  {50, TARGET_CODE_DELAY}, /* Delay 10 seconds to drift. */
  {20, TARGET_CODE_SPEED}, /* Fast movement towards player. */
  {12, TARGET_CODE_STEER}, /* Head to leading player. */
  {15, TARGET_CODE_DELAY}, /* Delay 3 seconds to hound them. */
  {30, TARGET_CODE_SPEED}, /* Faster movement back to center. */
  {16, 11}, /* Head to center of screen. */
  {8, TARGET_CODE_SPEED}, /* Normal speed, avoids physics slowdown. */
  {0, TARGET_CODE_GOTO},
};


/* Set up the initial player data, mostly colours and animations. */
void InitialisePlayers(void)
{
  uint8_t iPlayer;
  uint16_t pixelCoord;
  player_pointer pPlayer;

  /* Cache the positions of the walls. */

  g_play_area_wall_bottom_y = g_play_area_height_pixels -
    PLAYER_PIXEL_DIAMETER_NORMAL / 2; 
  g_play_area_wall_left_x = PLAYER_PIXEL_DIAMETER_NORMAL / 2; 
  g_play_area_wall_right_x = g_play_area_width_pixels -
    PLAYER_PIXEL_DIAMETER_NORMAL / 2;
  g_play_area_wall_top_y = PLAYER_PIXEL_DIAMETER_NORMAL / 2;

#if 0
printf("Walls adjusted for player size:\n");
printf("(%d to %d, %d to %d)\n",
  g_play_area_wall_left_x,
  g_play_area_wall_right_x,
  g_play_area_wall_top_y,
  g_play_area_wall_bottom_y
);
#endif

  /* Set the constant velocity change used for separating collided players. */

  INT_TO_FX(1, g_SeparationVelocityFxAdd);

  /* Set up the players. */

  pixelCoord = 32; /* Scatter players across screen. */
  for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
  iPlayer++, pPlayer++)
  {
    pPlayer->player_array_index = iPlayer; /* Fast convert pointer to index. */

    INT_TO_FX(pixelCoord, pPlayer->pixel_center_x);
    INT_TO_FX(pixelCoord, pPlayer->pixel_center_y);
    pixelCoord += 32;
    pPlayer->pixel_flying_height = 2;
    INT_FRACTION_TO_FX(0, iPlayer * 256, pPlayer->velocity_x);
    DIV4_FX(pPlayer->velocity_x, pPlayer->velocity_x);
    INT_FRACTION_TO_FX(0, 0x1000, pPlayer->velocity_y);
    pPlayer->player_collision_count = 0;

    /* Make them all AI players, since you can take over from AI easily. */

    bzero(&pPlayer->brain_info, sizeof(pPlayer->brain_info));
    pPlayer->brain = (player_brain) BRAIN_ALGORITHM;
    pPlayer->brain_info.algo.desired_speed = 4 + iPlayer;
    pPlayer->brain_info.algo.trail_remaining = 50 + iPlayer * 8;
    pPlayer->brain_info.algo.target_list_index = 0;
    pPlayer->brain_info.algo.steer = false;
    pPlayer->brain_info.algo.target_player = 10;

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


/* Figure out the joystick bits which are needed to turn towards a given
   octant angle.
*/
static uint8_t JoystickForOctant(uint8_t desired_octant)
{
  uint8_t joystickOutput = 0;

  if (desired_octant <= 1 || desired_octant == 7)
    joystickOutput |= Joy_Right;
  else if (desired_octant >= 3 && desired_octant <= 5)
    joystickOutput |= Joy_Left;
  if (desired_octant >= 1 && desired_octant <= 3)
    joystickOutput |= Joy_Down;
  else if (desired_octant >= 5)
    joystickOutput |= Joy_Up;
  return joystickOutput;
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

  /* Set the maximum amount to move between axis, larger makes for faster
     rotations. */

  fx amountToMove;
#if 1 /* Use 1.5 pixels per turn update, makes for wide turns at high speeds. */
  INT_FRACTION_TO_FX(1, 0x8000, amountToMove);
#else /* More precise control, use average velocity for turning speed. */
  {
    COPY_FX(&pPlayer->velocity_x, &amountToMove);
    ABS_FX(&amountToMove);
    if (IS_NEGATIVE_FX(&pPlayer->velocity_y))
      SUBTRACT_FX(&amountToMove, &pPlayer->velocity_y, &amountToMove);
    else
      ADD_FX(&amountToMove, &pPlayer->velocity_y, &amountToMove);
    DIV2_FX(&amountToMove);
#if 0 /* For slightly slower turns with slightly more turning radius. */
    DIV2_FX(&amountToMove);
#endif
  }
#endif

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


/* Figure out what joystick action the AI player should do based on various
   algorithms.
*/
static void BrainUpdateJoystick(player_pointer pPlayer)
{
  uint8_t joystickOutput = 0;

  /* Do the harvest mode vs trailing tiles mode based on time duty cycle that
     gets shorter if we need more speed (using the technique of laying down a
     tile and then harvesting it in the next frame, rather than the bigger
     picture technique of heading towards larger areas of your tiles). */

  if (--pPlayer->brain_info.algo.trail_remaining == 0)
  {
    /* More rapidly alternate between harvest and lay down tiles if more
       speed is needed.  If speed not needed, go for longer stretches of
       laying down tiles. */
    bool wasHarvesting = (pPlayer->joystick_inputs & Joy_Button) != 0;
    int8_t deltaSpeed = /* Positive if more speed needed. */
      (int8_t) pPlayer->brain_info.algo.desired_speed -
      (int8_t) pPlayer->speed;
    uint8_t time_to_trail; /* New time limit for the next harvest/trail mode. */

    if (deltaSpeed <= 0)
    {
      if (wasHarvesting)
        time_to_trail = 10; /* 10 == 2 seconds. */
      else /* Next mode will be harvest, keep it short. */
        time_to_trail = 1;
    }
    else if (deltaSpeed > 19)
      time_to_trail = 1; /* Alternate trail and harvest every update. */
    else /* 0 to 19 deltaSpeed. */
      time_to_trail = 20 - deltaSpeed;
    pPlayer->brain_info.algo.trail_remaining = time_to_trail;

    /* Flip Joy_Button bit of joystickOutput to flip harvest mode. */
    if (!wasHarvesting) /* Should we harvest in the next cycle? */
      joystickOutput |= Joy_Button;
  }

  /* Has the current opcode finished?  Or is it still waiting? */

  if (pPlayer->brain_info.algo.delay_remaining != 0)
  { /* Keep on waiting until the countdown has finished. */
    pPlayer->brain_info.algo.delay_remaining--;
  }
  else if (pPlayer->brain_info.algo.target_distance >=
  PLAYER_PIXEL_DIAMETER_NORMAL)
  {
    /* Not yet close enough to the target.  Continue hunting. */
  }
  else
  { /* No more waiting, at target and delay has finished, do current opcode
       actions and point to next one for next time. */

    target_list_item_record currentOpcode;
    currentOpcode = g_target_list[pPlayer->brain_info.algo.target_list_index++];

    switch (currentOpcode.target_pixel_y)
    {
    case TARGET_CODE_SPEED:
      pPlayer->brain_info.algo.desired_speed = currentOpcode.target_pixel_x;
      break;

    case TARGET_CODE_STEER:
      /* X coordinate is 0 to 7 to steer towards a given quadrant in drift
         mode, 8 to 11 to steer to your corner of the board or next player's
         corner for higher numbers, 12 to target a rival player with the
         highest score, other values make you drift rather than steer. */
      if (currentOpcode.target_pixel_x <= 7)
      { /* Steer in the given octant direction.  Next opcode usually delays. */
        joystickOutput = ((joystickOutput & Joy_Button) |
          JoystickForOctant(currentOpcode.target_pixel_x));
        pPlayer->brain_info.algo.steer = false; /* Just drift. */
      }
      else if (currentOpcode.target_pixel_x >= 8 &&
      currentOpcode.target_pixel_x <= 11)
      { /* Target your own corner of the game board, or the next ones around. */
        uint8_t iMyself = (3 &
          (pPlayer->player_array_index + currentOpcode.target_pixel_x));
        if (iMyself == 0 || iMyself == 3)
          pPlayer->brain_info.algo.target_pixel_x = TILE_PIXEL_WIDTH / 2;
        else
          pPlayer->brain_info.algo.target_pixel_x =
            g_play_area_width_tiles * (int16_t) TILE_PIXEL_WIDTH -
            TILE_PIXEL_WIDTH / 2;
        if (iMyself == 0 || iMyself == 1)
          pPlayer->brain_info.algo.target_pixel_y = TILE_PIXEL_WIDTH / 2;
        else
          pPlayer->brain_info.algo.target_pixel_y =
            g_play_area_height_tiles * (int16_t) TILE_PIXEL_WIDTH -
            TILE_PIXEL_WIDTH / 2;
        pPlayer->brain_info.algo.target_player = MAX_PLAYERS;
        pPlayer->brain_info.algo.steer = true;
      }
      else if (currentOpcode.target_pixel_x == 12)
      { /* Target the rival player with highest score. */
        uint8_t iMyself = pPlayer->player_array_index;
        uint8_t iBestPlayer = MAX_PLAYERS;
        uint16_t maxScore = 0;
        uint8_t iPlayer;
        for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
        {
          if (iPlayer == iMyself)
            continue;
          uint16_t theirScore = GetPlayerScore(iPlayer);
          if (theirScore >= maxScore) /* So when scores are zero, it works. */
          {
            maxScore = theirScore;
            iBestPlayer = iPlayer;
          }
        }
        pPlayer->brain_info.algo.target_player = iBestPlayer;
        pPlayer->brain_info.algo.steer = true;
      }
      else /* Unknown steering orders, just drift. */
      {
        joystickOutput = (joystickOutput & Joy_Button); /* Use no direction. */
        pPlayer->brain_info.algo.steer = false; /* Just drift. */
      }
      break;

    case TARGET_CODE_DELAY:
      pPlayer->brain_info.algo.delay_remaining = currentOpcode.target_pixel_x;
      break;

    case TARGET_CODE_GOTO:
      pPlayer->brain_info.algo.target_list_index = currentOpcode.target_pixel_x;
      break;

    default: /* Just a pair of X and Y column/row target coordinates. */
      pPlayer->brain_info.algo.target_pixel_x =
        currentOpcode.target_pixel_x * TILE_PIXEL_WIDTH;
      pPlayer->brain_info.algo.target_pixel_y =
        currentOpcode.target_pixel_y * TILE_PIXEL_WIDTH;
      pPlayer->brain_info.algo.target_player = MAX_PLAYERS;
      pPlayer->brain_info.algo.steer = true;
      break;
    }
  }

  /* Update the steering direction, and detect closeness to targets. */

  if (pPlayer->brain_info.algo.steer)
  {
    /* First update target location to the position of the player being
       followed, if any.  Static targets just leave the target coordinates
       unchanged and don't specify a player to follow. */

    if (pPlayer->brain_info.algo.target_player < MAX_PLAYERS)
    {
      player_pointer pTargetPlayer =
        g_player_array + pPlayer->brain_info.algo.target_player;
      pPlayer->brain_info.algo.target_pixel_x =
        GET_FX_INTEGER(pTargetPlayer->pixel_center_x);
      pPlayer->brain_info.algo.target_pixel_y =
        GET_FX_INTEGER(pTargetPlayer->pixel_center_y);
    }

    /* How far and what direction to the target? */

    int16_t deltaX = pPlayer->brain_info.algo.target_pixel_x -
      GET_FX_INTEGER(pPlayer->pixel_center_x);
    int16_t deltaY = pPlayer->brain_info.algo.target_pixel_y -
      GET_FX_INTEGER(pPlayer->pixel_center_y);

    int16_t targetDistance;
    if (deltaX < 0)
      targetDistance = -deltaX;
    else
      targetDistance = deltaX;
    if (deltaY < 0)
      targetDistance -= deltaY;
    else
      targetDistance += deltaY;
    pPlayer->brain_info.algo.target_distance = targetDistance;

    /* Steer in the desired direction.  Don't steer (no joystick input) if
       already going in that direction to save on CPU. */

    uint8_t steerDirection = (INT16_TO_OCTANT(deltaX, deltaY) & 7);
    if (pPlayer->velocity_octant != steerDirection)
      joystickOutput |= JoystickForOctant(steerDirection);
  }

  pPlayer->joystick_inputs = joystickOutput;
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

  for (iInput = 0; iInput < 5; iInput++)
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
      if (iStick < 4)
      {
        input_consumed[iStick] = true;
        /* Note Nabu has high bits always set in joystick data, we want zero for
           no input from the user. */
        pPlayer->joystick_inputs = (g_JoystickStatus[iStick] & 0x1F);
      }
      else /* Bug of some sort, with a garbage joystick number. */
        pPlayer->joystick_inputs = 0;
      continue;
    }

    /* One of the fancier brains, do algorithmic joystick simulation.  But only
       update one AI each frame to save on CPU time. */
    if (pPlayer->brain == ((player_brain) BRAIN_ALGORITHM))
    {
      if (iPlayer == (g_FrameCounter & MAX_PLAYERS_MASK))
        BrainUpdateJoystick(pPlayer);
      continue;
    }

    /* Unimplemented type of user input source, do nothing. */
    pPlayer->joystick_inputs = 0;
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

    /* Find an idle player and assign it to this input.  Or an AI player? */

    for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
      iPlayer++, pPlayer++)
    {
      if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE))
        continue;
      break;
    }
    if (iPlayer >= MAX_PLAYERS) /* Hunt down an AI player to replace. */
    {
      for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
        iPlayer++, pPlayer++)
      {
        if (pPlayer->brain != ((player_brain) BRAIN_ALGORITHM))
          continue;
        break;
      }
    }
    if (iPlayer < MAX_PLAYERS)
    {
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
    }
  }

  /* Process the player actions.  Do idle timeout for inactive players. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    uint8_t joyStickData;

    if (pPlayer->brain == ((player_brain) BRAIN_INACTIVE))
      continue;
    joyStickData = pPlayer->joystick_inputs;

    /* If idle too long (30 seconds), deactivate the player. */

    if (joyStickData)
      pPlayer->last_brain_activity_time = g_FrameCounter;
    else if (g_FrameCounter - pPlayer->last_brain_activity_time > 30 * 30)
    {
      pPlayer->brain = ((player_brain) BRAIN_INACTIVE);
      continue;
    }

    /* Update the direction the player's velocity is pointing in.  It's only
       used here for modifying velocity direction, not in the physics
       simulation.  Needed for applying harvest, and for steering. */

    uint8_t player_velocity_octant = 0;
    if (pPlayer->thrust_harvested ||
    (joyStickData & (Joy_Up | Joy_Down | Joy_Right | Joy_Left)))
    {
      player_velocity_octant = VECTOR_FX_TO_OCTANT(
        &pPlayer->velocity_x, &pPlayer->velocity_y);
      pPlayer->velocity_octant_right_on = ((player_velocity_octant & 0x80) != 0);
      player_velocity_octant &= 7; /* Low 3 bits contain octant number. */
      pPlayer->velocity_octant = player_velocity_octant;
    }

    /* If thrusted in the last frame and harvested some points, speed up in
       the current velocity direction.  Number of tiles harvested is converted
       to a fraction of pixel per update velocity (else things go way too
       fast and the frame rate slows down a lot as the physics keeps up). */

    if (pPlayer->thrust_harvested)
    {
      fx thrustAmount;
      INT_FRACTION_TO_FX(0, pPlayer->thrust_harvested * 2048, thrustAmount);

      if (player_velocity_octant <= 1 || player_velocity_octant == 7)
        ADD_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);
      else if (player_velocity_octant >= 3 && player_velocity_octant <= 5)
        SUBTRACT_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);

      if (player_velocity_octant >= 1 && player_velocity_octant <= 3)
        ADD_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);
      else if (player_velocity_octant >= 5)
        SUBTRACT_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);

      /* Play a sound for harvesting.  Maybe. */
      PlaySound(SOUND_HARVEST, pPlayer);
    }

    /* Apply joystick actions.  Fire does harvesting.  Directional buttons
       steer.  Or just drift if not specifying a direction or firing (and since
       not harvesting or steering, don't need to update the velocity octant). */

    pPlayer->thrust_active = (joyStickData & Joy_Button);

    if (joyStickData & (Joy_Up | Joy_Down | Joy_Right | Joy_Left))
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

#if DEBUG_PRINT_OCTANTS
printf("Player %d Velocity (%f,%f) octant %d, right on %d, desire %d.\n",
iPlayer,
GET_FX_FLOAT(pPlayer->velocity_x), GET_FX_FLOAT(pPlayer->velocity_y),
pPlayer->velocity_octant, pPlayer->velocity_octant_right_on, desired_octant);
#endif

      /* Modify the velocity direction to be closer to our desired_octant. */

      TurnVelocityTowardsOctant(pPlayer, desired_octant);
#if DEBUG_PRINT_OCTANTS
printf("  Turned velocity (%f,%f).\n",
GET_FX_FLOAT(pPlayer->velocity_x), GET_FX_FLOAT(pPlayer->velocity_y));
#endif
    }

#if 1
    /* Add some friction, to reduce excessive velocity which slows the frame
       rate (physics has to run multiple steps) and is uncontrollable for the
       user.  Use cheap speed from simulation update (lags a frame). */

    if (pPlayer->speed >= FRICTION_SPEED)
    {
      static fx portionOfVelocity;
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
      pPlayer->thrust_harvested == 0)
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

