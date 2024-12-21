/******************************************************************************
 * Nth Pong Wars, players.c for player drawing and thinking.
 *
 * AGMS20241220 - Started this code file.
 */

#include "players.h"

#ifdef NABU_H
#include "Art/NthPong1.h" /* Artwork data definitions. */

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


/* Set up the initial player data, mostly colours and animations. */
void InitialisePlayers(void)
{
  uint8_t iPlayer;
  uint16_t pixelCoord;
  player_pointer pPlayer;

  pixelCoord = 32; /* Scatter players across screen. */
  for (iPlayer = 0, pPlayer = g_player_array; iPlayer < MAX_PLAYERS;
  iPlayer++, pPlayer++)
  {
    INT_TO_FX(pixelCoord, pPlayer->pixel_center_x);
    INT_TO_FX(pixelCoord, pPlayer->pixel_center_y);
    pixelCoord += 32;
    pPlayer->brain = BRAIN_INACTIVE + 1 /* bleeble */;
    pPlayer->brain_info = 0;
    pPlayer->main_colour =
#ifdef NABU_H
      k_PLAYER_COLOURS[iPlayer].main;
#else /* ncurses uses 1 to 4.  init_pair() sets it up, 0 for B&W text. */
      iPlayer + 1;
#endif

#ifdef NABU_H
    pPlayer->main_anim_type = SPRITE_ANIM_BALL_ROLLING;
    pPlayer->main_anim = g_SpriteAnimData[pPlayer->main_anim_type];
    pPlayer->shadow_colour = k_PLAYER_COLOURS[iPlayer].shadow;
    pPlayer->sparkle_colour = k_PLAYER_COLOURS[iPlayer].sparkle;
    pPlayer->sparkle_anim_type = SPRITE_ANIM_NONE;
    pPlayer->sparkle_anim = g_SpriteAnimData[pPlayer->sparkle_anim_type];
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

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != BRAIN_INACTIVE)
    {
      UpdateOneAnimation(&pPlayer->main_anim);
      if (pPlayer->sparkle_anim_type != SPRITE_ANIM_NONE)
        UpdateOneAnimation(&pPlayer->sparkle_anim);

      /* Update the sprite screen coordinates. */

      screenX = GET_FX_INTEGER(pPlayer->pixel_center_x);
      screenX -= 8 /* Half of hardware Sprite width; center -> top left. */;

      screenY = GET_FX_INTEGER(pPlayer->pixel_center_y);
      screenY -= 8 /* Half of hardware Sprite width; center -> top left. */;

      if (screenX <= -16 || screenY <= -16 || screenX >= 256 || screenY >= 192)
      { /* Sprite is off screen, make it hidden off bottom. */
        pPlayer->vdpEarlyClock32Left = 0;
        pPlayer->vdpSpriteX = 255;
        pPlayer->vdpSpriteY = 192;
      }
      else /* Partly or fully on screen. */
      {
        pPlayer->vdpSpriteY = screenY;
        if (screenX < 0) /* Hardware hack to do more than 256 coordinates. */
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
uint8_t CopyPlayersToSprites(void)
{
  uint8_t spriteCount;
  uint8_t iPlayer;
  player_pointer pPlayer;

  spriteCount = 0;
  vdp_setWriteAddress(_vdpSpriteAttributeTableAddr);

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != BRAIN_INACTIVE)
    {
      /* Do the main ball sprite.  Presumably always present. */
      IO_VDPDATA = pPlayer->vdpSpriteY;
      IO_VDPDATA = pPlayer->vdpSpriteX;
      IO_VDPDATA = pPlayer->main_anim.current_name;
      IO_VDPDATA = pPlayer->vdpEarlyClock32Left | pPlayer->main_colour;
      spriteCount++;
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  /* Do the sparkle sprites. */

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != BRAIN_INACTIVE)
    {
      if (pPlayer->sparkle_anim_type != SPRITE_ANIM_NONE)
      {
        /* Do the sparkle power-up sprite.  Same location as ball. */
        IO_VDPDATA = pPlayer->vdpSpriteY;
        IO_VDPDATA = pPlayer->vdpSpriteX;
        IO_VDPDATA = pPlayer->sparkle_anim.current_name;
        IO_VDPDATA = pPlayer->vdpEarlyClock32Left | pPlayer->sparkle_colour;
        spriteCount++;
      }
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  /* Do the shadow sprites.  Same as ball sprite, but different colour and
     slightly offset.  In future could offset it more to show ball flying. */

  iPlayer = MAX_PLAYERS - 1;
  pPlayer = g_player_array;
  do {
    if (pPlayer->brain != BRAIN_INACTIVE)
    {
      /* Do the shadow ball sprite.  Presumably always present. */
      IO_VDPDATA = pPlayer->vdpSpriteY + 1;
      IO_VDPDATA = pPlayer->vdpSpriteX + 1;
      IO_VDPDATA = pPlayer->main_anim.current_name;
      IO_VDPDATA = pPlayer->vdpEarlyClock32Left | pPlayer->shadow_colour;
      spriteCount++;
    }
    pPlayer++;
  } while (iPlayer-- != 0);

  return spriteCount;
}
#endif /* NABU_H */

