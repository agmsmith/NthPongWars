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
/* Use this Y sprite position to mark a sprite as not drawable.  It's the same
   as the 0xD0 magic value the hardware uses to mark the end of the sprite list,
   so it's not a value you would normally see as a VDP Y coordinate. */
#define SPRITE_NOT_DRAWABLE 208
#endif /* NABU_H */


/* The various brains that can run a player.
*/
typedef enum brain_enum {
  BRAIN_INACTIVE = 0, /* This player is not active, don't display it. */
  BRAIN_KEYBOARD, /* Player controlled by a Human using the local keyboard. */
  BRAIN_JOYSTICK, /* Player controlled by a Human using joystick #n. */
  BRAIN_NETWORK, /* Player controlled by a remote entity over the network. */
  BRAIN_CONSTANT_SPEED,
  /* Tries to speed up to move at a constant speed in whatever direction it is
     already going in. */
  BRAIN_HOMING,
  /* Tries to go towards another player at a constant speed.  Other player
     choice changes sequentially every 10 seconds. */
  BRAIN_MAX
} player_brain;


/* Joystick directions as bits.  0 bit means not pressed.  Whole byte is zero if
   no buttons are pressed.  Same as NABU's JOYSTICKENUM, and our own definition
   for other computer systems.  If you see some buttons being pressed, it means
   a user wants to take over a player.
*/
#ifdef NABU_H
  #define joystick_enum JOYSTICKENUM
#else
typedef enum joystick_enum {
    Joy_Left = 0b00000001,
    Joy_Down = 0b00000010,
    Joy_Right = 0b00000100,
    Joy_Up = 0b00001000,
    Joy_Button = 0b00010000,
  };
#endif

extern uint8_t g_KeyboardFakeJoystickStatus;
/* Your device specific keyboard code puts an emulated joystick state in this
   global, usually reflecting the cursor keys and some other button for fire */

#ifdef NABU_H
  /* NABU-LIB already has a global with joystick status, just use it. */
  #define g_JoystickStatus _joyStatus
#else /* Need to collect joystick data in our own global array instead. */
  extern uint8_t g_JoystickStatus[4];
#endif


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

  uint8_t velocity_octant;
  /* Direction the velocity is going in, calculated from velocity x,y but only
     when needed, by UpdatePlayerVelocityDirection().  Actually this is the
     lower boundary of the octant, the upper one is 45 degrees clockwise from
     this. */

  bool velocity_octant_right_on;
  /* Set to TRUE if the velocity is right along the velocity_octant.  Needed so
     that we can aim for the next more counter-clockwise octant.  The octant
     finding code carefully uses the lower bound if the velocity is exactly on
     an octant direction. */ 

  fx step_velocity_x;
  fx step_velocity_y;
  /* The speed the player is moving, in pixels per step, with several steps per
    frame update to ensure that each step doesn't advance more than one tile
    width so that we don't skip over tiles and miss collisions.  This is a
    temporary value used by Simulate(). */

  uint8_t joystick_inputs;
  /* What action is the player currently requesting?  See joystick_enum for
     the various bits.  Zero if no buttons pressed.  Human, AI and remote
     players all talk to the simulation code through these five bits. */

  player_brain brain;
  /* What controls this player, or marks it as inactive (not drawn). */

  union brain_info_union {
    uint8_t iJoystick; /* For BRAIN_JOYSTICK, which joystick number? */
  } brain_info;
  /* Extra information about this brain.  Joystick number for joysticks,
     network identification for remote players, algorithm stuff for AI. */

  uint16_t last_brain_activity_time;
  /* Frame counter when the last brain activity was seen (joystick moved off
     neutral), so we can detect idle players.  After about 30 seconds, we
     change that player slot to idle. */

  uint16_t score;
  /* Current score of each player.  Counts the number of tiles they own, plus
    extra points for aged tiles.  Once a player has reached the goal score,
    the game is over.  Gave up on doing it as percentages, too CPU expensive.
    Score incremental updates are done in SetTileOwner(). */

  uint16_t score_displayed;
  /* The score value currently being shown on the display and in score_text.
     Don't need to update the display if it's the same as score. */

  char score_text[8];
  /* Text version of the score, corresponds to score_displayed.  May have a
     following % sign, and at most 5 digits, and a NUL.  On the NABU it can use
     different letters in the font for coloured digits for each player. */

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

  SpriteAnimRecord main_anim;/* Animation for the main ball sprite. */

/* Now just using VDP_BLACK for extra contrast.
  uint8_t shadow_colour;
  /* Predefined colour for this player's shadow sprite.  We're using colours
     which have a darker version of the same colour, so that can be a
     drop-shadow.  Caches the values from k_PLAYER_COLOURS[player#].
     The shadow sprite will be showing the main animation, just offset a bit. */

  uint8_t sparkle_colour;
  /* Predefined colour for this player's sparkle sprite.  It's usually used for
     power-up animations and other such things around the ball.  Caches the
     values from k_PLAYER_COLOURS[player#]. */

  SpriteAnimRecord sparkle_anim;
#endif /* NABU_H */
} player_record, *player_pointer;

/* An array of players.  To avoid using alloc/free with resulting fragmentation
   of the minuscule NABU memory, it is a global array sized in advance to be
   as large as possible, and we just flag inactive players (can be
   non-sequential as joysticks are picked up or go idle). */
extern player_record g_player_array[MAX_PLAYERS];

/* Calculated by InitialisePlayers(), these are the adjusted pixel boundaries
   of the play area.  Adjusted by the ball radius to make collision tests
   easier.  If the ball center position beyond this value, it has hit
   the wall. */
extern fx g_play_area_wall_bottom_y;
extern fx g_play_area_wall_left_x;
extern fx g_play_area_wall_right_x;
extern fx g_play_area_wall_top_y;

extern void InitialisePlayers(void);
/* Set up the initial player data, mostly colours and animations. */

extern void UpdatePlayerAnimations(void);
/* Update animations for the next frame.  Also convert location of the player
   to hardware pixel coordinates. */

extern void UpdatePlayerInputs(void);
/* Process the joystick and keyboard inputs.  Input activity assigns a player
   to the joystick or keyboard.  Also updates brains to generate fake joystick
   inputs if needed.  Then use the joystick inputs to modify the player's
   velocities (steering if just specifying a direction, thrusting if the fire
   button is pressed too). */

#ifdef NABU_H
extern void CopyPlayersToSprites(void);
/* Copy all the players to hardware sprites.  Returns the number of the next
   free sprite.  Inactive players don't use any sprites.  Also inactive sparkle
   animations don't use a sprite.  They are copied in priority order, all balls,
   then sparkles, then shadows.  Should be the first thing updated as the
   vertical blanking starts, since sprites updated during display look bad. */
#endif /* NABU_H */

#endif /* _PLAYERS_H */

