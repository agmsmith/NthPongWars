/******************************************************************************
 * Nth Pong Wars, players.c for player drawing and thinking.
 *
 * AGMS20241220 - Started this code file.
 */

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

    pPlayer->brain = BRAIN_INACTIVE;
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
    if (pPlayer->brain != BRAIN_INACTIVE)
    {
      UpdateOneAnimation(&pPlayer->main_anim);

#ifdef NABU_H
      if (pPlayer->sparkle_anim.type != SPRITE_ANIM_NONE)
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


/* Figure out what joystick action the player should do based on various
   algorithms. */
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

  /* Copy inputs to player records.  For AI players, generates the inputs. */

  pPlayer = g_player_array;
  for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
  {
    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    if (pPlayer->brain == BRAIN_KEYBOARD)
    {
      pPlayer->joystick_inputs = g_KeyboardFakeJoystickStatus;
      input_consumed[4] = true;
      continue;
    }

    if (pPlayer->brain == BRAIN_JOYSTICK)
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

  /* Look for unconsumed inputs, and assign an idle player to them. */

  for (iInput = 0; iInput < sizeof (input_consumed); iInput++)
  {
    uint8_t joyStickData;

    if (input_consumed[iInput])
      continue; /* Input already used up by an assigned player. */

    if (iInput < 4)
      joyStickData = (g_JoystickStatus[iInput] & 0x1F);
    else
      joyStickData = g_KeyboardFakeJoystickStatus;
    if (joyStickData == 0)
      continue; /* No buttons pressed, no activity. */

    /* Find an idle player and assign it to this input. */

    pPlayer = g_player_array;
    for (iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++, pPlayer++)
    {
      if (pPlayer->brain != BRAIN_INACTIVE)
        continue;

      pPlayer->brain_info.iJoystick = iInput;
      pPlayer->brain = (iInput < 4) ? BRAIN_JOYSTICK : BRAIN_KEYBOARD;
      pPlayer->joystick_inputs = joyStickData;
#if 0
printf("Player %d assigned to %s #%d.\n", iPlayer,
  (pPlayer->brain == BRAIN_JOYSTICK) ? "joystick" : "keyboard",
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

    if (pPlayer->brain == BRAIN_INACTIVE)
      continue;

    /* If idle too long (30 seconds), deactivate the player. */

    joyStickData = pPlayer->joystick_inputs;
    if (joyStickData)
      pPlayer->last_brain_activity_time = g_FrameCounter;
    else if (g_FrameCounter - pPlayer->last_brain_activity_time > 30 * 30)
    {
      pPlayer->brain = BRAIN_INACTIVE;
      continue;
    }

    /* If thrusting, speed up in the joystick direction. */

    if (joyStickData & Joy_Button)
    {
      if (joyStickData & Joy_Left)
         ADD_FX(&pPlayer->velocity_x, &gfx_Constant_MinusOne, &pPlayer->velocity_x);
      if (joyStickData & Joy_Right)
         ADD_FX(&pPlayer->velocity_x, &gfx_Constant_One, &pPlayer->velocity_x);
      if (joyStickData & Joy_Up)
         ADD_FX(&pPlayer->velocity_y, &gfx_Constant_MinusOne, &pPlayer->velocity_y);
      if (joyStickData & Joy_Down)
         ADD_FX(&pPlayer->velocity_y, &gfx_Constant_One, &pPlayer->velocity_y);
    }
    else /* Fire not pressed, just steer by rotating velocity direction. */
    {
#if 0
      if (joyStickData & Joy_Right)
      {
        /* Want to increase X velocity to positive, decrease Y velocity to zero.  The invariant is abs(x) + abs(y) = constant.  So first see if X is negative, then we can move X to zero and increase Y with the residue. */
        fx amountToMove;
        SUBTRACT_FX(&pPlayer->velocity_y, &pPlayer->velocity_x, &amountToMove);
        
        gfx_Constant_MinusEighth
        if (IS_NEGATIVE_FX(pPlayer->velocity_x))
        {
          /* Reducing negative X towards zero.  Have to increase Y correspondingly. */
          if (IS_NEGATIVE_FX(pPlayer->velocity_y))
          {
          
          }
          else
          {
          if (COMPARE_FX(pPlayer->velocity_y,
          }
        }
        else /* Increase X as much as possible. */
        {
        }
        
        
      }
      // bleeble
#endif
    }

    /* Add some friction, reduce velocity. */
    fx portionOfVelocity;

    DIV256_FX(pPlayer->velocity_x, portionOfVelocity);
    SUBTRACT_FX(&pPlayer->velocity_x, &portionOfVelocity, &pPlayer->velocity_x);
    DIV256_FX(pPlayer->velocity_y, portionOfVelocity);
    SUBTRACT_FX(&pPlayer->velocity_y, &portionOfVelocity, &pPlayer->velocity_y);
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
    if (pPlayer->brain != BRAIN_INACTIVE &&
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
    if (pPlayer->brain != BRAIN_INACTIVE &&
    pPlayer->sparkle_anim.type != SPRITE_ANIM_NONE &&
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
    if (pPlayer->brain != BRAIN_INACTIVE &&
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

