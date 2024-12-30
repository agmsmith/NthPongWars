/******************************************************************************
 * Nth Pong Wars, players.h for player data structures.
 *
 * The players are represented as ball shaped objects (easier collision
 * detection) and on the NABU are rendered as hardware sprites.  The balls are
 * coloured to reflect the player choice (maybe with a drop-shadow), and
 * animated if a power-up is applied to the object (if several power-ups are
 * active, the animations sequence through the individual power up animations).
 *
 * As they collide with squares on the board which aren't their own colour, they
 * change ownership of the square to the player and then bounce off the square.
 * They have position (in pixels), velocity (pixels per frame) and acceleration
 * (player thrust and world gravity) and bounce off things like walls and other
 * balls.  The collisions are handled with some fancy algorithms so they can
 * move large distances and yet collide with things in the way, and also move
 * slowly at fractional pixels per frame.
 *
 * AGMS20241024 - Start this header file.
 */
#ifndef _PLAYERS_H
#define _PLAYERS_H 1

#include "fixed_point.h"

/* The maximum number of players allowed.  On the NABU, it's limited to 4 due to
   colour palette limits and the maximum number of visible sprites (4 visible,
   more per scan line don't get drawn, and we use sprites for balls). */
#define MAX_PLAYERS 4

/* Variable sized balls are problematic.  Having in-between sizes doesn't add
   much to the game, just a few ones are great for sweeping out large areas.
   But even then, we'd double the animation space needed (we have only 2K of
   VRAM for all frames), so we're sticking with fixed size players.  For nice
   detail on the NABU, we use 16x16 sprites (all sprites have to be the same)
   and just draw an 8x8 ball in the center of the sprite and animate power-up
   effects around the ball.  The player is considered to be spherical (with this
   diameter), centered in the hardware sprite box. */
#define PLAYER_PIXEL_DIAMETER_NORMAL 8

#ifdef NABU_H

/* Offset from the player screen coordinates to the top left corner of the
   sprite.  Since the players are 8 pixels in diameter, centered in the sprite
   box, the offset is 8 pixels in both X and Y. */
#define PLAYER_SCREEN_TO_SPRITE_OFFSET 8 

/* Use this Y sprite position to mark a sprite as not drawable.  It's the same
   as the 0xD0 magic value the hardware uses to mark the end of the sprite list,
   so it's not a value you would normally see as a VDP Y coordinate.
#define SPRITE_NOT_DRAWABLE 208

#endif /* NABU_H */


/* The various brains that can run a player.
*/
typedef enum brain_enum {
  BRAIN_INACTIVE = 0, /* This player is not active, don't display it. */
  BRAIN_KEYBOARD, /* Player controlled by a Human using the keyboard. */
  BRAIN_JOYSTICK, /* Player controlled by a Human using a joystick. */
  BRAIN_NETWORK, /* Player controlled by a remote entity over the network. */
  BRAIN_IDLE,
  /* This one doesn't accelerate, just drifts around.  It's used for
     disconnected players, so they can reconnect within a time delay without
     much happening.  But if they take too long, the player becomes inactive. */
  BRAIN_CONSTANT_SPEED,
  /* Tries to speed up to move at a constant speed in whatever direction it is
     already going in. */
  BRAIN_HOMING,
  /* Tries to go towards another player at a constant speed.  Other player
     choice changes sequentially every 10 seconds. */
  BRAIN_MAX
} player_brain;


/* This is the main player status record.  Where they are, what they are doing.
   Inactive players also get a record, in case they join a game in progress.
*/
typedef struct player_struct {
  fx pixel_center_x;
  fx pixel_center_y;
  /* Position of the center of the player in the play area, in pixels.  Used
     during collision detection to avoid recalculating it.  Need 16 bits since
     the play area can be larger than the NABU screen, so can go over 256 in
     pixel coordinates.  Signed, because players can go off screen, though
     tiles can't, but we do math with both.  The sprite's top left corner has
     to be calculated from the center. */
  uint8_t pixel_flying_height;
    /* How far above the board the player is.  Will draw the shadow sprite
       offset diagonally by this many pixels. 2 is a good value, 1 is hard to
       see, 0 is hidden by the ball (ball on ground?). */

  fx velocity_x;
  fx velocity_y;
  /* The speed the player is moving, in pixels per update. */

  player_brain brain;
    /* What controls this player, or marks it as inactive (not drawn). */

  uint32_t brain_info;
    /* Extra information about this brain.  Joystick number for joysticks,
       network identification for remote players, algorithm stuff for AI. */

  uint8_t main_colour;
  /* Predefined colour for this player's main graphic.  On the NABU this is a
     number from 1 to 15 in the standard TMS9918A palette, see the
     k_PLAYER_COLOURS[iPlayer].main value.  In Unix, using ncurses, this is a
     number from 1 to 4, which we'll calculate from the player index. */

#ifdef NABU_H
  uint8_t vdpSpriteX; /* 0 left, to 255 almost off right side. */
  uint8_t vdpSpriteY;
  /* $E1 past top, $FF=-1 for fully visible at top of screen, 191 off bottom,
     207 maximum past bottom, 208=$D0 to end sprite list.  We also use that
     magic value to flag sprites that are off screen and shouldn't be drawn. */
  uint8_t vdpEarlyClock32Left; /* 0, or $80 for shift left 32 pixels. */
  /* Ball location converted to sprite coordinates.  The main ball sprite and
     the sparkle sprite are at these locations.  The shadow sprite is offset
     a bit to show depth. */
  uint8_t vdpShadowSpriteX;
  uint8_t vdpShadowSpriteY;
  uint8_t vdpShadowEarlyClock32Left;
  
  SpriteAnimationType main_anim_type; /* Animation for the main ball sprite. */
  SpriteAnimRecord main_anim; /* A copy of the related animation data. */

  uint8_t shadow_colour;
  /* Predefined colour for this player's shadow sprite.  We're using colours
     which have a darker version of the same colour, so that can be a
     drop-shadow.  Caches the values from k_PLAYER_COLOURS[player#].
     The shadow sprite will be showing the main animation, just offset a bit. */

  uint8_t sparkle_colour;
  /* Predefined colour for this player's sparkle sprite.  It's usually used for
     power-up animations and other such things around the ball.  Caches the
     values from k_PLAYER_COLOURS[player#]. */
  SpriteAnimationType sparkle_anim_type; /* Which sparkle animation, or none. */
  SpriteAnimRecord sparkle_anim;
#endif /* NABU_H */
} player_record, *player_pointer;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a global array sized in advance to be
   as large as possible, and we just flag inactive players (can be
   non-sequential as joysticks are picked up or go idle). */
extern player_record g_player_array[MAX_PLAYERS];

extern void InitialisePlayers(void);
/* Set up the initial player data, mostly colours and animations. */

extern void UpdatePlayerAnimations(void);
/* Update animations for the next frame.  Also convert location of the player
   to hardware pixel coordinates. */


#ifdef NABU_H
extern void CopyPlayersToSprites(void);
/* Copy all the players to hardware sprites.  Returns the number of the next
   free sprite.  Inactive players don't use any sprites.  Also inactive sparkle
   animations don't use a sprite.  They are copied in priority order, all balls,
   then sparkles, then shadows.  Should be the first thing updated as the
   vertical blanking starts, since sprites updated during display look bad. */
#endif /* NABU_H */

#endif /* _PLAYERS_H */

