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
#include "soundscreen.h"
#include "levels.h"

#ifdef NABU_H
#include "Art/NthPong1.h" /* Artwork data definitions for player sprites. */

/* Predefined colour choices for the players, used on the NABU.  The palette is
   so limited that it's not worthwhile letting the player choose.  The first
   three are fairly distinct, though the fourth redish one is easily confused
   with others, which is why it's fourth. */

const colour_triplet_record k_PLAYER_COLOURS[MAX_PLAYERS] = {
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

uint8_t gLevelMaxAIPlayers = MAX_PLAYERS;

uint8_t g_FrictionSpeed = 16;
fx g_FrictionSpeedFx;

fx g_SeparationVelocityFxAdd;
fx g_SeparationVelocityFxStepAdd;

uint8_t g_PhysicsStepSizeLimit = 16;

uint8_t g_PhysicsTurnRate = 6;
fx g_TurnRateFx;

uint8_t g_KeyboardFakeJoystickStatus;
#ifndef NABU_H
  uint8_t g_JoystickStatus[4];
#endif

/* List of column/row locations for AIs to move towards, and some instructions,
   has a byte sized index in the opcodes so the array is currently limited to
   at most 256 elements. */
target_list_item_record g_target_list[] = {
  {12, TARGET_CODE_POWER_UP}, /* Minor diversions, busy drawing a letter. */
  {255, TARGET_CODE_SPEED}, /* Pure harvest mode, no trail. */
  {9, 20}, /* Capital N, start at bottom left corner of letter. */
  {2, TARGET_CODE_SPEED}, /* Slow speed, leave a trail. */
  {9, 11},
  {9, 2},
  {1, TARGET_CODE_SPEED}, /* Even slower, more solid diagonal trail. */
  {10, 4},
  {12, 8},
  {14, 11},
  {16, 15},
  {17, 18},
  {19, 20},
  {2, TARGET_CODE_SPEED}, /* Slow speed with a trail. */
  {19, 11},
  {19, 3},
  {10, TARGET_CODE_SPEED}, /* Speed up, upwards. */
  {19, 1}, /* Straight up to top of screen. */
  {255, TARGET_CODE_SPEED}, /* No trail harvest mode, head right along top. */
  {27, 1},
  {9 * 8, TARGET_CODE_POWER_UP}, /* Divert up to 9 tiles away for power-ups. */
  {10, TARGET_CODE_SPEED}, /* Speed down right side of screen. */
  {30, 1},
  {30, 23},
  {3 * 8, TARGET_CODE_POWER_UP}, /* Divert up to 3 tiles away for power-ups. */
  {9, 23}, /* Back under the starting point of the letter. */
  {0, TARGET_CODE_GOTO},
  {0, TARGET_CODE_GOTO},
  {0, TARGET_CODE_GOTO},
  {0, TARGET_CODE_GOTO},
/* Instruction 30 */ {20, TARGET_CODE_SPEED}, /* Move faster. */
  {12, TARGET_CODE_POWER_UP}, /* Divert up to 1.5 tiles away for power-ups. */
  {4, TARGET_CODE_STEER}, /* Head to leading player.  Just bump into them. */
  {9, TARGET_CODE_SPEED}, /* Slow down, below friction speed. */
  {99, TARGET_CODE_STEER}, /* Just bounce around, no steering. */
  {50, TARGET_CODE_DELAY}, /* Delay 10 seconds idling and bouncing. */
  {0, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {32, TARGET_CODE_POWER_UP}, /* Divert up to 4 tiles away for power-ups. */
  {1, TARGET_CODE_STEER}, /* Steer to next corner. */
  {30, TARGET_CODE_GOTO},
/* Instruction 40 */ {11, TARGET_CODE_SPEED},
  {0, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {4 * 8, TARGET_CODE_POWER_UP}, /* Divert up to 4 tiles away for power-ups. */
  {1, TARGET_CODE_STEER}, /* Steer to next corner. */
  {6, TARGET_CODE_SPEED}, /* Slower movement. */
  {0, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {0, TARGET_CODE_POWER_UP}, /* No diversions, busy chasing player. */
  {15, TARGET_CODE_SPEED}, /* Fast movement towards player. */
  {5, TARGET_CODE_STEER}, /* Head to leading Human player. */
  {7 * 8, TARGET_CODE_POWER_UP}, /* Divert up to 7 tiles away for power-ups. */
  {5, TARGET_CODE_DELAY}, /* Delay 1 second to hound them. */
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
  {40, TARGET_CODE_GOTO},
/* Instruction 60 */ {8, TARGET_CODE_SPEED}, /* Go below friction speed. */
  {50, TARGET_CODE_POWER_UP}, /* Get middling distance power-ups. */
  {0, TARGET_CODE_STEER}, /* Steer to your own corner. */
  {0, TARGET_CODE_POWER_UP}, /* Don't chase power-ups. */
  {16, TARGET_CODE_SPEED}, /* Faster speed. */
  {99, TARGET_CODE_STEER}, /* Just bounce around, no steering, no power-ups. */
  {200, TARGET_CODE_DELAY}, /* Delay a while for idle bouncing. */
  {8, TARGET_CODE_SPEED}, /* Go below friction speed. */
  {50, TARGET_CODE_POWER_UP}, /* Get middling distance power-ups. */
  {2, TARGET_CODE_STEER}, /* Steer to opposite corner. */
  {0, TARGET_CODE_POWER_UP}, /* Don't chase power-ups, much. */
  {16, TARGET_CODE_SPEED}, /* Faster speed. */
  {99, TARGET_CODE_STEER}, /* Just bounce around, no steering, no power-ups. */
  {200, TARGET_CODE_DELAY}, /* Delay a while for idle bouncing. */
  {60, TARGET_CODE_GOTO},
  {60, TARGET_CODE_GOTO},
  {60, TARGET_CODE_GOTO},
  {60, TARGET_CODE_GOTO},
  {60, TARGET_CODE_GOTO},
  {60, TARGET_CODE_GOTO},
/* Instruction 80 */ {8, TARGET_CODE_SPEED}, /* Go below friction speed. */
  {0, TARGET_CODE_POWER_UP}, /* Don't chase power-ups. */
  {99, TARGET_CODE_STEER}, /* Just bounce around, no steering, no power-ups. */
  {200, TARGET_CODE_DELAY}, /* Delay a while for idle bouncing. */
  {80, TARGET_CODE_GOTO},
  {80, TARGET_CODE_GOTO},
/* Instruction 86 */ {28, 16}, /* End of 4th lane. */
  {80, TARGET_CODE_GOTO},
/* Instruction 88 */ {28, 11}, /* End of 3rd lane. */
  {80, TARGET_CODE_GOTO},
/* Instruction 90 */ {10, TARGET_CODE_SPEED}, /* Go below friction speed. */
  {50, TARGET_CODE_POWER_UP}, /* Get middling distance power-ups. */
  {4, TARGET_CODE_STEER}, /* Head to leading player.  Just bump into them. */
  {6, TARGET_CODE_SPEED}, /* Slow down. */
  {99, TARGET_CODE_STEER}, /* Just bounce around, no steering, no power-ups. */
  {5, TARGET_CODE_DELAY}, /* Delay 1 second idling and bouncing. */
  {90, TARGET_CODE_GOTO},
};

/* Where to start the instructions for player 0 to player N-1. */
uint8_t g_target_start_indices[MAX_PLAYERS] =
{30, 40, 0, 30};


/* Set up the initial player data, mostly colours and animations. */
void InitialisePlayers(void)
{
  uint8_t iPlayer;
  uint16_t pixelCoord;
  player_pointer pPlayer;

  /* Set the constant velocity change used for separating collided players. */

  INT_TO_FX(1, g_SeparationVelocityFxAdd);

  /* Some other simulation control FX values need setting the first time. */

  INT_TO_FX(g_FrictionSpeed, g_FrictionSpeedFx);
  DIV2Nth_FX(&g_FrictionSpeedFx, 2); /* Divide by 4 = quarters */

  INT_TO_FX(g_PhysicsTurnRate, g_TurnRateFx);
  DIV2Nth_FX(&g_TurnRateFx, 2); /* Divide by 4 = quarters */

  /* Set up the players, scattered across the screen. */

  pixelCoord = 32;
  for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
  iPlayer++, pPlayer++)
  {
    pPlayer->player_array_index = iPlayer; /* Fast convert pointer to index. */
    pPlayer->win_count = 0;

    pPlayer->starting_level_pixel_x = pixelCoord;
    pPlayer->starting_level_pixel_y = pixelCoord;
    pPlayer->starting_level_pixel_flying_height = 15;
    pixelCoord += 32;

    /* Make them all inactive players.  AI players get added later while the
       game runs. */

    pPlayer->brain = (player_brain) BRAIN_INACTIVE;
    bzero(&pPlayer->brain_info, sizeof(pPlayer->brain_info));

    pPlayer->main_colour =
#ifdef NABU_H
      k_PLAYER_COLOURS[iPlayer].main;
#else /* ncurses uses 1 to 4.  init_pair() sets it up, 0 for B&W text. */
      iPlayer + 1;
#endif

#ifdef NABU_H
    pPlayer->shadow_colour = k_PLAYER_COLOURS[iPlayer].shadow;
    pPlayer->sparkle_colour = k_PLAYER_COLOURS[iPlayer].sparkle;
#endif /* NABU_H */
  }

  InitialisePlayersForNewLevel();
}


/* Reset player things to get ready for running the next level.
*/
void InitialisePlayersForNewLevel(void)
{
  uint8_t iPlayer;
  player_pointer pPlayer;

  /* Cache the positions of the walls. */

  g_play_area_wall_bottom_y = g_play_area_height_pixels -
    PLAYER_PIXEL_DIAMETER_NORMAL / 2;
  g_play_area_wall_left_x = PLAYER_PIXEL_DIAMETER_NORMAL / 2;
  g_play_area_wall_right_x = g_play_area_width_pixels -
    PLAYER_PIXEL_DIAMETER_NORMAL / 2;
  g_play_area_wall_top_y = PLAYER_PIXEL_DIAMETER_NORMAL / 2;

  /* Reset the players for a new level. */

  for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
  iPlayer++, pPlayer++)
  {
    INT_TO_FX(pPlayer->starting_level_pixel_x, pPlayer->pixel_center_x);
    INT_TO_FX(pPlayer->starting_level_pixel_y, pPlayer->pixel_center_y);
    pPlayer->pixel_flying_height = pPlayer->starting_level_pixel_flying_height;
    INT_TO_FX(0, pPlayer->velocity_x);
    INT_TO_FX(0, pPlayer->velocity_y);
    pPlayer->player_collision_count = 0;
    bzero(&pPlayer->power_up_timers, sizeof(pPlayer->power_up_timers));
    pPlayer->joystick_inputs = 0;
    pPlayer->brain_info.algo.target_list_index =
      g_target_start_indices[iPlayer];
    pPlayer->brain_info.algo.steer = true;
    pPlayer->brain_info.algo.target_player = MAX_PLAYERS;
#ifdef NABU_H
    pPlayer->main_anim = g_SpriteAnimData[SPRITE_ANIM_BALL_ROLLING];
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

  /* Set the maximum amount to move between axis (turnAmount), larger
     makes for faster rotations. */

  static fx turnAmount; /* Static faster code than having it on the stack. */

  if (g_PhysicsTurnRate == 0) /* Means calculate it based on velocity. */
  {
    COPY_FX(&pPlayer->velocity_x, &turnAmount);
    ABS_FX(&turnAmount);
    if (IS_NEGATIVE_FX(&pPlayer->velocity_y))
      SUBTRACT_FX(&turnAmount, &pPlayer->velocity_y, &turnAmount);
    else
      ADD_FX(&turnAmount, &pPlayer->velocity_y, &turnAmount);
    DIV2_FX(&turnAmount); /* Don't make turns too boringly sharp. */
  }
  else /* Using a constant turn rate limit. */
  {
    COPY_FX(&g_TurnRateFx, &turnAmount);
  }

  switch (head_towards_octant)
  {
    case 0:
      if (head_clockwise)
      { /* Reduce negative Y towards 0, add to positive X. */
        fx positiveY;
        COPY_NEGATE_FX(&pPlayer->velocity_y, &positiveY);
        if (COMPARE_FX(&positiveY, &turnAmount) < 0)
          COPY_FX(&positiveY, &turnAmount); /* Limited by available Y. */
        ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
      }
      else
      { /* Reduce positive Y towards 0, add to positive X. */
        if (COMPARE_FX(&pPlayer->velocity_y, &turnAmount) < 0)
          COPY_FX(&pPlayer->velocity_y, &turnAmount);
        SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
      }
      break;

    case 1:
      if (head_clockwise)
      { /* Reduce positive X, add to positive Y until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y|
             Note that we can't just subtract/add the half balance from both,
             since there could be rounding errors in the last bit. */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
          ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce positive Y, add to positive X until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
          ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        }
      }
      break;

    case 2:
      if (head_clockwise)
      { /* Reduce positive X towards 0, add to positive Y. */
        if (COMPARE_FX(&pPlayer->velocity_x, &turnAmount) < 0)
          COPY_FX(&pPlayer->velocity_x, &turnAmount);
        SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
      }
      else
      { /* Reduce negative X towards 0, add to positive Y. */
        fx positiveX;
        COPY_NEGATE_FX(&pPlayer->velocity_x, &positiveX);
        if (COMPARE_FX(&positiveX, &turnAmount) < 0)
          COPY_FX(&positiveX, &turnAmount);
        ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
      }
      break;

    case 3:
      if (head_clockwise)
      { /* Reduce positive Y, increase negative X until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_NEGATE_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        }
      }
      else
      { /* Reduce negative X, increase positive Y until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_NEGATE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
          ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        }
      }
      break;

    case 4:
      if (head_clockwise)
      { /* Reduce positive Y towards 0, increase negative X. */
        if (COMPARE_FX(&pPlayer->velocity_y, &turnAmount) < 0)
          COPY_FX(&pPlayer->velocity_y, &turnAmount);
        SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
      }
      else
      { /* Reduce negative Y towards 0, increase negative X. */
        fx positiveY;
        COPY_NEGATE_FX(&pPlayer->velocity_y, &positiveY);
        if (COMPARE_FX(&positiveY, &turnAmount) < 0)
          COPY_FX(&positiveY, &turnAmount);
        ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
      }
      break;

    case 5:
      if (head_clockwise)
      { /* Reduce negative X, increase negative Y until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
          SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce negative Y, increase negative X until equal balance. */
        fx balance;
        SUBTRACT_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        }
      }
      break;

    case 6:
      if (head_clockwise)
      { /* Reduce negative X towards 0, increase negative Y. */
        fx positiveX;
        COPY_NEGATE_FX(&pPlayer->velocity_x, &positiveX);
        if (COMPARE_FX(&positiveX, &turnAmount) < 0)
          COPY_FX(&positiveX, &turnAmount);
        ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
      }
      else
      { /* Reduce positive X towards 0, increase negative Y. */
        if (COMPARE_FX(&pPlayer->velocity_x, &turnAmount) < 0)
          COPY_FX(&pPlayer->velocity_x, &turnAmount);
        SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
        SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
      }
      break;

    case 7:
      if (head_clockwise)
      { /* Reduce negative Y, increase positive X until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &balance);
        NEGATE_FX(&balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          ADD_FX(&pPlayer->velocity_y, &balance, &pPlayer->velocity_y);
          COPY_NEGATE_FX(&pPlayer->velocity_y, &pPlayer->velocity_x);
        }
        else
        {
          ADD_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
          ADD_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
        }
      }
      else
      { /* Reduce positive X, increase negative Y until equal balance. */
        fx balance;
        ADD_FX(&pPlayer->velocity_x, &pPlayer->velocity_y, &balance);
        if (COMPARE_FX(&balance, &turnAmount) < 0)
        { /* Make it be exactly on the octant diagonal line, with |X|==|Y| */
          DIV2_FX(&balance);
          SUBTRACT_FX(&pPlayer->velocity_x, &balance, &pPlayer->velocity_x);
          COPY_NEGATE_FX(&pPlayer->velocity_x, &pPlayer->velocity_y);
        }
        else
        {
          SUBTRACT_FX(&pPlayer->velocity_y, &turnAmount, &pPlayer->velocity_y);
          SUBTRACT_FX(&pPlayer->velocity_x, &turnAmount, &pPlayer->velocity_x);
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
  if (!gVictoryModeHighestTileCount) /* Not playing a game, so do nothing. */
  {
    pPlayer->joystick_inputs = 0;
    return;
  }

  /* Player position is used a lot, cache a copy here. */
  int16_t playerX = GET_FX_INTEGER(pPlayer->pixel_center_x);
  int16_t playerY = GET_FX_INTEGER(pPlayer->pixel_center_y);

  /* Alternate every update between trail and harvest mode when the AI needs
     more speed (using the technique of laying down a tile and then harvesting
     it in the next frame, rather than the bigger picture technique of heading
     towards larger areas of your tiles).  Special speed of 255(-1) means stay
     in harvest mode to not leave a trail while moving (though do trail if you
     become completely stationary. */

  uint8_t joystickOutput = 0;
  uint8_t desiredSpeed = pPlayer->brain_info.algo.desired_speed;
  bool wasHarvesting = (pPlayer->joystick_inputs & Joy_Button) != 0;

  if (desiredSpeed == (uint8_t) 255)
  {
    /* Code for always harvest, used for not leaving a trail.  Well, except
       when stopped, then coast/harvest alternate updates else the AI player
       gets stuck. */

    if (pPlayer->speed != 0 || !wasHarvesting)
      joystickOutput |= Joy_Button;
  }
  else
  {
    /* deltaSpeed is a positive number if more speed needed.  Note that player
      speed is in 1/4 pixels per update for extra precision. */
    int8_t deltaSpeed = (int8_t) desiredSpeed - (int8_t) pPlayer->speed;

    /* Go to a slightly faster speed when you start accelerating, so you can
       then coast for longer, making for less frequent annoying harvest sound
       effects. */
    bool hysteresis = pPlayer->brain_info.algo.speed_hysteresis;
    if (hysteresis)
      deltaSpeed += 4;

    if (deltaSpeed > 0)
    {
      if (!wasHarvesting)
        joystickOutput |= Joy_Button; /* Alternately turn on harvest mode. */

      if (!hysteresis)
      {
        pPlayer->brain_info.algo.speed_hysteresis = true;
#if 0
        strcpy(g_TempBuffer, "Player #");
        AppendDecimalUInt16(pPlayer->player_array_index);
        strcat(g_TempBuffer, " hysteresis on, speed was ");
        AppendDecimalUInt16(pPlayer->speed);
        strcat(g_TempBuffer, ".\n");
        DebugPrintString(g_TempBuffer);
#endif
      }
    }
    else /* Have reached target speed. */
    {
      if (hysteresis)
      {
        pPlayer->brain_info.algo.speed_hysteresis = false;
#if 0
        strcpy(g_TempBuffer, "Player #");
        AppendDecimalUInt16(pPlayer->player_array_index);
        strcat(g_TempBuffer, " removed hysteresis, speed was ");
        AppendDecimalUInt16(pPlayer->speed);
        strcat(g_TempBuffer, ".\n");
        DebugPrintString(g_TempBuffer);
#endif
      }
    }
  }

  /* Has the current opcode finished?  Or is it still waiting? */

  if (pPlayer->brain_info.algo.delay_remaining != 0)
  { /* Keep on waiting until the countdown has finished. */
    pPlayer->brain_info.algo.delay_remaining--;
  }
  else if (pPlayer->brain_info.algo.divert_to_pTile)
  {
    /* Busy diverting to a nearby "good" power-up.  Is it still there?  If
       someone else takes it, or we take it, or if we are too far above it to
       pick it up, end diversion. */

    if (pPlayer->brain_info.algo.divert_to_pTile->owner <
    (tile_owner) OWNER_PUPS_GOOD_FOR_AI ||
    pPlayer->pixel_flying_height >= FLYING_ABOVE_TILES_HEIGHT)
    {
      /* Resume normal targeting. */
      pPlayer->brain_info.algo.divert_to_pTile = NULL;
      pPlayer->brain_info.algo.target_pixel_x =
        pPlayer->brain_info.algo.divert_saved_pixel_x;
      pPlayer->brain_info.algo.target_pixel_y =
        pPlayer->brain_info.algo.divert_saved_pixel_y;
    }
  }
  else if (pPlayer->brain_info.algo.target_distance >
  PLAYER_PIXEL_DIAMETER_NORMAL)
  {
    /* Not yet close enough to the target.  Continue hunting. */
  }
  else
  { /* No more waiting, at target and delay has finished, do current opcode
       actions and point to next one for next time. */

    target_list_item_record currentOpcode;
    currentOpcode = g_target_list[pPlayer->brain_info.algo.target_list_index++];

    switch (currentOpcode.target_tile_y)
    {
    case TARGET_CODE_SPEED:
      pPlayer->brain_info.algo.desired_speed = currentOpcode.target_tile_x;
      break;

    case TARGET_CODE_STEER:
      /* X coordinate is 0 to 3 to steer towards a corner.  Zero for your corner
         of the board, 1 for next player's corner, 2 for next next player and 3
         for the next next next player.  4 to target a rival player with the
         highest score, 5 to target the human with the highest score, other
         values make you drift rather than steer. */
      if (currentOpcode.target_tile_x <= 3)
      { /* Target your own corner of the game board, or the next ones around. */
        uint8_t iMyself = (3 &
          (pPlayer->player_array_index + currentOpcode.target_tile_x));
        if (iMyself == 0 || iMyself == 3)
          pPlayer->brain_info.algo.target_pixel_x = TILE_PIXEL_WIDTH;
        else
          pPlayer->brain_info.algo.target_pixel_x =
            g_play_area_width_tiles * (int16_t) TILE_PIXEL_WIDTH -
            TILE_PIXEL_WIDTH;
        if (iMyself == 0 || iMyself == 1)
          pPlayer->brain_info.algo.target_pixel_y = TILE_PIXEL_WIDTH;
        else
          pPlayer->brain_info.algo.target_pixel_y =
            g_play_area_height_tiles * (int16_t) TILE_PIXEL_WIDTH -
            TILE_PIXEL_WIDTH;
        pPlayer->brain_info.algo.target_player = MAX_PLAYERS;
        pPlayer->brain_info.algo.steer = true;
      }
      else if (currentOpcode.target_tile_x == 4 ||
      currentOpcode.target_tile_x == 5)
      { /* Target the rival player with highest score. */
        uint8_t iMyself = pPlayer->player_array_index;
        uint8_t iBestPlayer = MAX_PLAYERS; /* Also code for don't target. */
        uint16_t maxScore = 0;
        bool onlyHumans = (currentOpcode.target_tile_x == 5);
        uint8_t iPlayer;
        for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
        {
          if (iPlayer == iMyself)
            continue;
          uint16_t theirScore = GetPlayerScore(iPlayer);
          if (theirScore < maxScore) /* Note - zero can be a high score. */
            continue;
          if (onlyHumans && g_player_array[iPlayer].brain == BRAIN_ALGORITHM)
            continue; /* Reject AI players. */

          /* Got a better player. */
          maxScore = theirScore;
          iBestPlayer = iPlayer;
        }
        pPlayer->brain_info.algo.steer = true;
        pPlayer->brain_info.algo.target_player = iBestPlayer;
        if (iBestPlayer >= MAX_PLAYERS)
        {
          /* No target player found, set current position as the static target
             location, so that this opcode completes successfully very soon.
             So you can target the best Human player, and if no Humans are
             playing, the opcode will do almost nothing. */

          pPlayer->brain_info.algo.target_pixel_x = playerX;
          pPlayer->brain_info.algo.target_pixel_y = playerY;
        }
      }
      else /* Unknown steering orders, just drift. */
      {
        joystickOutput = (joystickOutput & Joy_Button); /* Use no direction. */
        pPlayer->brain_info.algo.steer = false; /* Just drift. */
      }
      break;

    case TARGET_CODE_DELAY:
      pPlayer->brain_info.algo.delay_remaining = currentOpcode.target_tile_x;
      break;

    case TARGET_CODE_GOTO:
      pPlayer->brain_info.algo.target_list_index = currentOpcode.target_tile_x;
      break;

    case TARGET_CODE_POWER_UP:
      pPlayer->brain_info.algo.divert_powerup_distance =
        currentOpcode.target_tile_x;
      break;

    default:
      /* Just a pair of X and Y column/row target coordinates, go to center of
         that tile (any closer to walls and you bounce off the wall).  Clip to
         just inside the actual board size so you don't get AI's trying to go
         past the edge of the board when you load a smaller board than what the
         opcodes use. */

      uint8_t tileX;
      tileX = currentOpcode.target_tile_x;
      if (tileX >= g_play_area_width_tiles - 1)
        tileX = g_play_area_width_tiles - 2;
      pPlayer->brain_info.algo.target_pixel_x =
        tileX * TILE_PIXEL_WIDTH;

      uint8_t tileY;
      tileY = currentOpcode.target_tile_y;
      if (tileY >= g_play_area_height_tiles - 1)
        tileY = g_play_area_height_tiles - 2;
      pPlayer->brain_info.algo.target_pixel_y =
        tileY * TILE_PIXEL_WIDTH;

      pPlayer->brain_info.algo.target_player = MAX_PLAYERS;
      pPlayer->brain_info.algo.steer = true;
      break;
    }
  }

  /* Update the steering direction, and detect closeness to targets. */

  if (pPlayer->brain_info.algo.steer)
  {
    /* See if there are any nearby power-ups we might want to divert to. */

    int16_t powerUpMaxPixelDistance =
      pPlayer->brain_info.algo.divert_powerup_distance;

    if (powerUpMaxPixelDistance > 0 &&
    pPlayer->brain_info.algo.divert_to_pTile == NULL &&
    pPlayer->pixel_flying_height < FLYING_ABOVE_TILES_HEIGHT)
    {
      /* Just want to check over the "good" power-ups to see if they are near
        enough for a diversion.  Bad ones we ignore, no avoidance behaviour
        yet. */

      int16_t bestDistance = 0x7FFF; /* Maximum positive value of int16. */
      tile_pointer bestTile = NULL;

      tile_owner powerup_type;
      for (powerup_type = (tile_owner) OWNER_PUPS_GOOD_FOR_AI;
      powerup_type < (tile_owner) OWNER_MAX; powerup_type++)
      {
        tile_pointer *ppCachedTile = &g_TileOwnerRecentTiles[powerup_type][0];
        uint8_t recentIndex;
        for (recentIndex = 0; recentIndex < MAX_RECENT_TILES; recentIndex++)
        {
          tile_pointer pTile = *ppCachedTile++;
          if (pTile == NULL)
            break; /* End of this list. */

          /* Find the distance to the powerup. */

          int16_t deltaX = pTile->pixel_center_x - playerX;
          int16_t deltaY = pTile->pixel_center_y - playerY;
          int16_t distance;

          if (deltaX < 0)
            distance = -deltaX;
          else
            distance = deltaX;

          if (deltaY < 0)
            distance -= deltaY;
          else
            distance += deltaY;

          if (distance < bestDistance)
          {
            bestDistance = distance;
            bestTile = pTile;
          }
        }
      }

      /* Got a good power-up candidate?  Head towards it. */

      if (bestTile != NULL && bestDistance <= powerUpMaxPixelDistance)
      {
        pPlayer->brain_info.algo.divert_to_pTile = bestTile;
        pPlayer->brain_info.algo.divert_saved_pixel_x =
          pPlayer->brain_info.algo.target_pixel_x;
        pPlayer->brain_info.algo.divert_saved_pixel_y =
          pPlayer->brain_info.algo.target_pixel_y;
        pPlayer->brain_info.algo.target_pixel_x = bestTile->pixel_center_x;
        pPlayer->brain_info.algo.target_pixel_y = bestTile->pixel_center_y;
      }
    }

    /* Update target location to the position of the player being followed,
       if any.  Static targets or power-up targets just leave the target
       coordinates unchanged and don't specify a player to follow. */

    if (pPlayer->brain_info.algo.divert_to_pTile == NULL &&
    pPlayer->brain_info.algo.target_player < MAX_PLAYERS)
    {
      player_pointer pTargetPlayer =
        g_player_array + pPlayer->brain_info.algo.target_player;

      if (pTargetPlayer->brain == ((player_brain) BRAIN_INACTIVE))
        pPlayer->brain_info.algo.target_player = MAX_PLAYERS; /* Ignore it. */
      else
      {
        pPlayer->brain_info.algo.target_pixel_x =
          GET_FX_INTEGER(pTargetPlayer->pixel_center_x);
        pPlayer->brain_info.algo.target_pixel_y =
          GET_FX_INTEGER(pTargetPlayer->pixel_center_y);
      }
    }

    /* How far and what direction to the target? */

    int16_t deltaX = pPlayer->brain_info.algo.target_pixel_x - playerX;
    int16_t deltaY = pPlayer->brain_info.algo.target_pixel_y - playerY;

    /* Get sum of absolute values of deltas to find distance. */

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


/* Internal utility function for printing to debug output what player is
   controlled by what device.  Uses g_TempBuffer.
*/
void DebugPrintPlayerAssignment(player_pointer pPlayer)
{
  strcpy(g_TempBuffer, "Player #");
  AppendDecimalUInt16(pPlayer->player_array_index);
  strcat(g_TempBuffer, " assigned to ");
  switch (pPlayer->brain)
  {
    case BRAIN_INACTIVE:
      strcat(g_TempBuffer, "nothing");
      break;

    case BRAIN_KEYBOARD:
      strcat(g_TempBuffer, "keyboard");
      break;

    case BRAIN_JOYSTICK:
      strcat(g_TempBuffer, "joystick ");
      AppendDecimalUInt16(pPlayer->brain_info.iJoystick);
      break;

    case BRAIN_NETWORK:
      strcat(g_TempBuffer, "remote network");
      break;

    case BRAIN_ALGORITHM:
      strcat(g_TempBuffer, "algorithm running at ");
      AppendDecimalUInt16(pPlayer->brain_info.algo.target_list_index);
      break;

    default:
      strcat(g_TempBuffer, "unknown");
  }

  strcat(g_TempBuffer, ".\n");
  DebugPrintString(g_TempBuffer);
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

    /* Find an idle player and assign it to this input. */

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
      pPlayer->win_count = 0;
      DebugPrintPlayerAssignment(pPlayer);
    }
  }

  /* See if we need to add an AI player to make the quota of AI players.  But
     only check about once every eight seconds. */

  if ((uint8_t) g_FrameCounter == (uint8_t) 187 && gLevelMaxAIPlayers > 0)
  {
    uint8_t numAIPlayers = 0;
    player_pointer pFreePlayer = NULL;

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain == ((player_brain) BRAIN_INACTIVE))
        pFreePlayer = pPlayer;
      else if (pPlayer->brain == ((player_brain) BRAIN_ALGORITHM))
        numAIPlayers++;
    }
    if (numAIPlayers < gLevelMaxAIPlayers && pFreePlayer != NULL)
    {
      /* Make an AI player. */
      bzero(&pFreePlayer->brain_info, sizeof(pFreePlayer->brain_info));
      pFreePlayer->brain = (player_brain) BRAIN_ALGORITHM;
      pFreePlayer->brain_info.algo.desired_speed = 4 + numAIPlayers;
      pFreePlayer->brain_info.algo.target_list_index =
        g_target_start_indices[pFreePlayer->player_array_index];
      pFreePlayer->brain_info.algo.steer = false;
      pFreePlayer->brain_info.algo.target_player = MAX_PLAYERS;
      pFreePlayer->last_brain_activity_time = g_FrameCounter;
      DebugPrintPlayerAssignment(pFreePlayer);
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
      DebugPrintPlayerAssignment(pPlayer);
      continue;
    }

    /* Do some velocity adjustments here, outside the simulation, but only
       if the game is running (it makes tile harvesting noises). */

    if (gVictoryModeHighestTileCount) /* If game running. */
    {
      /* Update the direction the player's velocity is pointing in.  It's only
         used here for modifying velocity direction, not in the physics
         simulation.  Needed for applying harvest, and for steering.  Only
         update it if needed, and occasionally every 16th frame to keep AI's
         pointed in the right direction. */

      uint8_t player_velocity_octant = 0;
      if (pPlayer->thrust_harvested ||
      (joyStickData & (Joy_Up | Joy_Down | Joy_Right | Joy_Left)) ||
      ((uint8_t) g_FrameCounter & 15) == (iPlayer << 2))
      {
        player_velocity_octant = VECTOR_FX_TO_OCTANT(
          &pPlayer->velocity_x, &pPlayer->velocity_y);
        pPlayer->velocity_octant_right_on =
          ((player_velocity_octant & 0x80) != 0);
        player_velocity_octant &= 7; /* Low 3 bits contain octant number. */
        pPlayer->velocity_octant = player_velocity_octant;
      }

      /* If thrusted in the last frame and harvested some points, speed up in
         the current velocity direction.  Number of tiles harvested is converted
         to a fraction of a pixel per update velocity (else things go way too
         fast and the frame rate slows down as the physics adds more steps to
         keep up). */

      if (pPlayer->thrust_harvested)
      {
        fx thrustAmount;
        INT_TO_FX(pPlayer->thrust_harvested, thrustAmount);
        DIV2Nth_FX(&thrustAmount, 6);

        if (player_velocity_octant <= 1 || player_velocity_octant == 7)
          ADD_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);
        else if (player_velocity_octant >= 3 && player_velocity_octant <= 5)
          SUBTRACT_FX(&pPlayer->velocity_x, &thrustAmount, &pPlayer->velocity_x);

        if (player_velocity_octant >= 1 && player_velocity_octant <= 3)
          ADD_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);
        else if (player_velocity_octant >= 5)
          SUBTRACT_FX(&pPlayer->velocity_y, &thrustAmount, &pPlayer->velocity_y);

        /* Play a sound for harvesting.  Maybe, if nobody else is doing it. */
        PlaySound(SOUND_HARVEST, pPlayer);
      }

      /* Apply joystick actions.  Fire does harvesting.  Directional buttons
         steer.  Or just drift if not specifying a direction or firing (and
         since not harvesting or steering, don't need to update the velocity
         octant). */

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

      if (pPlayer->speed >= g_FrictionSpeed)
      {
        static fx portionOfVelocity;
        DIV256_FX(pPlayer->velocity_x, portionOfVelocity);
        SUBTRACT_FX(&pPlayer->velocity_x, &portionOfVelocity, &pPlayer->velocity_x);
        DIV256_FX(pPlayer->velocity_y, portionOfVelocity);
        SUBTRACT_FX(&pPlayer->velocity_y, &portionOfVelocity, &pPlayer->velocity_y);
      }
  #endif

      /* Update special effect animations, mostly for feedback to the player.
         Note the priority order implied, though in future we could have more
         sparkle animations active simultaneously. */

      SpriteAnimationType newAnimType = (SpriteAnimationType) SPRITE_ANIM_NONE;
      if (pPlayer->power_up_timers[OWNER_PUP_BASH_WALL])
        newAnimType = (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_BASH;
      else if (pPlayer->power_up_timers[OWNER_PUP_SOLID])
        newAnimType = (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_SOLID;
      else if (pPlayer->power_up_timers[OWNER_PUP_WIDER])
        newAnimType = (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_WIDER;
      else if (pPlayer->thrust_active)
        newAnimType = (SpriteAnimationType) SPRITE_ANIM_BALL_EFFECT_THRUST;

      /* Start a new animation if needed, copying the whole animation structure
         to set the initial state.  Otherwise the previous animation continues
         to play. */

      if (newAnimType != pPlayer->sparkle_anim.type)
        pPlayer->sparkle_anim = g_SpriteAnimData[newAnimType];

      /* Switch in an extra bold harvest animation frame if they actually
         harvested something.  Once it has played, the current animation will
         restart from the beginning, because the override frame number is higher
         than the normal range of animation frames in every animation. */

      if (pPlayer->thrust_harvested)
      {
         pPlayer->sparkle_anim.current_delay =
          g_SpriteAnimData[SPRITE_ANIM_BALL_EFFECT_THRUST].delay;
         pPlayer->sparkle_anim.current_name =
          SPRITE_ANIM_BALL_EFFECT_THRUST_BOLD_FRAME;
      }
    }
  }
}


/* Make all players brainless.  Sometimes after a slide show you want a level
   to start with no Human players.  A player would have been assigned when a
   button was pressed in the slide show.
*/
void DeassignPlayersFromDevices(void)
{
  uint8_t iPlayer = MAX_PLAYERS - 1;
  player_pointer pPlayer = g_player_array;
  do {
    if (pPlayer->brain != ((player_brain) BRAIN_INACTIVE))
    {
      pPlayer->brain = ((player_brain) BRAIN_INACTIVE);
      DebugPrintPlayerAssignment(pPlayer);
    }
    pPlayer++;
  } while (iPlayer-- != 0);
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
      /* Confusing for player to change the shadow colour, leave it black.
      IO_VDPDATA = pPlayer->vdpShadowEarlyClock32Left | pPlayer->shadow_colour;
      */
      IO_VDPDATA = pPlayer->vdpShadowEarlyClock32Left | VDP_BLACK;
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  /* No more sprites to draw, terminate sprite list for the VDP with magic
     vertical position of 208 or $D0. */
  IO_VDPDATA = 0xD0;
}
#endif /* NABU_H */


/* For debugging, print all the player assignments on the terminal.
   Uses g_TempBuffer.
*/
void DumpPlayersToTerminal(void)
{
  uint8_t iPlayer = MAX_PLAYERS - 1;
  player_pointer pPlayer = g_player_array;

  DebugPrintString("Player data dump...\n");

  do {
    DebugPrintPlayerAssignment(pPlayer);

    strcpy(g_TempBuffer, "Location (");
    AppendDecimalUInt16(GET_FX_INTEGER(pPlayer->pixel_center_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalUInt16(GET_FX_INTEGER(pPlayer->pixel_center_y));
    strcat(g_TempBuffer, ", ");
    AppendDecimalUInt16(pPlayer->pixel_flying_height);
    strcat(g_TempBuffer, "), velocity (");
    AppendDecimalUInt16(GET_FX_INTEGER(pPlayer->velocity_x));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_x));
    strcat(g_TempBuffer, ", ");
    AppendDecimalUInt16(GET_FX_INTEGER(pPlayer->velocity_y));
    strcat(g_TempBuffer, ".");
    AppendDecimalUInt16(GET_FX_FRACTION(pPlayer->velocity_y));
    strcat(g_TempBuffer, ").\n");
    DebugPrintString(g_TempBuffer);

    pPlayer++;
  } while (iPlayer-- != 0);
}

